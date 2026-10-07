cmake_minimum_required(VERSION 3.21)

project(easy-vulkan-qemu-launcher VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Test)

set(LAUNCHER_CORE_SOURCES
    src/launcher_config.cpp
    src/overlay_manager.cpp
    src/qemu_process_controller.cpp
)

add_executable(qemu-vulkan-launcher WIN32
    src/main.cpp
    src/main_window.cpp
    src/main_window.h
    src/launcher_config.h
    src/overlay_manager.h
    src/qemu_process_controller.h
    ${LAUNCHER_CORE_SOURCES}
)
target_include_directories(qemu-vulkan-launcher PRIVATE src)
target_link_libraries(qemu-vulkan-launcher PRIVATE Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    add_executable(qemu-launcher-core-tests
        tests/core_tests.cpp
        src/launcher_config.h
        src/overlay_manager.h
        ${LAUNCHER_CORE_SOURCES}
    )
    target_include_directories(qemu-launcher-core-tests PRIVATE src)
    target_link_libraries(qemu-launcher-core-tests PRIVATE Qt6::Test)
    add_test(NAME qemu-launcher-core-tests COMMAND qemu-launcher-core-tests)
endif()
