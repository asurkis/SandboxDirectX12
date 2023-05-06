cmake_minimum_required(VERSION 3.10)
project(SandboxDirectX12)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(DirectX-Headers)

set(SHADER_FILES 
    Vertex1.hlsl 
    Vertex2.hlsl 
    Pixel1.hlsl
    Pixel2.hlsl
)

set(CLASSES
    Application
    Camera
    CommandQueue
    DescriptorHeap
    Game
    MainWindow
)

find_package(assimp REQUIRED)

add_custom_target(Shaders SOURCES ${SHADER_FILES})

set_source_files_properties( Vertex1.hlsl PROPERTIES ShaderType vs VS_SHADER_TYPE Vertex )
set_source_files_properties( Vertex2.hlsl PROPERTIES ShaderType vs VS_SHADER_TYPE Vertex )
set_source_files_properties(  Pixel1.hlsl PROPERTIES ShaderType ps VS_SHADER_TYPE  Pixel )
set_source_files_properties(  Pixel2.hlsl PROPERTIES ShaderType ps VS_SHADER_TYPE  Pixel )

foreach(FILE ${SHADER_FILES})
    set_source_files_properties(${FILE} PROPERTIES VS_SHADER_MODEL 6.0)
    configure_file("${FILE}" "${CMAKE_CURRENT_BINARY_DIR}/${FILE}" COPYONLY)
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

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/DirectX-Headers/include/directx"
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    DirectX-Headers 
    DirectX-Guids 
    d3d12.lib 
    dxgi.lib 
    d3dcompiler.lib
    assimp::assimp
)
