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
#pragma region Constructors
	/// <summary>
	/// Contains the main DXR context. This class is used to manage the DXR context and its resources.
	/// </summary>
	/// <param name="width">The width of the window.</param>
	/// <param name="height">The height of the window.</param>
	DXRContext(UINT width, UINT height);
#pragma endregion

#pragma region Camera
	// Camera buffer
	Camera* m_pCamera;
	ComPtr< ID3D12Resource > m_cameraBuffer;
	uint32_t m_cameraBufferSize = 256;
#pragma endregion

#pragma region Lighting
	// Lighting buffer
	ComPtr< ID3D12Resource > m_lightingBuffer;
	uint32_t m_lightingBufferSize = 256;
#pragma endregion

#pragma region ImGui
	ComPtr<ID3D12DescriptorHeap> m_IMGUIDescHeap;
#pragma endregion

#pragma region Pipeline Objects
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
#pragma endregion

#pragma region Synchronization
	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;
#pragma endregion

#pragma region Acceleration Structures
	ComPtr<ID3D12Resource> m_bottomLevelAS; // Storage for the bottom Level AS

	nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
	AccelerationStructureBuffers m_topLevelASBuffers;
#pragma endregion

#pragma region Shader Libraries
	ComPtr<IDxcBlob> m_rayGenLibrary; // ray gen shader
	ComPtr<IDxcBlob> m_hitLibrary; // hit shader
	ComPtr<IDxcBlob> m_missLibrary; // miss shader
#pragma endregion

#pragma region Root Signatures
	ComPtr<ID3D12RootSignature> m_rayGenSignature; // ray gen signature (a link to to the registers in the shader)
	ComPtr<ID3D12RootSignature> m_hitSignature; // hit signature (a link to to the registers in the shader)
	ComPtr<ID3D12RootSignature> m_missSignature; // miss signature (a link to to the registers in the shader)
#pragma endregion

#pragma region Ray Tracing Pipeline
	// Ray tracing pipeline state
	ComPtr<ID3D12StateObject> m_rtStateObject;

	// Ray tracing pipeline state properties, retaining the shader identifiers
	// to use in the Shader Binding Table
	ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;
#pragma endregion

#pragma region Output Resources
	ComPtr<ID3D12Resource> m_outputResource; // where the colours are written (before being copied to a render target)
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap; // the main heap used by the shaders, which will give access to the raytracing output and the top-level acceleration structure
#pragma endregion

#pragma region Shader Binding Table
	nv_helpers_dx12::ShaderBindingTableGenerator m_sbtHelper;
	ComPtr<ID3D12Resource> m_sbtStorage;
#pragma endregion
};