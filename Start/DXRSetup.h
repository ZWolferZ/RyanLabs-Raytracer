#pragma once

#include "DXRApp.h"
#include "common.h"

class DXRSetup
{
	friend class DXRRuntime;
private:
	static const UINT FrameCount = FRAME_COUNT;
	DXRApp* m_app;
	ComPtr<ID3D12Device5> m_device;

public:
	DXRSetup(DXRApp* app);

	void initialise();

	void CreateCamera();
	void UpdateCamera();
	void CreateColourBuffer();
	void UpdateColourBuffer(XMFLOAT4 objectColour, XMFLOAT4 planeColour);

	XMFLOAT4 m_objectColour;
	XMFLOAT4 m_planeColour;

	XMFLOAT4 m_originalObjectColour = { 1.0f, 0.75f, 0.8f, 1.0f };
	XMFLOAT4 m_originalPlaneColour = { 0.68f, 0.85f, 0.90f, 1.0f };

private:

	void SetupIMGUI();

	void CheckRaytracingSupport();

	void LoadPipeline();
	void LoadAssets();

	/// Create all acceleration structures, bottom and top
	void CreateAccelerationStructures();

	/// Create the main acceleration structure that holds all instances of the scene
	/// \param     instances : pair of BLAS and transform
	void CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool update);

	/// Create the acceleration structure of an instance
	///
	/// \param     vVertexBuffers : pair of buffer and vertex count
	/// \return    AccelerationStructureBuffers for TLAS
	AccelerationStructureBuffers CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
		std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers);

	// #DXR
	ComPtr<ID3D12RootSignature> CreateRayGenSignature();
	ComPtr<ID3D12RootSignature> CreateMissSignature();
	ComPtr<ID3D12RootSignature> CreateHitSignature();

	void CreateRaytracingPipeline();

	void CreateRaytracingOutputBuffer();
	void CreateShaderResourceHeap();

	void CreateShaderBindingTable();
};
