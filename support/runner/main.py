"""
SGCT
Simple Graphics Cluster Toolkit

Copyright (c) 2012-2026
For conditions of distribution and use, see copyright notice in LICENSE.md


Runs the image tests in the `visualtests` folder and submits the rendered images to the
regression server, which compares them with the reference images of the hardware that
rendered them. Each test is a configuration file that is loaded into the calibrator,
which takes a screenshot of the back buffer of every window and terminates. For clusters,
one calibrator is started for every node. The images are submitted as the "sgct"
application, using the folders of a test, joined with '-', as the group and the filename
as the name. If a test creates more than one image, the window name (and the node and
eye) are appended to the name. If no config.json exists next to this script, the images
are stored in a `results` folder next to this script instead.

Examples:
  python main.py
  python main.py --test projections/fisheye,stereo/*
  python main.py --test-id projections/fisheye-crop,capture/two-windows-first
"""

import argparse
import datetime
import fnmatch
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import time

RUNNER_DIR = pathlib.Path(__file__).resolve().parent
ROOT = RUNNER_DIR.parents[1]
TESTS_DIR = ROOT / "visualtests"
CONFIG_FILE = RUNNER_DIR / "config.json"
RESULTS_DIR = RUNNER_DIR / "results"
APPLICATION = "sgct"

# The number of seconds after which the calibrators of a test are stopped. The calibrator
# terminates on its own a few frames after starting, so this is only reached if it hangs
TEST_TIMEOUT = 120

# The regression server only accepts group, test, and hardware names that start with a
# letter or digit and only contain letters, digits, '.', '_', and '-'. Keep this in sync
# with the `NamePattern` in the regression server's src/validation.ts
NAME_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$")

# Explanations for the errors that the regression server returns for a submission
SUBMISSION_ERRORS = {
    400: "The server rejected the values of the submission",
    401: "The runner 'id' in the config.json is not accepted by the server",
    409: "A test run with the same timestamp was already submitted for this test",
    413: "The image or the log is too large",
    429: "Too many failed attempts, the server is rejecting requests for a while"
}


class Test:
    """
    A single image test, which is a configuration file in the `visualtests` folder
    """
    def __init__(self, path):
        self.path = path
        # The path relative to the visualtests folder without the extension, for example
        # "projections/fisheye"
        self.relative = path.relative_to(TESTS_DIR).with_suffix("").as_posix()
        parts = self.relative.split("/")
        self.group = "-".join(parts[:-1])
        self.name = parts[-1]

        with open(path) as f:
            self.nodes = len(json.load(f)["nodes"])


def default_executable():
    """
    Returns the path at which the calibrator is located after building the
    windows-msvc-relwithdebinfo or the linux-ninja-release preset
    """
    if os.name == "nt":
        folder = ROOT / "build" / "windows-msvc" / "bin" / "RelWithDebInfo"
        return folder / "calibrator.exe"
    else:
        return ROOT / "build" / "linux-ninja-release" / "bin" / "calibrator"


def find_tests():
    """
    Returns all tests in the visualtests folder
    """
    return [Test(path) for path in sorted(TESTS_DIR.rglob("*.json"))]


def select_by_path(tests, patterns):
    """
    Returns the tests whose path relative to the visualtests folder, without extension,
    matches one of the `patterns`, which can contain wildcards. Raises a `SystemExit` if a
    pattern does not match any test
    """
    selected = []
    for pattern in patterns:
        matches = [t for t in tests if fnmatch.fnmatchcase(t.relative, pattern)]
        if not matches:
            raise SystemExit(f"No test matches '{pattern}'")
        selected += [t for t in matches if t not in selected]
    return selected


def select_by_id(tests, ids):
    """
    Returns the tests that might create the images with the provided `group/name` ids.
    The image of a test that creates a single image has the name of the test, the images
    of other tests start with the name of the test followed by a '-'. If a test has the
    exact name of the image, only that test is selected. Raises a `SystemExit` if no test
    can create one of the images
    """
    selected = []
    for test_id in ids:
        group, _, name = test_id.rpartition("/")
        candidates = [t for t in tests if t.group == group]
        matches = [t for t in candidates if t.name == name]
        if not matches:
            matches = [t for t in candidates if name.startswith(f"{t.name}-")]
        if not matches:
            raise SystemExit(f"No test can create the image '{test_id}'")
        selected += [t for t in matches if t not in selected]
    return selected


