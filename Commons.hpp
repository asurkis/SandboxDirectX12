#pragma once

#include <optional>
#include <stdexcept>

#include <DirectXMath.h>
#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

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
