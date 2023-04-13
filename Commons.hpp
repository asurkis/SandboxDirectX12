#pragma once

#include <DirectXMath.h>
#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <stdexcept>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

using PDevice              = ComPtr<ID3D12Device2>;
using PCommandQueue        = ComPtr<ID3D12CommandQueue>;
using PSwapChain           = ComPtr<IDXGISwapChain4>;
using PGraphicsCommandList = ComPtr<ID3D12GraphicsCommandList>;
using PCommandAllocator    = ComPtr<ID3D12CommandAllocator>;
using PDescriptorHeap      = ComPtr<ID3D12DescriptorHeap>;
using PFence               = ComPtr<ID3D12Fence>;
using PResource            = ComPtr<ID3D12Resource>;
