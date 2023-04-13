cmake_minimum_required(VERSION 3.0)
project(SandboxDirectX12)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(DirectX-Headers)
add_executable(${PROJECT_NAME} WIN32
    main.cpp
    Commons.hpp
    Application.hpp
    Utils.hpp
    MainWindow.hpp
    Device.hpp
    CommandQueue.hpp
)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/DirectX-Headers/include/directx")
target_link_libraries(${PROJECT_NAME} PRIVATE DirectX-Headers DirectX-Guids d3d12.lib dxgi.lib)
