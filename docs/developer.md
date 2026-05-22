# Developer Docs

## Dependencies via vcpkg

### CUDA

``` powershell
$env:VCPKG_OVERLAY_TRIPLETS="triplets"
$env:VCPKG_DEFAULT_TRIPLET="x64-windows-ggml-61"

vcpkg install ggml[cuda]
vcpkg install llama-cpp
```

References:

- <https://github.com/ggml-org/llama.cpp/blob/master/docs/build.md#override-compute-capability-specifications>
- CMAKE_CUDA_ARCHITECTURES <https://github.com/ggml-org/llama.cpp/blob/4f0e43da6f8f6e9390d88409610098ec2d2dc5c7/ggml/src/ggml-cuda/CMakeLists.txt#L8>

## Adding document for the plugin

``` shell
# Inside the `project` folder:
godot --doctool ../ --gdextension-docs
# The command above generates XML files inside the `doc_classes` folder.
```

---

## Useful Link

- <https://docs.godotengine.org/en/stable/tutorials/scripting/cpp/build_system/cmake.html#secondary-build-system-working-with-cmake>
- <https://docs.godotengine.org/en/latest/tutorials/scripting/cpp/gdextension_docs_system.html>
