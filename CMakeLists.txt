include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME util_raytracing)
pr_add_library(${PROJ_NAME} SHARED)

pr_add_dependency(${PROJ_NAME} util_ocio TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_udm TARGET PUBLIC)

find_package(openimagedenoise REQUIRED)
pr_add_external_dependency(${PROJ_NAME} openimagedenoise LIBRARY PUBLIC)

find_package(opensubdiv REQUIRED)
pr_add_external_dependency(${PROJ_NAME} opensubdiv LIBRARY PUBLIC)

pr_init_module(${PROJ_NAME})

pr_add_compile_definitions(${PROJ_NAME} -DENABLE_TEST_AMBIENT_OCCLUSION PUBLIC)

pr_add_compile_definitions(${PROJ_NAME} -DOIIO_STATIC_BUILD -DGLEW_STATIC -DRTUTIL_DLL)

if(WIN32)
	target_link_libraries(${PROJ_NAME}
		PUBLIC
		opengl32
		glu32
	)
endif()

pr_finalize(${PROJ_NAME})
