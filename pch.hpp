#pragma once

#include <algorithm>
#include <chrono>
#include <codecvt>
#include <deque>
#include <filesystem>
#include <functional>
#include <ios>
#include <iostream>
#include <locale>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include <Windows.h>
#include <wrl.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>

#include <DirectXMath.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/WICTextureLoader.h>

using std::size_t;
using std::uint64_t;

using Microsoft::WRL::ComPtr;

using PBlob                = ComPtr<ID3DBlob>;
using PCommandAllocator    = ComPtr<ID3D12CommandAllocator>;
using PCommandQueue        = ComPtr<ID3D12CommandQueue>;
using PDevice              = ComPtr<ID3D12Device2>;
using PFactory             = ComPtr<IDXGIFactory4>;
using PFence               = ComPtr<ID3D12Fence>;
using PGraphicsCommandList = ComPtr<ID3D12GraphicsCommandList>;
using PPipelineState       = ComPtr<ID3D12PipelineState>;
using PResource            = ComPtr<ID3D12Resource>;
using PRootSignature       = ComPtr<ID3D12RootSignature>;
using PSwapChain           = ComPtr<IDXGISwapChain4>;

using DirectX::DescriptorHeap;
using DirectX::ResourceUploadBatch;
