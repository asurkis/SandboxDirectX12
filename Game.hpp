#pragma once

#include "pch.hpp"

class Application;

class Game
{
    PResource                m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    PResource                m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView;
    PResource                m_DepthBuffer;

    PDescriptorHeap m_DSVHeap;
    PRootSignature  m_RootSignature;
    PPipelineState  m_PipelineState;
    D3D12_VIEWPORT  m_Viewport;
    D3D12_RECT      m_ScissorRect;

    float             m_FoV;
    DirectX::XMMATRIX m_ModelMatrix;
    DirectX::XMMATRIX m_ViewMatrix;
    DirectX::XMMATRIX m_ProjectionMatrix;

    int m_Width;
    int m_Height;

    UINT64 m_FenceValues[3];

    bool m_ContentLoaded = false;

  public:
    Game(Application *application, int width, int height, bool vSync);
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

    void TransitionResource(PGraphicsCommandList  commandList,
                            PResource             resource,
                            D3D12_RESOURCE_STATES beforeState,
                            D3D12_RESOURCE_STATES afterState)
    {
        CD3DX12_RESOURCE_BARRIER barrier
            = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
        commandList->ResourceBarrier(1, &barrier);
    }

    void ClearRTV(PGraphicsCommandList commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT *clearColor)
    {
        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    void ClearDepth(PGraphicsCommandList commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
    {
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
    }
};
