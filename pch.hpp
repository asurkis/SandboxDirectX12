#pragma once

#include <algorithm>
#include <chrono>
#include <deque>
#include <ios>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include <DirectXMath.h>
#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using std::size_t;
using std::uint64_t;

using Microsoft::WRL::ComPtr;

using PBlob                = ComPtr<ID3DBlob>;
using PCommandAllocator    = ComPtr<ID3D12CommandAllocator>;
using PCommandQueue        = ComPtr<ID3D12CommandQueue>;
using PDescriptorHeap      = ComPtr<ID3D12DescriptorHeap>;
using PDevice              = ComPtr<ID3D12Device2>;
using PFence               = ComPtr<ID3D12Fence>;
using PGraphicsCommandList = ComPtr<ID3D12GraphicsCommandList>;
using PPipelineState       = ComPtr<ID3D12PipelineState>;
using PResource            = ComPtr<ID3D12Resource>;
using PRootSignature       = ComPtr<ID3D12RootSignature>;
using PSwapChain           = ComPtr<IDXGISwapChain4>;
