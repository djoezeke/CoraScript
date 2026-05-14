
# Plugins
add_library(io_plugin SHARED io.cpp)
target_include_directories(io_plugin PRIVATE ../include ../src)
set_target_properties(io_plugin PROPERTIES OUTPUT_NAME "io" PREFIX "")

add_library(stl_plugin SHARED stl_plugin.cpp)
target_include_directories(stl_plugin PRIVATE ../include ../src)
set_target_properties(stl_plugin PROPERTIES OUTPUT_NAME "stl" PREFIX "")
