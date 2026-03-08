cmake_minimum_required(VERSION 3.16)
project(TodoDesktop VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

add_executable(TodoDesktop
    main.cpp
    mainwindow.cpp
    todomanager.cpp
    todocontainerwidget.cpp
    todoitemwidget.cpp

)

set_target_properties(TodoDesktop PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

#ADD ICONS

#qt_add_resources(TodoDesktop "resources"
#    PREFIX "/"
#    FILES
#        resources/icon.ico
#)

#if(WIN32)
    #add_custom_command(
    #    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.o
    #    COMMAND windres ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc -o ${CMAKE_CURRENT_BINARY_DIR}/resources.o
    #    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc
    #    COMMENT "Generating resources.o (Windows icon resource)"
    #)

    #target_sources(TodoDesktop PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources.o)
#endif()

target_link_libraries(TodoDesktop PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    target_link_libraries(TodoDesktop PRIVATE user32)
endif()

if(WIN32)
    set_target_properties(TodoDesktop PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
