# Vulkan conversion — follow-up plan

Working off the 2026-10-27 assessment. Baseline at start: windows-msvc build
green, ctest 2/2 (config loading only), calibrator -runTests renders real
content on Windows. Item 4 (CI) is intentionally out of scope.

## 1. Wire the Vulkan validation layer (currently unreachable)

- [x] Add `--vulkan-validation` CLI flag in src/commandline.cpp + help text
- [x] Flag sets the Configuration field read by Engine (src/engine.cpp:96-97)
- [x] Verify calibrator runs clean with validation enabled
      (positive "validation layer is enabled" log added in context.cpp;
      0 warnings/errors across a full -runTests pass)

Note: also fixed a stdout-loss bug found while verifying — the CRT's
exit-time iostream flush does not run for the windowed calibrator process,
so buffered std::cout was lost when redirected. Added explicit
std::cout.flush() on both exit paths in calibrator/main.cpp.

## 2. Rendering smoke test in ctest

- [x] Validation ERROR severity sets a flag in vulkan::Context; calibrator
      returns non-zero if any validation error occurred
      (Context::_validationErrors atomic, counted in debugCallback via pUserData;
      calibrator queries hasValidationErrors() before Engine::destroy())
- [x] Generate reference screenshots with -runTests; verify run-to-run
      determinism; commit references to the repo
      (4 shots, bit-identical across runs; committed to tests/reference/)
- [x] ctest entry: run calibrator -runTests with validation, fail on exit code
      (CalibratorRenderSmoke in calibrator/CMakeLists.txt)
- [x] Screenshot comparison against references with tolerance (Catch2 test)
      (tests/test_render_screenshot.cpp; per-channel tol 2, <0.1% pixels may differ;
      SKIPs if no generated screenshots)
- [x] ctest fully green (3/3: CalibratorRenderSmoke, SGCTTest, VcpkgManifestSync)

Notes:
- Moved enable_testing() ahead of add_subdirectory in the top-level CMakeLists so
  add_test in calibrator/ registers (it was processed before enable_testing).
- SGCTTest now links stb_image (sgct::Image pulls it in) when the calibrator is built.
- Finding: all 4 -runTests screenshots are identical — the test scene is static
  (the box renders the whole run; renderGrid is never set true in the scripted
  sequence). Rendering is correct and deterministic; the scene just doesn't change.

## 3. Linux/XCB surface path

- [x] Check for a usable Linux environment (Docker `openspace-ubuntu-2404-gcc13`:
      CMake 4.0, GCC 13, system Vulkan loader + X11/XCB dev, vcpkg; headless)
- [x] Build on Linux (linux-ninja-debug preset): all targets compile + link
      (libsgct.a, calibrator, SGCTTest)
- [x] Fix the Linux-only compile/link errors the build exposed (see notes)
- [ ] Run calibrator on Linux — NOT possible in this environment (no GPU/display
      in the headless container); compile+link is the verifiable scope here

Notes:
- GLFW 3.5 (the vcpkg baseline version) removed the XCB native functions
  (glfwGetXcbInstance/glfwGetXcbWindow), so the XCB surface could not compile.
  Per decision, switched the Linux surface to Xlib (VK_KHR_xlib_surface) using
  GLFW 3.5's glfwGetX11Display/glfwGetX11Window:
  - src/CMakeLists.txt: VK_USE_PLATFORM_XCB_KHR -> VK_USE_PLATFORM_XLIB_KHR
  - src/vulkan/context.cpp: X11/Xlib.h include, GLFW_EXPOSE_NATIVE_X11, Xlib
    surface creation, surfaceExtension() returns VK_KHR_xlib_surface
- The Spout/NDI stream calls in src/window.cpp were unguarded (declarations are
  #if SGCT_HAS_SPOUT||SGCT_HAS_NDI); guarded the 3 call sites so Linux (no
  Spout/NDI) compiles.
