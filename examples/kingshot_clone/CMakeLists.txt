add_executable(kingshot_clone
    main.cpp
    camera.cpp
    camera.hpp
    grid.cpp
    grid.hpp
    building.cpp
    building.hpp
)
target_link_libraries(kingshot_clone PRIVATE p5cpp)
