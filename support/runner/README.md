# Runner
The _Runner_ is a Python script that renders the image tests in the `visualtests` folder and submits the images to the [regression server](https://regression.openspaceproject.com), which compares each image with the reference image for the hardware that rendered it. The images are submitted for the "sgct" application of the regression server. The `requests` PIP package is needed to submit the images, for example installed using `pip install -r requirements.txt`.

## Tests
Each image test is an SGCT configuration file in a subfolder of `visualtests`, together with the files that the configuration refers to, such as masks or correction meshes. The subfolders group the tests by their scenario, for example `capture`, `cluster`, `correction`, `masks`, `projections`, `stereo`, and `window`. Relative paths in the configuration files are resolved relative to the configuration file. A new test only requires a new configuration file; every configuration file in `visualtests` is also checked against the schema and loaded by the `Roundtrip` unit test.

The _Runner_ loads each configuration into the `calibrator` with the `--image-test` argument. In that mode, the calibrator renders its test pattern box, takes a screenshot of the back buffer of every window in the fifth frame, which includes the correction meshes, the masks, and the composited stereo images, and terminates. For configurations with more than one node, a calibrator is started for every node on this computer using the `--local` argument, so the nodes need different addresses (for example `127.0.0.1` and `127.0.0.2`) and different ports. The server waits for the clients in every frame if the configuration uses `"firmsync": true`, which guarantees that every node renders the frame with the screenshot.

The group of a test on the regression server consists of the folders of the configuration file relative to `visualtests`, joined with `-`, and its name is the filename without the extension. A test that creates a single image is submitted under this name, for example `projections/fisheye-crop`. If a test creates multiple images, for example for multiple windows or nodes, the name of each screenshot without its number is appended, for example `capture/two-windows-first` or `cluster/two-nodes-node1_client`.

## Running
The _Runner_ needs a build of the calibrator, for example created with:
```
cmake --preset windows-msvc
cmake --build --preset windows-msvc-relwithdebinfo --target calibrator
```

Executing `main.py --help` returns all command-line arguments that can be used to customize the execution:

| Parameter | Description |
| --------- | ----------- |
| `--test` | A comma-separated list of the tests that should be run, given as their path relative to the `visualtests` folder without the extension, for example `projections/fisheye`. Wildcards are supported, for example `stereo/*`. If this is omitted, all tests are run. |
| `--test-id` | A comma-separated list of images, identified by the group and name under which they are submitted to the regression server, for example `capture/two-windows-first`. Only the tests that can create these images are run, and only these images are submitted. This is used by the regression server to rerun a single test and cannot be combined with `--test`. |
| `--executable` | The path to the calibrator. On Windows, this defaults to the executable of the `windows-msvc-relwithdebinfo` build preset, on Linux to the one of the `linux-ninja-release` preset. |
| `--results` | The folder in which the images are stored if they are not submitted. Defaults to the `results` folder next to `main.py`. |
| `--store-locally` | Stores the images in the results folder even if a `config.json` exists. |

Example: `python main.py --test "projections/fisheye*,stereo/*"`

The _Runner_ returns an error code if a calibrator fails or does not create any images, or if an image could not be submitted. The images of a test that failed are submitted nevertheless. As the regression server counts every line of the submitted log as an error, only the warnings and errors that SGCT logged are submitted for a test that passed, and the whole output of the calibrators for a test that failed.

The Docker test runner in `support/docker` also renders all image tests with Mesa's llvmpipe after the unit tests, which checks the rendering with the sanitizers and includes it in the coverage report. These images are only stored and never submitted.

## Submitting
A `config.json` next to `main.py` is needed to submit the images to the regression server. The `config.sample.json` provides a stub that can be used as the starting point for it. If no `config.json` exists, all images are instead stored in the results folder, with one subfolder per group, so that they can be inspected manually. As the first image that is submitted for a test becomes its reference image, the images of new or changed tests should be inspected this way before they are submitted.

The `config.json` specifies the URL of the regression server, the hardware under which the images are submitted, and a runner id that has to be provided by the administrator of the regression server. The hardware should be of the form "operating system"-"graphics card vendor", for example "windows-nvidia". The runner id works like a password, so the `config.json` should not be shared or committed. The regression server only accepts hardware, group, and test names that start with a letter or a digit and only contain letters, digits, `.`, `_`, and `-`. The _Runner_ refuses to start if the configured hardware does not follow this rule and does not run tests whose group or name does not. If the server rejects a submission, the _Runner_ prints the server's error message together with an explanation of the error.

The `Jenkinsfile.imagetesting` in the root of the repository builds the calibrator and runs the _Runner_ every night on the image testing nodes, and it can be started by the regression server to rerun a single test.
