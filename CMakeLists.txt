FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.cpp)
FILE(GLOB_RECURSE include_sources ${CMAKE_SOURCE_DIR}/include/*.hpp)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS ${include_sources})