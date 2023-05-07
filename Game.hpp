#pragma once

#include "pch.hpp"

#include "MyDXLib/Camera.hpp"
#include "MyDXLib/DescriptorHeap.hpp"
#include "MyDXLib/Mesh.hpp"
#include "MyDXLib/Utils.hpp"

class Application;

class Game
{
    Mesh m_CubeMesh;
    Mesh m_ScreenMesh;

    Mesh m_SponzaMesh;

    PResource m_ColorBuffer;
    PResource m_DepthBuffer;

    DescriptorHeap m_DSVHeap;
    DescriptorHeap m_TextureHeap;

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

    bool m_ContentLoaded = false;

  public:
    Game(Application *application, int width, int height);

    void ReloadShaders();

    void UpdateBufferResource(PGraphicsCommandList commandList,
                              ID3D12Resource     **destinationResource,
                              ID3D12Resource     **intermediateResource,
                              size_t               numElements,
                              size_t               elementSize,
                              const void          *bufferData,
                              D3D12_RESOURCE_FLAGS flags);
    void ResizeBuffers(int width, int height);

    void OnResize(int width, int height);
    void OnUpdate();
    void OnRender();

    int    m_FovStep       = 0;
    double m_ShakeStrength = 0.0;
    bool   m_ZLess         = true;

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