- src/openxr.cpp: the global X11 `Window` typedef (pulled in by the Vulkan Xlib
  surface + OpenXR headers) was ambiguous with sgct::Window under
  `using namespace sgct;`; added `using sgct::Window;`.
- stb_image link order: the sgct static lib references stb symbols (image.cpp)
  but only linked the header, so consumers hit undefined refs on the strict GNU
  linker (MSVC is lenient, which is why Windows passed). Added the read impl to
  the sgct target's link interface via $<IF:...,$<LINK_ONLY:...>> for the static
  case; removed the now-redundant direct link from SGCTTest.
- Windows build + ctest re-verified green after these changes (3/3).

## 5. Docs for the new app-facing API

- [x] Locate the document repository (or write in-repo if not found locally)
      (found at A:\work\sgct\sgct-docs — a Sphinx/MyST site)
- [x] Document the app contract: pipeline creation against
      Window::renderPass()/sceneFormat(), recording into RenderData::commandBuffer,
      offscreen targets and stereo layout, composite ordering
      (new page developers/rendering.md, added to the index toctree)

Notes:
- New page developers/rendering.md documents the full contract: the rendering
  model, the shared vulkan::Context, pipeline creation (Window::renderPass() is
  the offscreen target's render pass; Settings.setSceneFormat(window.sceneFormat())),
  the draw callback (RenderData.commandBuffer; viewport/scissor already set as
  dynamic state), offscreen targets and stereo layout, and the composite ordering
  (warp mesh + masks per swapchain layer).
- All API usage in the examples was verified against the actual headers
  (pipeline.h, sceneformat.h, offscreenbuffer.h, context.h, callbackdata.h,
  window.h, definitions.h) and against the calibrator's real draw callback.
- Updated the now-outdated OpenGL references in developers/how-it-works.md
  (the removed initOpenGL callback -> "Initializing Vulkan resources"; the Draw
  section now mentions the command buffer; Pre-Window and Cleanup are
  Vulkan-accurate) and replaced the glClear() example in
  developers/getting-started.md with a minimal Vulkan full-screen-quad example.
- Validated the markdown with a Sphinx/MyST build (sphinx 7.2.6 + myst-parser
  2.0.0): all four developer pages parse and render with no errors.

## 6. Rename stale OpenGL-era public names

- [x] Configuration::useOpenGLDebugContext -> useVulkanValidation
      (include/sgct/commandline.h, src/commandline.cpp, src/engine.cpp)
- [x] Engine::Settings::createDebugContext -> enableValidation
      (include/sgct/engine.h, src/engine.cpp)
- [x] Build + tests green after rename (windows-msvc build clean; ctest 3/3,
      including CalibratorRenderSmoke which runs with --vulkan-validation)

## 7. Deferred items

- [x] Tracy GPU zones: wire the Vulkan context into Tracy (profiling.h:25)
      (TracyVkContext per in-flight command buffer in Window::createCommandBuffers;
      TracyVkZone "Scene"/"Composite" in the render path; TracyVkCollect in
      swapBuffers. Verified: windows-msvc-tracy build green, calibrator -runTests
      with --vulkan-validation runs clean to frame 40, 4 screenshots, no
      validation errors)
- [ ] Zero-copy Spout via VK_EXT_external_memory_win32: assess feasibility,
      implement or document why readback stays

Note on the Tracy wiring: TracyVkCollect must be called while the command buffer
is still in the recording state (before vkEndCommandBuffer) and outside of a
render pass — Tracy records a vkCmdResetQueryPool into the buffer. The initial
placement called it after vkQueueSubmit, which is undefined behavior and crashed
on the first multi-frame run (frame 5). It now sits just before vkEndCommandBuffer
in Window::swapBuffers, after the last render pass has ended.

## Commit strategy

Split the uncommitted conversion into logical commits once the work above is
done (see assessment); do not commit until the user confirms.
