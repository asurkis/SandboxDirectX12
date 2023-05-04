#pragma once

#include "pch.hpp"

class Application;

class Game
{
    PResource                m_VertexBuffer1;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView1;
    PResource                m_IndexBuffer1;
    D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView1;

    PResource                m_VertexBuffer2;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView2;

    PResource m_ColorBuffer;
    PResource m_DepthBuffer;

    PDescriptorHeap m_RTVHeap;
    PDescriptorHeap m_DSVHeap;
    PRootSignature  m_RootSignature1;
    PRootSignature  m_RootSignature2;
    PPipelineState  m_PipelineStateLess;
    PPipelineState  m_PipelineStateGreater;
    PPipelineState  m_PipelineStatePost;
    D3D12_RECT      m_ScissorRect;

    PCommandAllocator    m_CommandAllocator;
    PGraphicsCommandList m_CommandList;

    DirectX::XMMATRIX m_ModelMatrix;
    DirectX::XMMATRIX m_ViewMatrix;
    DirectX::XMMATRIX m_ProjectionMatrix;

    int m_Width;
    int m_Height;

    UINT64 m_FenceValues[3];

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
    void ResizeDepthBuffer(int width, int height);

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
