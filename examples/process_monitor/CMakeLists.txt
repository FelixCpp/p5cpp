add_executable(process_monitor main.cpp process_stats.cpp)
target_link_libraries(process_monitor PRIVATE p5cpp)