def commit_hash():
    """
    Returns the hash of the commit that is checked out in the repository, or an empty
    string if it cannot be determined
    """
    try:
        result = subprocess.run(
            ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
            check=True, capture_output=True, text=True
        )
        return result.stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def run_test(executable, test, folder):
    """
    Runs the calibrator for every node of the `test` and returns whether all of them
    succeeded, the number of seconds it took, and their combined output. The screenshots
    are written into the `folder`
    """
    screenshots = folder / "screenshots"
    screenshots.mkdir()
    processes = []
    start = time.perf_counter()
    for node in range(test.nodes):
        command = [
            str(executable),
            "--config", str(test.path),
            "--image-test",
            "--screenshot-path", str(screenshots),
            "--local", str(node)
        ]
        if node > 0:
            command.append("--client")
        if test.nodes > 1:
            command.append("--add-node-name-in-screenshot")
        log = open(folder / f"node{node}.log", "w+", errors="replace")
        # The calibrator loads its test patterns from the working directory
        process = subprocess.Popen(
            command, cwd=executable.parent, stdout=log, stderr=subprocess.STDOUT,
            text=True
        )
        processes.append((process, log))

    passed = True
    output = ""
    deadline = start + TEST_TIMEOUT
    for node, (process, log) in enumerate(processes):
        try:
            process.wait(timeout=max(deadline - time.perf_counter(), 0))
            if process.returncode != 0:
                passed = False
                message = f"Node {node} exited with code {process.returncode}"
            else:
                message = ""
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            passed = False
            message = f"Node {node} was stopped after {TEST_TIMEOUT} seconds"
        log.seek(0)
        content = log.read()
        log.close()
        if test.nodes > 1:
            output += f"Node {node}:\n"
        output += content
        if message:
            output += f"{message}\n"
    return passed, time.perf_counter() - start, output


def test_images(test, folder):
    """
    Returns the screenshots that the `test` created in the `folder` as a list of
    `(name, path)` tuples
    """
    files = sorted((folder / "screenshots").glob("*.png"))
    if len(files) == 1:
        return [(test.name, files[0])]
    # The screenshots are called [nodeN_]<window>_[L_|R_]<number>.png
    return [(f"{test.name}-{re.sub(r'_[0-9]+$', '', f.stem)}", f) for f in files]


def submission_log(passed, output):
    """
    Returns the log that is submitted together with the images of a test. The regression
    server counts every line of the log as an error, so for a test that passed, only the
    warnings and errors that SGCT logged are submitted. For a test that failed, the whole
    output is submitted
    """
    if not passed:
        return output
    lines = output.splitlines()
    return "\n".join(l for l in lines if l.startswith(("(Warning)", "(Error)")))


def submit_image(url, runner, hardware, group, name, timestamp, timing, commit, image,
                 log):
    """
    Submits an image to the regression server and returns whether it was accepted
    """
    # Only imported here so that the images can be stored locally without the package
    import requests

    try:
        with open(image, "rb") as f:
            res = requests.post(
                url,
                # The runner id is sent first, so that the server can reject an unknown
                # runner before the image is uploaded
                data={
                    "runnerID": runner,
                    "application": APPLICATION,
                    "group": group,
                    "name": name,
                    "hardware": hardware,
                    "timestamp": timestamp,
                    "timing": timing,
                    "commitHash": commit
                },
                files={
                    "file": f,
                    "log": log
                },
                # Seconds to wait for the connection and for the server to process the
                # image
                timeout=(30, 600)
            )
    except requests.RequestException as e:
        print(f"  Submitting {group}/{name} failed: {e}")
        return False

    if res.status_code == 200:
        print(f"  Submitted {group}/{name}")
        return True

    try:
        message = res.json().get("error", res.text)
    except ValueError:
        message = res.text
    print(f"  Submitting {group}/{name} failed with error {res.status_code}: {message}")
    if res.status_code in SUBMISSION_ERRORS:
        print(f"    {SUBMISSION_ERRORS[res.status_code]}")
    return False


