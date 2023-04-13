#include "Game.hpp"
#include "Application.hpp"

using DirectX::XMFLOAT3;
using DirectX::XMMATRIX;

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
    0,
    1,
    2,
    2,
    1,
    3,

    4,
    6,
    5,
    5,
    6,
    7,
};

Game::Game(Application *application, int width, int height, bool vSync)
    : m_Application(application),
      m_ScissorRect{0, 0, LONG_MAX, LONG_MAX},
      m_Viewport{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)},
      m_FoV(45.0f)
{
    PDevice              device       = m_Application->GetDevice();
    CommandQueue        &commandQueue = m_Application->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
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

    ComPtr<ID3DBlob> vertexShaderBlob;
    try
    {
        Assert(D3DReadFileToBlob(L"Vertex.cso", &vertexShaderBlob));
    }
    catch (std::runtime_error const &e)
    {
        throw;
    }
    ComPtr<ID3DBlob> pixelShaderBlob;
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

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

    D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
                                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
                                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
                                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
    CD3DX12_ROOT_PARAMETER1 rootParameters[1] = {};
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / sizeof(DWORD), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC sigDesc = {};
    sigDesc.Init_1_1(1, rootParameters, 0, nullptr, flags);

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errorBlob;
    Assert(D3DX12SerializeVersionedRootSignature(&sigDesc, featureData.HighestVersion, &rootSigBlob, &errorBlob));
    Assert(device->CreateRootSignature(
        0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets      = 1;
    rtvFormats.RTFormats[0]          = DXGI_FORMAT_R8G8B8A8_UNORM;

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS                    VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS                    PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature        = m_RootSignature.Get();
    pipelineStateStream.InputLayout           = {inputLayout, _countof(inputLayout)};
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats            = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pssDesc = {};
    pssDesc.SizeInBytes                      = sizeof(pipelineStateStream);
    pssDesc.pPipelineStateSubobjectStream    = &pipelineStateStream;
    Assert(device->CreatePipelineState(&pssDesc, IID_PPV_ARGS(&m_PipelineState)));

    UINT64 fenceValue = commandQueue.ExecuteCommandList(commandList);
    commandQueue.WaitForFenceValue(fenceValue);
    m_ContentLoaded = true;
}

void Game::UpdateBufferResource(PGraphicsCommandList commandList,
                                ID3D12Resource     **destinationResource,
                                ID3D12Resource     **intermediateResource,
                                size_t               numElements,
                                size_t               elementSize,
                                const void          *bufferData,
                                D3D12_RESOURCE_FLAGS flags)
{
    PDevice device     = m_Application->GetDevice();
    size_t  bufferSize = numElements * elementSize;
    Assert(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                           D3D12_HEAP_FLAG_NONE,
                                           &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
                                           D3D12_RESOURCE_STATE_COPY_DEST,
                                           nullptr,
                                           IID_PPV_ARGS(destinationResource)));
    if (!bufferData) return;
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
