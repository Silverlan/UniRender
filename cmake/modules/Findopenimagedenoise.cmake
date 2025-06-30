set(PCK "openimagedenoise")

if (${PCK}_FOUND)
  return()
endif()

find_path(${PCK}_INCLUDE_DIR
  NAMES OpenImageDenoise/oidn.hpp
  HINTS
    ${PRAGMA_DEPS_DIR}/openimagedenoise/include
)

find_library(${PCK}_LIBRARY
  NAMES OpenImageDenoise
  HINTS
    ${PRAGMA_DEPS_DIR}/openimagedenoise/lib
)

find_library(${PCK}_CORE_LIBRARY
  NAMES OpenImageDenoise_core libOpenImageDenoise_core.so.2.2.2
  HINTS
    ${PRAGMA_DEPS_DIR}/openimagedenoise/lib
)

find_library(${PCK}_DEVICE_CPU_LIBRARY
  NAMES OpenImageDenoise_device_cpu libOpenImageDenoise_device_cpu.so.2.2.2
  HINTS
    ${PRAGMA_DEPS_DIR}/openimagedenoise/lib
)

find_library(${PCK}_DEVICE_CUDA_LIBRARY
  NAMES OpenImageDenoise_device_cuda libOpenImageDenoise_device_cuda.so.2.2.2
  HINTS
    ${PRAGMA_DEPS_DIR}/openimagedenoise/lib
)

find_library(${PCK}_DEVICE_HIP_LIBRARY
  NAMES OpenImageDenoise_device_hip libOpenImageDenoise_device_hip.so.2.2.2
  HINTS
    ${PRAGMA_DEPS_DIR}/openimagedenoise/lib
)

find_library(${PCK}_DEVICE_SYCL_LIBRARY
  NAMES OpenImageDenoise_device_sycl libOpenImageDenoise_device_sycl.so.2.2.2
  HINTS
    ${PRAGMA_DEPS_DIR}/openimagedenoise/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${PCK}
  REQUIRED_VARS ${PCK}_LIBRARY ${PCK}_CORE_LIBRARY ${PCK}_DEVICE_CPU_LIBRARY ${PCK}_DEVICE_CUDA_LIBRARY ${PCK}_DEVICE_HIP_LIBRARY ${PCK}_DEVICE_SYCL_LIBRARY ${PCK}_INCLUDE_DIR
)

if(${PCK}_FOUND)
  set(${PCK}_LIBRARIES   ${${PCK}_LIBRARY} ${${PCK}_CORE_LIBRARY})
  set(${PCK}_device_LIBRARIES   ${${PCK}_DEVICE_CPU_LIBRARY} ${${PCK}_DEVICE_CUDA_LIBRARY} ${${PCK}_DEVICE_HIP_LIBRARY} ${${PCK}_DEVICE_SYCL_LIBRARY})
  set(${PCK}_INCLUDE_DIRS ${${PCK}_INCLUDE_DIR})
endif()
