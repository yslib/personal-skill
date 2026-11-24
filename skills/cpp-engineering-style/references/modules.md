# Item 1. Give modules a common build vocabulary

A source module groups code with one responsibility and one include policy. A linkable artifact is something the project builds or ships. Decide these independently. kuai compiles core, runtime, and profiler as object modules and assembles the host library at the C API target. A separately built vendor package has its own entry point and SDK requirements.

## Establish the layout

| Path, relative to project root | Contents and audience |
| --- | --- |
| `CMakeLists.txt`, `CMakePresets.json` | Project entry, build products, installation, and supported configurations. |
| `cmake/ProjectModule.cmake`, `cmake/toolchains/` | Shared module helper and compiler or SDK selection. Keep machine-specific settings in toolchains or presets. |
| `src/CMakeLists.txt` | Module inventory and `add_subdirectory` calls. Keep dependency edges in the owning target declarations. |
| `src/<module>/CMakeLists.txt` | One module declaration plus its actual dependencies and compile requirements. |
| `src/<module>/include/<package>/<module>/` | Self-contained public headers. Build-tree and installed callers use the same include spelling. |
| `src/<module>/private/<module>/` | Shared implementation contracts such as a private factory or an accessor used by an explicitly privileged peer. |
| `src/<module>/src/` | Translation units and details used only by those files. File-local names can live in an unnamed namespace. |
| `vendor/include/` and `vendor/<backend>/` | For independent backends, keep common implementation contracts separate from vendor SDK headers, native registration, and vendor-specific translation units. Create these directories when the project needs them. |

## Introduce the helper before multiplying module manifests

Reuse an existing helper in the target project. For a new project, copy [ProjectModule.cmake](../assets/ProjectModule.cmake) to `cmake/`. The asset is an independent, portable implementation of the directory convention. It requires CMake 3.23 or newer for header file sets. It uses target-scoped requirements and leaves language level, exception flags, installation, and external dependencies to the project.

Root of a project named `orbit`:

```cmake
cmake_minimum_required(VERSION 3.23)
project(orbit LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectModule)
include(GNUInstallDirs)
add_subdirectory(src)
```

In `src/core/CMakeLists.txt`, `project_add_module(core)` creates the object target `orbit_core` and alias `orbit::core`. In `src/runtime/CMakeLists.txt`, declare the next target and use native CMake commands for its dependencies:

```cmake
project_add_module(runtime TYPE STATIC)
target_link_libraries(orbit_runtime PUBLIC orbit::core)
```

Choose `PUBLIC` for requirements that consumers of a public header need and `PRIVATE` for implementation dependencies. For a peer with a reason to include `"core/DeviceAccess.h"`, link the build-only `orbit::core::private` facade privately. The facade exposes private headers and the underlying module's usage requirements. When assembling libraries from object targets, link the actual object target into its owning artifact (or supply its object files explicitly). A transitive interface link to an object target only carries usage requirements. Assign each object module to its final artifact once.

The helper accepts `TYPE OBJECT|STATIC|SHARED|INTERFACE`, `PACKAGE` for the include prefix and alias namespace, `TARGET` for the build target name, and `SOURCES` for an explicit source list. Default C++ discovery is scoped to the module's `src/` and uses `CONFIGURE_DEPENDS`. Use explicit sources for generated files, C, CUDA, mixed SDKs, or a project that requires explicit manifests. Enable those languages in the owning project. Header-only interface modules expose public header sets. A private facade exists only when the module has a private directory. Keep the helper small by applying unusual target properties directly at the call site.

## Decide what consumers can install

For an independently shipped library, install the target and its public file set, export it under a namespace, and supply its package config. Export any public dependency or arrange for the config to find it. For a single SDK assembled from object modules, install the aggregate binary and selected public header sets. Give the installed target its package-relative include root and public compile requirements. Keep private header sets and access facades in the build tree. Enable position-independent code on object targets that feed a shared library where required.

Check both views: build a privileged peer against the source tree, then build a separate consumer against a fresh install prefix using only the exported package and `#include <orbit/core/...>`. Verify the install contains no private headers or absolute source-tree paths. These checks catch private includes in public headers and missing transitive dependencies.
