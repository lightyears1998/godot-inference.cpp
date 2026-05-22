set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# ggml[cuda]
# https://github.com/ggml-org/llama.cpp/blob/master/docs/build.md#override-compute-capability-specifications
# ======
# Compute Capability
# 61: GTX 1060
# 75: RTX 2060
if("${PORT}" STREQUAL "ggml")
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_CUDA_ARCHITECTURES:STRING=61\\;75-real")
endif()
