#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <codecvt>
#include <deque>
#include <filesystem>
#include <fstream>
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
#include <unordered_set>

#include <Windows.h>
#include <wrl.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>

#include <DirectXMath.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/WICTextureLoader.h>

using std::size_t;
using std::uint64_t;

using Microsoft::WRL::ComPtr;

using PFactory   = ComPtr<IDXGIFactory4>;
using PDevice    = ComPtr<ID3D12Device2>;
using PSwapChain = ComPtr<IDXGISwapChain4>;
using PFence     = ComPtr<ID3D12Fence>;

using PCommandAllocator    = ComPtr<ID3D12CommandAllocator>;
using PCommandQueue        = ComPtr<ID3D12CommandQueue>;
using PGraphicsCommandList = ComPtr<ID3D12GraphicsCommandList>;

using PPipelineState = ComPtr<ID3D12PipelineState>;
using PResource      = ComPtr<ID3D12Resource>;
using PRootSignature = ComPtr<ID3D12RootSignature>;

using PBlob              = ComPtr<ID3DBlob>;
using PDxcCompiler       = ComPtr<IDxcCompiler3>;
using PDxcUtils          = ComPtr<IDxcUtils>;
using PDxcIncludeHandler = ComPtr<IDxcIncludeHandler>;

using DirectX::DescriptorHeap;
using DirectX::ResourceUploadBatch;
