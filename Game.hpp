#pragma once

#include "pch.hpp"

#include "MyDXLib/Camera.hpp"
#include "MyDXLib/Scene.hpp"
#include "MyDXLib/ShaderCompiler.hpp"
#include "MyDXLib/Utils.hpp"

class Application;

class Game
{
    Mesh  m_CubeMesh;
    Mesh  m_ScreenMesh;
    Scene m_SponzaScene;

    PResource m_ColorBuffer;
    PResource m_DepthBuffer;

    std::optional<DescriptorHeap> m_DSVHeap;
    std::optional<DescriptorHeap> m_TextureHeap;

    ShaderCompiler m_ShaderCompiler{std::move(std::filesystem::path(__FILE__).remove_filename())};

    PRootSignature m_RootSignatureCube;
    PRootSignature m_RootSignatureFilter;
    PRootSignature m_RootSignatureSponza;

    PPipelineState m_PipelineStateCubeLess;
    PPipelineState m_PipelineStateCubeGreater;
    PPipelineState m_PipelineStateSponzaLess;
    PPipelineState m_PipelineStateSponzaGreater;

    PPipelineState m_PipelineStateFilter;
    D3D12_RECT     m_ScissorRect;

    DirectX::XMMATRIX m_ModelMatrix;
    Camera            m_Camera;

    int m_Width;
    int m_Height;

    int m_LastMouseX = 0;
    int m_LastMouseY = 0;

    bool m_ContentLoaded = false;

  public:
    explicit Game(Application *application, int width, int height);

    void ReloadShaders();
    void ResizeBuffers(int width, int height);

    void OnResize(int width, int height);
    void OnMouseDown(int x, int y);
    void OnMouseDrag(int x, int y);
    void OnUpdate();
    void OnRender();

    int    m_FovStep       = 0;
    double m_ShakeStrength = 0.0;
    bool   m_ZLess         = true;

    bool m_MoveForward = false;
    bool m_MoveBack    = false;
    bool m_MoveLeft    = false;
    bool m_MoveRight   = false;

    void TransitionResource(PGraphicsCommandList  commandList,
                            PResource             resource,
                            D3D12_RESOURCE_STATES beforeState,
                            D3D12_RESOURCE_STATES afterState)
    {
        CD3DX12_RESOURCE_BARRIER barrier
            = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
        commandList->ResourceBarrier(1, &barrier);
    }
};
