#include "Game.hpp"
#include "Application.hpp"

#include <iostream>
#include <random>

using namespace DirectX;

struct VertexPosColor
{
    XMFLOAT3 Position;
    XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[] = {
    {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f)},
    {XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
    {XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
    {XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f)},
    {XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
    {XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f)},
    {XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f)},
    {XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
};

static WORD g_Indices[] = {
    0, 1, 2, 2, 1, 3,

    4, 6, 5, 5, 6, 7,

    0, 2, 4, 4, 2, 6,

    1, 5, 3, 3, 5, 7,

    5, 1, 4, 4, 1, 0,

    2, 3, 6, 6, 3, 7,
};

Game::Game(Application *application, int width, int height)
    : m_ScissorRect{0, 0, LONG_MAX, LONG_MAX},
      m_Viewport{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f},
      m_Width(width),
      m_Height(height)
{
    PDevice              device       = Application::Get()->GetDevice();
    CommandQueue        &commandQueue = Application::Get()->GetCommandQueueCopy();
    PGraphicsCommandList commandList  = commandQueue.GetCommandList();

    PResource intermediateVertexBuffer;
    UpdateBufferResource(commandList,
                         &m_VertexBuffer,
                         &intermediateVertexBuffer,
                         _countof(g_Vertices),
                         sizeof(VertexPosColor),
                         g_Vertices,
                         D3D12_RESOURCE_FLAG_NONE);
    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes    = sizeof(g_Vertices);
    m_VertexBufferView.StrideInBytes  = sizeof(VertexPosColor);

    PResource intermediateIndexBuffer;
    UpdateBufferResource(commandList,
                         &m_IndexBuffer,
                         &intermediateIndexBuffer,
                         _countof(g_Indices),
                         sizeof(WORD),
                         g_Indices,
                         D3D12_RESOURCE_FLAG_NONE);
    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.SizeInBytes    = sizeof(g_Indices);
    m_IndexBufferView.Format         = DXGI_FORMAT_R16_UINT;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors             = 1;
    heapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Assert(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DSVHeap)));

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

    D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
                                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
                                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
                                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / sizeof(DWORD), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_ROOT_SIGNATURE_DESC sigDesc(1, rootParameters, 0, nullptr, flags);

    PBlob rootSigBlob;
    PBlob errorBlob;
    Assert(D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob));
    Assert(device->CreateRootSignature(
        0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    ReloadShaders();

    UINT64 fenceValue = commandQueue.ExecuteCommandList(commandList);
    commandQueue.WaitForFenceValue(fenceValue);
    m_ContentLoaded = true;
    ResizeDepthBuffer(width, height);
}

void Game::ReloadShaders()
{
    PDevice device = Application::Get()->GetDevice();
    PBlob   vertexShaderBlob;
    PBlob   pixelShaderBlob;
    PBlob   errorBlob;

#ifdef _DEBUG
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    Assert(D3DReadFileToBlob(L"Vertex.cso", &vertexShaderBlob));
    Assert(D3DReadFileToBlob(L"Pixel.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION",
         0, DXGI_FORMAT_R32G32B32_FLOAT,
         0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",
         0, DXGI_FORMAT_R32G32B32_FLOAT,
         0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

    gpsDesc.pRootSignature    = m_RootSignature.Get();
    gpsDesc.VS                = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    gpsDesc.PS                = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    gpsDesc.SampleMask        = UINT_MAX;
    gpsDesc.BlendState        = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    gpsDesc.RasterizerState   = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
    gpsDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());

    // ��� ��������� � reversed Z
    gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    gpsDesc.InputLayout.pInputElementDescs = inputLayout;
    gpsDesc.InputLayout.NumElements        = _countof(inputLayout);
    gpsDesc.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gpsDesc.NumRenderTargets               = 1;
    gpsDesc.RTVFormats[0]                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    gpsDesc.DSVFormat                      = DXGI_FORMAT_D32_FLOAT;
    gpsDesc.SampleDesc.Count               = 1;
    gpsDesc.SampleDesc.Quality             = 0;

    Assert(device->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&m_PipelineStateLess)));

    gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
    Assert(device->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&m_PipelineStateGreater)));
}

void Game::UpdateBufferResource(PGraphicsCommandList commandList,
                                ID3D12Resource     **destinationResource,
                                ID3D12Resource     **intermediateResource,
                                size_t               numElements,
                                size_t               elementSize,
                                const void          *bufferData,
                                D3D12_RESOURCE_FLAGS flags)
{
    PDevice device     = Application::Get()->GetDevice();
    size_t  bufferSize = numElements * elementSize;
    Assert(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                           D3D12_HEAP_FLAG_NONE,
                                           &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
                                           D3D12_RESOURCE_STATE_COPY_DEST,
                                           nullptr,
                                           IID_PPV_ARGS(destinationResource)));
    if (!bufferData)
        return;
    Assert(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                           D3D12_HEAP_FLAG_NONE,
                                           &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                           nullptr,
                                           IID_PPV_ARGS(intermediateResource)));

    D3D12_SUBRESOURCE_DATA data = {};
    data.pData                  = bufferData;
    data.RowPitch               = bufferSize;
    data.SlicePitch             = bufferSize;
    UpdateSubresources(commandList.Get(), *destinationResource, *intermediateResource, 0, 0, 1, &data);
}

