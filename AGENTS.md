# AGENTS.md
## Build & test
### Windows
- Configure: `cmake --preset windows-msvc`
- Build: `cmake --build --preset windows-msvc-debug`
- Test: `build/windows-msvc-debug/bin/SGCTTest`
- Style check: `python support/check_style_guide.py`

### Linux
- Configure: `cmake --preset linux-ninja-debug`
- Build: `cmake --build --preset linux-ninja-debug`
- Test: `build/linux-ninja-debug/bin/SGCTTest`
- Image tests: `python support/runner/main.py` (needs the `calibrator` target)
- Style check: `python support/check_style_guide.py`

## Rules
- The image tests are configuration files in `visualtests/<group>/`, which the runner in `support/runner` renders with the calibrator and submits to the regression server (https://regression.openspaceproject.com). The server compares them with the reference images of each hardware, so no reference images are stored in this repository. Changes to the rendering must update or add image tests. Without a `config.json`, the runner stores the images in `support/runner/results`; inspect them for new or changed tests, as their first submission becomes the reference image. The C++ tests in `tests/visual` only contain checks that do not compare with reference images.
- Config file format changes must update `sgct.schema.json`, the example files in `config/`, and the tests in `tests/`.
- Verify every user-facing change (config options, command-line arguments, public API, behavior) against the documentation at https://github.com/sgct/sgct.dev. If the docs are outdated by the change, update that repository as well.
