#pragma once

#include "Commons.hpp"

class Application;

class Game
{
    Application             *m_Application;
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
};
