add_executable(paint
    main.cpp
    layer.cpp
    history.cpp
    tools.cpp
    ui.cpp
)
target_link_libraries(paint PRIVATE p5cpp)
