#pragma once

#pragma region Includes
//Include{s}
#include "Camera.h"
#include "DXRApp.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"
#include "common.h"
#pragma endregion

class DXRContext
{
public:

	DXRContext(UINT width, UINT height);

	Camera* m_pCamera;
	ComPtr< ID3D12Resource > m_cameraBuffer;
	uint32_t m_cameraBufferSize = 256;

	// Lighting buffer
	ComPtr< ID3D12Resource > m_lightingBuffer;
	uint32_t m_lightingBufferSize = 256;

	ComPtr<ID3D12DescriptorHeap> m_IMGUIDescHeap;

	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device5> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[FRAME_COUNT];
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12GraphicsCommandList4> m_commandList;
	UINT m_rtvDescriptorSize;

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;

	ComPtr<ID3D12Resource> m_bottomLevelAS; // Storage for the bottom Level AS

	nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
	AccelerationStructureBuffers m_topLevelASBuffers;

	ComPtr<IDxcBlob> m_rayGenLibrary; // ray gen shader
	ComPtr<IDxcBlob> m_hitLibrary; // hit shader
	ComPtr<IDxcBlob> m_missLibrary; // miss shader

	ComPtr<ID3D12RootSignature> m_rayGenSignature; // ray gen signature (a link to to the registers in the shader)
	ComPtr<ID3D12RootSignature> m_hitSignature; // hit signature (a link to to the registers in the shader)
	ComPtr<ID3D12RootSignature> m_missSignature; // miss signature (a link to to the registers in the shader)

	// Ray tracing pipeline state
	ComPtr<ID3D12StateObject> m_rtStateObject;

	// Ray tracing pipeline state properties, retaining the shader identifiers
	// to use in the Shader Binding Table
	ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

	ComPtr<ID3D12Resource> m_outputResource; // where the colours are written (before being copied to a render target)
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap; // the main heap used by the shaders, which will give access to the raytracing output and the top-level acceleration structure

	nv_helpers_dx12::ShaderBindingTableGenerator m_sbtHelper;
	ComPtr<ID3D12Resource> m_sbtStorage;
};
