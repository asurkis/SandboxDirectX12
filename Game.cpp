#include "Game.hpp"
#include "Application.hpp"
#include "MyDXLib/SceneData.hpp"
#include <cmath>

using namespace DirectX;

struct VertexPosColor
{
    XMFLOAT3 Position;
    XMFLOAT3 Color;
};

static const VertexPosColor g_CubeVertices[] = {
    {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f)},
    {XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
    {XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
    {XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f)},
    {XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
    {XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f)},
    {XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f)},
    {XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
};

static const WORD g_CubeIndices[] = {
    0, 1, 2, 2, 1, 3,

    4, 6, 5, 5, 6, 7,

    0, 2, 4, 4, 2, 6,

    1, 5, 3, 3, 5, 7,

    5, 1, 4, 4, 1, 0,

    2, 3, 6, 6, 3, 7,
};

static const XMFLOAT2 g_FullScreen[] = {
    XMFLOAT2(0.0f, 0.0f),
    XMFLOAT2(0.0f, 1.0f),
    XMFLOAT2(1.0f, 1.0f),
    XMFLOAT2(1.0f, 1.0f),
    XMFLOAT2(1.0f, 0.0f),
    XMFLOAT2(0.0f, 0.0f),
};

Game::Game(Application *application, int width, int height)
    : m_ScissorRect{0, 0, LONG_MAX, LONG_MAX},
      m_Width(width),
      m_Height(height)
{
    PDevice              device       = Application::Get()->GetDevice();
    CommandQueue        &commandQueue = Application::Get()->GetCommandQueueCopy();
    PGraphicsCommandList commandList  = commandQueue.ResetCommandList();

    MeshData cubeData;
    MeshData fullScreenData;

    cubeData.InitData(g_CubeVertices, _countof(g_CubeVertices), g_CubeIndices, _countof(g_CubeIndices));
    fullScreenData.InitVertices(g_FullScreen, 6);

    // sponzaData.LoadFromFile("C:\\Users\\asurk\\Documents\\Main.1_Sponza\\", "NewSponza_Main_glTF_002.gltf");
    SceneData sponzaData;
    auto scenePath = std::filesystem::path(__FILE__).remove_filename() / "3rd-party" / "Sponza" / "glTF" / "Sponza.gltf";
    sponzaData.LoadFromFile(scenePath);

    ResourceUploadBatch upload(device.Get());
    upload.Begin(D3D12_COMMAND_LIST_TYPE_COPY);

    m_CubeMesh.QueryInit(device, upload, cubeData);
    m_ScreenMesh.QueryInit(device, upload, fullScreenData);

    m_DSVHeap.emplace(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

    m_TextureHeap.emplace(device.Get(),
                          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                          D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                          1 + sponzaData.TextureCount());

    m_SponzaScene.QueryInit(device, upload, *m_TextureHeap, sponzaData);
    ReloadShaders();
    upload.End(commandQueue.Get().Get()).wait();

    m_ContentLoaded = true;
    ResizeBuffers(width, height);

    m_Camera.Pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_Camera.Rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void Game::ReloadShaders()
{
    PDevice device = Application::Get()->GetDevice();

#ifdef _DEBUG
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    PBlob vertexShaderCubeBlob;
    PBlob vertexShaderFilterBlob;
    PBlob vertexShaderSponzaBlob;
    PBlob pixelShaderCubeBlob;
    PBlob pixelShaderFilterBlob;
    PBlob pixelShaderSponzaBlob;
    PBlob errorBlob;

    Assert(D3DReadFileToBlob(L"VertexCube.cso", &vertexShaderCubeBlob));
    Assert(D3DReadFileToBlob(L"VertexSponza.cso", &vertexShaderSponzaBlob));
    Assert(D3DReadFileToBlob(L"VertexFilter.cso", &vertexShaderFilterBlob));
    Assert(D3DReadFileToBlob(L"PixelCube.cso", &pixelShaderCubeBlob));
    Assert(D3DReadFileToBlob(L"PixelSponza.cso", &pixelShaderSponzaBlob));
    Assert(D3DReadFileToBlob(L"PixelFilter.cso", &pixelShaderFilterBlob));

    Assert(device->CreateRootSignature(0,
                                       vertexShaderCubeBlob->GetBufferPointer(),
                                       vertexShaderCubeBlob->GetBufferSize(),
                                       IID_PPV_ARGS(m_RootSignatureCube.ReleaseAndGetAddressOf())));
    Assert(device->CreateRootSignature(0,
                                       vertexShaderSponzaBlob->GetBufferPointer(),
                                       vertexShaderSponzaBlob->GetBufferSize(),
                                       IID_PPV_ARGS(m_RootSignatureSponza.ReleaseAndGetAddressOf())));
    Assert(device->CreateRootSignature(0,
                                       vertexShaderFilterBlob->GetBufferPointer(),
                                       vertexShaderFilterBlob->GetBufferSize(),
                                       IID_PPV_ARGS(m_RootSignatureFilter.ReleaseAndGetAddressOf())));

    D3D12_INPUT_ELEMENT_DESC inputLayoutCube[] = {
        {"POSITION",
         0, DXGI_FORMAT_R32G32B32_FLOAT,
         0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",
         0, DXGI_FORMAT_R32G32B32_FLOAT,
         0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_INPUT_ELEMENT_DESC inputLayoutSponza[] = {
        {"POSITION",
         0, DXGI_FORMAT_R32G32B32_FLOAT,
         0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"UV",
         0, DXGI_FORMAT_R32G32_FLOAT,
         0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_INPUT_ELEMENT_DESC inputLayoutFilter[] = {
        {"UV",
         0, DXGI_FORMAT_R32G32_FLOAT,
         0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

    gpsDesc.pRootSignature    = m_RootSignatureCube.Get();
    gpsDesc.VS                = CD3DX12_SHADER_BYTECODE(vertexShaderCubeBlob.Get());
    gpsDesc.PS                = CD3DX12_SHADER_BYTECODE(pixelShaderCubeBlob.Get());
    gpsDesc.SampleMask        = UINT_MAX;
    gpsDesc.BlendState        = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    gpsDesc.RasterizerState   = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
    gpsDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());

    // Для сравнения с reversed Z
    gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    gpsDesc.InputLayout.pInputElementDescs = inputLayoutCube;
    gpsDesc.InputLayout.NumElements        = _countof(inputLayoutCube);
    gpsDesc.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gpsDesc.NumRenderTargets               = 1;
    gpsDesc.RTVFormats[0]                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    gpsDesc.DSVFormat                      = DXGI_FORMAT_D32_FLOAT;
    gpsDesc.SampleDesc.Count               = 1;
    gpsDesc.SampleDesc.Quality             = 0;

    Assert(
        device->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(m_PipelineStateCubeLess.ReleaseAndGetAddressOf())));

    gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
    Assert(device->CreateGraphicsPipelineState(&gpsDesc,
                                               IID_PPV_ARGS(m_PipelineStateCubeGreater.ReleaseAndGetAddressOf())));

    gpsDesc                   = {};
    gpsDesc.pRootSignature    = m_RootSignatureSponza.Get();
    gpsDesc.VS                = CD3DX12_SHADER_BYTECODE(vertexShaderSponzaBlob.Get());
    gpsDesc.PS                = CD3DX12_SHADER_BYTECODE(pixelShaderSponzaBlob.Get());
    gpsDesc.SampleMask        = UINT_MAX;
    gpsDesc.BlendState        = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    gpsDesc.RasterizerState   = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
    gpsDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());

    gpsDesc.InputLayout.pInputElementDescs = inputLayoutSponza;
    gpsDesc.InputLayout.NumElements        = _countof(inputLayoutSponza);
    gpsDesc.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gpsDesc.NumRenderTargets               = 1;
    gpsDesc.RTVFormats[0]                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    gpsDesc.DSVFormat                      = DXGI_FORMAT_D32_FLOAT;
    gpsDesc.SampleDesc.Count               = 1;
    gpsDesc.SampleDesc.Quality             = 0;

    Assert(device->CreateGraphicsPipelineState(&gpsDesc,
                                               IID_PPV_ARGS(m_PipelineStateSponzaLess.ReleaseAndGetAddressOf())));

    gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
    Assert(device->CreateGraphicsPipelineState(&gpsDesc,
                                               IID_PPV_ARGS(m_PipelineStateSponzaGreater.ReleaseAndGetAddressOf())));

    gpsDesc                 = {};
    gpsDesc.pRootSignature  = m_RootSignatureFilter.Get();
    gpsDesc.VS              = CD3DX12_SHADER_BYTECODE(vertexShaderFilterBlob.Get());
    gpsDesc.PS              = CD3DX12_SHADER_BYTECODE(pixelShaderFilterBlob.Get());
    gpsDesc.SampleMask      = UINT_MAX;
    gpsDesc.BlendState      = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
    // gpsDesc.DepthStencilState;
    gpsDesc.InputLayout.pInputElementDescs = inputLayoutFilter;
    gpsDesc.InputLayout.NumElements        = _countof(inputLayoutFilter);
    gpsDesc.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gpsDesc.NumRenderTargets               = 1;
    gpsDesc.RTVFormats[0]                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    // gpsDesc.DSVFormat;
    gpsDesc.SampleDesc.Count   = 1;
    gpsDesc.SampleDesc.Quality = 0;
    Assert(device->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(m_PipelineStateFilter.ReleaseAndGetAddressOf())));
}

void Game::ResizeBuffers(int width, int height)
{
    if (!m_ContentLoaded)
        return;
    Application::Get()->Flush();
    width          = (std::max)(1, width);
    height         = (std::max)(1, height);
    PDevice device = Application::Get()->GetDevice();

    D3D12_CLEAR_VALUE colorClearValue = {};
    colorClearValue.Format            = DXGI_FORMAT_R8G8B8A8_UNORM;
    colorClearValue.Color[0]          = 1.0f;
    colorClearValue.Color[1]          = 0.75f;
    colorClearValue.Color[2]          = 0.5f;
    colorClearValue.Color[3]          = 1.0f;

    D3D12_CLEAR_VALUE depthClearValue = {};
    depthClearValue.Format            = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil      = {1.0f, 0};

    Assert(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &colorClearValue,
        IID_PPV_ARGS(m_ColorBuffer.ReleaseAndGetAddressOf())));

    Assert(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(m_DepthBuffer.ReleaseAndGetAddressOf())));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvDesc.ViewDimension                 = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice            = 0;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format                        = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice            = 0;
    dsvDesc.Flags                         = D3D12_DSV_FLAG_NONE;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip       = 0;
    srvDesc.Texture2D.MipLevels             = -1;
    srvDesc.Texture2D.PlaneSlice            = 0;
    srvDesc.Texture2D.ResourceMinLODClamp   = 0.0f;

    device->CreateRenderTargetView(m_ColorBuffer.Get(), &rtvDesc, Application::Get()->IntermediateRTV());
    device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsvDesc, m_DSVHeap->GetFirstCpuHandle());
    device->CreateShaderResourceView(m_ColorBuffer.Get(), &srvDesc, m_TextureHeap->GetFirstCpuHandle());
}

void Game::OnResize(int width, int height)
{
    if (width == m_Width && height == m_Height)
        return;
    ResizeBuffers(width, height);
    m_Width  = width;
    m_Height = height;
}

void Game::OnMouseDown(int x, int y)
{
    m_LastMouseX = x;
    m_LastMouseY = y;
}

void Game::OnMouseDrag(int x, int y)
{
    constexpr float PI = 3.14159265358979323846f;

    int   dx          = x - m_LastMouseX;
    int   dy          = y - m_LastMouseY;
    float aspectRatio = m_Width / static_cast<float>(m_Height);
    m_Camera.Rot.x -= 2.0f * PI * dy / static_cast<float>(m_Height);
    m_Camera.Rot.y -= 2.0f * PI * aspectRatio * dx / static_cast<float>(m_Width);

    m_Camera.Rot.x = (std::max)(-0.5f * 3.14159265358979323846f, (std::min)(0.5f * PI, m_Camera.Rot.x));

    m_LastMouseX = x;
    m_LastMouseY = y;
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

    XMVECTOR pos     = XMLoadFloat3(&m_Camera.Pos);
    XMVECTOR forward = dt * m_Camera.Forward();
    XMVECTOR right   = dt * m_Camera.Right();
    if (m_MoveForward)
        pos += forward;
    if (m_MoveBack)
        pos -= forward;
    if (m_MoveLeft)
        pos -= right;
    if (m_MoveRight)
        pos += right;

    XMStoreFloat3(&m_Camera.Pos, pos);

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

    double aspectRatio = m_Width / static_cast<double>(m_Height);

    double fov    = 45.0f * exp(-0.001f * m_FovStep);
    double tanFov = tan(0.5f * fov);

    double shiftX = 0.02f * m_ShakeStrength * distribution(randomEngine);
    double shiftY = 0.02f * m_ShakeStrength * distribution(randomEngine);

    if (m_ZLess)
    {
        m_Camera.Depth1 = 0.0f;
        m_Camera.Depth2 = 1.0f;
    }
    else
    {
        m_Camera.Depth1 = 1.0f;
        m_Camera.Depth2 = 0.0f;
    }

    m_Camera.xyz1 = XMFLOAT3((shiftX - tanFov) * aspectRatio, shiftY - tanFov, 0.1f);
    m_Camera.xyz2 = XMFLOAT3((shiftX + tanFov) * aspectRatio, shiftY + tanFov, 1000.0f);
    m_ShakeStrength *= exp(-dt);
}

void Game::OnRender()
{
    CommandQueue        &commandQueue = Application::Get()->GetCommandQueueDirect();
    PGraphicsCommandList commandList  = commandQueue.ResetCommandList();

    UINT      currentBackBufferIndex = Application::Get()->GetCurrentBackBufferIndex();
    PResource backBuffer             = Application::Get()->GetCurrentBackBuffer();
    auto      outRtv                 = Application::Get()->CurrentRTV();
    auto      rtv                    = Application::Get()->IntermediateRTV();
    auto      dsv                    = m_DSVHeap->GetFirstCpuHandle();

    ID3D12DescriptorHeap *const heapsToSet[] = {m_TextureHeap->Heap()};
    commandList->SetDescriptorHeaps(1, heapsToSet);

    TransitionResource(
        commandList, m_ColorBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    FLOAT clearColor[] = {1.0f, 0.75f, 0.5f, 1.0f};
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearRenderTargetView(outRtv, clearColor, 0, nullptr);

    CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &m_ScissorRect);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (m_ZLess)
    {
        commandList->SetPipelineState(m_PipelineStateCubeLess.Get());
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }
    else
    {
        commandList->SetPipelineState(m_PipelineStateCubeGreater.Get());
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
    }

    commandList->SetGraphicsRootSignature(m_RootSignatureCube.Get());

    XMMATRIX cameraMatrix = m_Camera.CalcMatrix();
    XMMATRIX mvpMatrix    = m_ModelMatrix * cameraMatrix;
    // m_CubeMesh.Draw(commandList);

    commandList->SetPipelineState((m_ZLess ? m_PipelineStateSponzaLess : m_PipelineStateSponzaGreater).Get());
    commandList->SetGraphicsRootSignature(m_RootSignatureSponza.Get());
    commandList->SetGraphicsRoot32BitConstants(0, 16, &cameraMatrix, 0);
    m_SponzaScene.Draw(commandList, cameraMatrix);

    TransitionResource(
        commandList, m_ColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);

    commandList->OMSetRenderTargets(1, &outRtv, FALSE, nullptr);
    commandList->SetPipelineState(m_PipelineStateFilter.Get());
    commandList->SetGraphicsRootSignature(m_RootSignatureFilter.Get());

    commandList->SetGraphicsRootDescriptorTable(1, m_TextureHeap->GetFirstGpuHandle());

    XMINT2 screenSize(m_Width, m_Height);
    commandList->SetGraphicsRoot32BitConstants(0, 2, &screenSize, 0);

    m_ScreenMesh.Draw(commandList);

    TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    UINT64 fenceValue = commandQueue.ExecuteCommandList(commandList);
    Application::Get()->Present();
    commandQueue.WaitForFenceValue(fenceValue);
}
