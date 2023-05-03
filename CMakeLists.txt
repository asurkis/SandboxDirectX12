cmake_minimum_required(VERSION 3.10)
project(SandboxDirectX12)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(DirectX-Headers)

set(SHADER_FILES 
    Vertex.hlsl 
    Pixel.hlsl
)

set(CLASSES
    Application
    CommandQueue
    DescriptorAllocator
    Device
    DynamicDescriptorHeap
    Game
    MainWindow
    UploadBuffer
)

add_custom_target(Shaders SOURCES ${SHADER_FILES})

set_source_files_properties( Vertex.hlsl PROPERTIES ShaderType vs VS_SHADER_TYPE Vertex )
set_source_files_properties(  Pixel.hlsl PROPERTIES ShaderType ps VS_SHADER_TYPE  Pixel )

foreach(FILE ${SHADER_FILES})
    set_source_files_properties(${FILE} PROPERTIES VS_SHADER_MODEL 6.0)
    get_filename_component(FILE_WE "${FILE}" NAME_WE)
    get_source_file_property(SHADER_TYPE "${FILE}" ShaderType)
    add_custom_command(
        TARGET Shaders
        COMMAND dxc.exe
            -T "${SHADER_TYPE}_6_0"
            -Zi
            -Fo "${FILE_WE}.cso"
            -Fd "${FILE_WE}.pdb"
            "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}")
endforeach()

set(CPP_FILES "")
foreach(CLS ${CLASSES})
    SET(CPP_FILES ${CPP_FILES} ${CLS}.cpp ${CLS}.hpp)
endforeach()

add_executable(${PROJECT_NAME} WIN32 main.cpp pch.hpp ${CPP_FILES})
target_precompile_headers(${PROJECT_NAME} PRIVATE pch.hpp)

add_dependencies(${PROJECT_NAME} Shaders)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/DirectX-Headers/include/directx")
target_link_libraries(${PROJECT_NAME} PRIVATE DirectX-Headers DirectX-Guids d3d12.lib dxgi.lib d3dcompiler.lib)
