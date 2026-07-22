set(PCK "openjph")

if (${PCK}_FOUND)
  return()
endif()

find_path(${PCK}_INCLUDE_DIR
  NAMES openjph/ojph_base.h
  HINTS
    ${PRAGMA_DEPS_DIR}/openjph/include
)

find_library(${PCK}_LIBRARY
  NAMES openjph
  HINTS
    ${PRAGMA_DEPS_DIR}/openjph/lib
)

set(REQ_VARS ${PCK}_LIBRARY ${PCK}_INCLUDE_DIR)
if(WIN32)
  find_file(
    ${PCK}_OPENJPH_RUNTIME
    NAMES openjph.0.25.dll
    HINTS ${PRAGMA_DEPS_DIR}/openjph/bin
  )
  set(REQ_VARS ${REQ_VARS} ${PCK}_OPENJPH_RUNTIME)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${PCK}
  REQUIRED_VARS ${REQ_VARS}
)

if(${PCK}_FOUND)
  set(${PCK}_LIBRARIES   ${${PCK}_LIBRARY})
  set(${PCK}_INCLUDE_DIRS ${${PCK}_INCLUDE_DIR})
  if(WIN32)
    set(${PCK}_RUNTIME ${${PCK}_OPENJPH_RUNTIME})
  endif()
endif()