void Game::ResizeDepthBuffer(int width, int height)
{
    if (!m_ContentLoaded)
        return;
    Application::Get()->Flush();
    width          = (std::max)(1, width);
    height         = (std::max)(1, height);
    PDevice device = Application::Get()->GetDevice();

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format            = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil      = {1.0f, 0};

    Assert(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_DepthBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format                        = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice            = 0;
    dsv.Flags                         = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
}

void Game::OnResize(int width, int height)
{
    if (width == m_Width && height == m_Height)
        return;
    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    ResizeDepthBuffer(width, height);
    m_Width  = width;
    m_Height = height;
}

void Game::OnUpdate()
{
    static uint64_t                           frameCounter = 0;
    static std::chrono::high_resolution_clock clock;
    static auto                               epoch   = clock.now();
    static auto                               tSecond = clock.now();
    static auto                               t0      = clock.now();

    static std::default_random_engine      randomEngine(std::random_device{}());
    static std::normal_distribution<float> distribution(0.0, 1.0);

    ++frameCounter;
    auto   t1 = clock.now();
    double dt = std::chrono::duration<double>(t1 - t0).count();
    t0        = t1;

    double elapsedSeconds = std::chrono::duration<double>(t1 - tSecond).count();
    if (elapsedSeconds > 1.0)
    {
        std::wstringstream ss;
        ss << L"TPS: " << frameCounter / elapsedSeconds << '\n';
        OutputDebugStringW(ss.str().c_str());
        frameCounter = 0;
        tSecond      = t1;
    }

    double   timeTotal    = std::chrono::duration<double>(t1 - epoch).count();
    double   angle        = timeTotal;
    XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    m_ModelMatrix         = DirectX::XMMatrixRotationAxis(rotationAxis, static_cast<float>(angle));
    XMVECTOR eyePosition  = XMVectorSet(0, -10, 0, 1);
    XMVECTOR focusPoint   = XMVectorSet(0, 0, 0, 1);
    XMVECTOR upDir        = XMVectorSet(0, 0, 1, 0);
    m_ViewMatrix          = DirectX::XMMatrixLookAtLH(eyePosition, focusPoint, upDir);

    double aspectRatio = m_Width / static_cast<double>(m_Height);

    double fov    = 45.0f * exp(-0.001f * m_FovStep);
    double tanFov = tan(0.5f * fov);

    double shiftX = 0.02f * m_ShakeStrength * distribution(randomEngine);
    double shiftY = 0.02f * m_ShakeStrength * distribution(randomEngine);

    double x1 = (shiftX - tanFov) * aspectRatio;
    double x2 = (shiftX + tanFov) * aspectRatio;
    double y1 = shiftY - tanFov;
    double y2 = shiftY + tanFov;
    double z1 = 0.1;
    double z2 = 100.0;

    m_ProjectionMatrix = XMMATRIX(static_cast<float>(2.0 / (x2 - x1)),                          // row 0 col 0
                                  0.0f,                                                         // row 1 col 0
                                  0.0f,                                                         // row 2 col 0
                                  0.0f,                                                         // row 3 col 0
                                  0.0f,                                                         // row 0 col 1
                                  static_cast<float>(2.0 / (y2 - y1)),                          // row 1 col 1
                                  0.0f,                                                         // row 2 col 1
                                  0.0f,                                                         // row 3 col 1
                                  static_cast<float>((x1 + x2) / (x1 - x2)),                    // row 0 col 2
                                  static_cast<float>((y1 + y2) / (y1 - y2)),                    // row 1 col 2
                                  static_cast<float>((m_ZLess ? z2 : -z1) / (z2 - z1)),         // row 2 col 2
                                  1.0f,                                                         // row 3 col 2
                                  0.0f,                                                         // row 0 col 3
                                  0.0f,                                                         // row 1 col 3
                                  static_cast<float>((m_ZLess ? -1 : 1) * z1 * z2 / (z2 - z1)), // row 2 col 3
                                  0.0f                                                          // row 3 col 3
    );
    m_ShakeStrength *= exp(-dt);
}

void Game::OnRender()
{
    CommandQueue        &commandQueue     = Application::Get()->GetCommandQueueDirect();
    PGraphicsCommandList commandList      = commandQueue.GetCommandList();

    UINT      currentBackBufferIndex = Application::Get()->CurrentBackBufferIndex();
    PResource backBuffer             = Application::Get()->CurrentBackBuffer();
    auto      rtv                    = Application::Get()->CurrentRTV();
    auto      dsv                    = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

    TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    FLOAT clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    if (m_ZLess)
    {
        commandList->SetPipelineState(m_PipelineStateLess.Get());
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }
    else
    {
        commandList->SetPipelineState(m_PipelineStateGreater.Get());
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
    }

    commandList->RSSetViewports(1, &m_Viewport);
    commandList->RSSetScissorRects(1, &m_ScissorRect);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    commandList->SetGraphicsRootSignature(m_RootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    commandList->IASetIndexBuffer(&m_IndexBufferView);

    XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
    mvpMatrix          = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(mvpMatrix) / 4, &mvpMatrix, 0);
    commandList->DrawIndexedInstanced(_countof(g_Indices), 1, 0, 0, 0);

    TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    m_FenceValues[currentBackBufferIndex] = commandQueue.ExecuteCommandList(commandList);
    Application::Get()->Present();
    commandQueue.WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
}