def store_image(results, group, name, image):
    """
    Copies the image into the `results` folder
    """
    destination = results / group / f"{name}.png"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy(image, destination)
    print(f"  Stored {group}/{name} in {destination}")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    tests = parser.add_mutually_exclusive_group()
    tests.add_argument(
        "-t", "--test",
        help="A comma-separated list of the tests that are run, given as their path "
            "relative to the visualtests folder without the extension, for example "
            "\"projections/fisheye\". Wildcards are supported, for example "
            "\"stereo/*\". If this is omitted, all tests are run"
    )
    tests.add_argument(
        "--test-id",
        help="A comma-separated list of the images that are submitted, identified by the "
            "group and name under which they are submitted to the regression server, for "
            "example \"capture/two-windows-first\". Only the tests that can create these "
            "images are run, and only these images are submitted. This is used by the "
            "regression server to rerun a single test"
    )
    parser.add_argument(
        "--executable",
        type=pathlib.Path,
        default=default_executable(),
        help="The path to the calibrator executable. Defaults to the location of the "
            "windows-msvc-relwithdebinfo build on Windows and of the linux-ninja-release "
            "build on Linux"
    )
    parser.add_argument(
        "--results",
        type=pathlib.Path,
        default=RESULTS_DIR,
        help="The folder in which the images are stored if they are not submitted. "
            "Defaults to the results folder next to this script"
    )
    parser.add_argument(
        "--store-locally",
        action="store_true",
        help="Store the images in the results folder even if a config.json exists, for "
            "example when rendering on a computer whose images should not be submitted"
    )
    args = parser.parse_args()

    total_start = time.perf_counter()

    submit = CONFIG_FILE.exists() and not args.store_locally
    if submit:
        with open(CONFIG_FILE) as f:
            config = json.load(f)
        submit_url = f"{config['url'].rstrip('/')}/api/submit-test"
        hardware = config["hardware"]
        runner_id = config["id"]
        print(f"Submit URL: {submit_url}")
        print(f"Hardware: {hardware}")
        # The runner id is a secret, so only its beginning is shown to identify it
        print(f"ID: {runner_id[:4]}{'*' * max(len(runner_id) - 4, 0)}")
        if not NAME_PATTERN.match(hardware):
            raise SystemExit(
                f"The hardware '{hardware}' cannot be submitted to the regression "
                "server. Names must start with a letter or digit and only contain "
                "letters, digits, '.', '_', and '-'"
            )
    elif args.store_locally:
        print(f"Test results will be stored in {args.results}")
    else:
        print("No 'config.json' provided. Test results will be stored locally instead")

    executable = args.executable.resolve()
    if not executable.exists():
        raise SystemExit(f"Could not find executable '{executable}'")

    all_tests = find_tests()
    wanted = None
    if args.test_id is not None:
        wanted = {t.strip() for t in args.test_id.split(",") if t.strip()}
        print(f"Submitting images: {sorted(wanted)}")
        selected = select_by_id(all_tests, wanted)
    elif args.test is not None:
        selected = select_by_path(all_tests, [t.strip() for t in args.test.split(",")])
    else:
        selected = all_tests
    commit = commit_hash()
    print(f"Commit: {commit}")

    failed_tests = []
    failed_submissions = []
    found = set()
    for i, test in enumerate(selected, start=1):
        print(f"[{i}/{len(selected)}] {test.relative}", flush=True)
        if not NAME_PATTERN.match(test.group) or not NAME_PATTERN.match(test.name):
            print("  Skipping the test as its group or name is not valid")
            failed_tests.append(test.relative)
            continue

        timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat()
        with tempfile.TemporaryDirectory(prefix="sgct-runner-") as folder:
            folder = pathlib.Path(folder)
            passed, timing, output = run_test(executable, test, folder)
            images = test_images(test, folder)
            if not images:
                output += "The test did not create any images\n"
                passed = False
            if not passed:
                print(output, flush=True)
                failed_tests.append(test.relative)
            print(f"  {'Passed' if passed else 'Failed'} after {timing:.2f} seconds")

            # The images of a test that failed are submitted nevertheless, as the failure
            # might be unrelated to the images
            log = submission_log(passed, output)
            for name, image in images:
                test_id = f"{test.group}/{name}"
                if wanted is not None and test_id not in wanted:
                    continue
                found.add(test_id)
                if not NAME_PATTERN.match(name):
                    print(f"  Skipping {test_id} as it is not a valid name")
                    failed_submissions.append(test_id)
                elif submit:
                    accepted = submit_image(
                        submit_url, runner_id, hardware, test.group, name, timestamp,
                        timing, commit, image, log
                    )
                    if not accepted:
                        failed_submissions.append(test_id)
                else:
                    store_image(args.results, test.group, name, image)

    print(f"Total time for all tests: {time.perf_counter() - total_start:.2f} seconds")

    success = True
    if failed_tests:
        print(f"{len(failed_tests)} tests failed:")
        for test in failed_tests:
            print(f"  {test}")
        success = False
    if failed_submissions:
        print(f"{len(failed_submissions)} images could not be submitted:")
        for test_id in failed_submissions:
            print(f"  {test_id}")
        success = False
    if wanted is not None and wanted - found:
        print(f"No test created the images: {sorted(wanted - found)}")
        success = False
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
