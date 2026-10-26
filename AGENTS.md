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
- Style check: `python support/check_style_guide.py`

## Rules
- Config file format changes must update `sgct.schema.json`, the example files in `config/`, and the tests in `tests/`.
- Verify every user-facing change (config options, command-line arguments, public API, behavior) against the documentation at https://github.com/sgct/sgct.dev. If the docs are outdated by the change, update that repository as well.
