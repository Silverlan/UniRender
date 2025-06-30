set(PCK "opensubdiv")

if (${PCK}_FOUND)
  return()
endif()

find_path(${PCK}_INCLUDE_DIR
  NAMES opensubdiv/version.h
  HINTS
    ${PRAGMA_DEPS_DIR}/opensubdiv/include
)

find_library(${PCK}_LIBRARY
  NAMES osdCPU
  HINTS
    ${PRAGMA_DEPS_DIR}/opensubdiv/lib
)

find_library(${PCK}_GPU_LIBRARY
  NAMES osdGPU
  HINTS
    ${PRAGMA_DEPS_DIR}/opensubdiv/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${PCK}
  REQUIRED_VARS ${PCK}_LIBRARY ${PCK}_GPU_LIBRARY ${PCK}_INCLUDE_DIR
)

if(${PCK}_FOUND)
  set(${PCK}_LIBRARIES   ${${PCK}_LIBRARY} ${${PCK}_GPU_LIBRARY})
  set(${PCK}_INCLUDE_DIRS ${${PCK}_INCLUDE_DIR})
endif()
