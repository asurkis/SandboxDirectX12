cmake_minimum_required(VERSION 3.0)
project(SandboxDirectX12)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(DirectX-Headers)

add_custom_target(SHADERS)

set(SHADER_FILES Vertex Pixel)
set_source_files_properties( Vertex.hlsl PROPERTIES ShaderType vs )
set_source_files_properties(  Pixel.hlsl PROPERTIES ShaderType ps )

foreach(FILE ${SHADER_FILES})
    get_source_file_property(SHADER_TYPE "${FILE}.hlsl" ShaderType)
    add_custom_command(
        TARGET SHADERS
        COMMAND dxc.exe 
            "-T" "${SHADER_TYPE}_6_0" 
            "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.hlsl" 
            "-Fo" "${FILE}.cso" 
            "-Zi" "-Fd" "${FILE}.pdb"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.hlsl"
    )
endforeach()

add_executable(${PROJECT_NAME} WIN32
    main.cpp
    Commons.hpp
    Application.hpp Application.cpp
    Utils.hpp
    Game.hpp Game.cpp
    MainWindow.hpp
    Device.hpp
    CommandQueue.hpp
)

add_dependencies(${PROJECT_NAME} SHADERS)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/DirectX-Headers/include/directx")
target_link_libraries(${PROJECT_NAME} PRIVATE DirectX-Headers DirectX-Guids d3d12.lib dxgi.lib d3dcompiler.lib)
