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
	void UpdateCamera(float rX, float rY);
	void CreateLightingBuffer();
	void UpdateLightingBuffer(XMFLOAT4 lightPosition, XMFLOAT4 lightAmbientColor, XMFLOAT4 lightDiffuseColor, XMFLOAT4 lightSpecularColor, float lightSpecularPower, float pointLightRange, UINT shadowRayCount);
	void CreateMaterialBuffers();
	void UpdateMaterialBuffers();

	float m_fovAngleY = 45.0f * XM_PI / 180.0f;

	bool m_transBackgroundMode = false;

	XMFLOAT4 m_originalLightPosition = { 0.0f, 2.0f, 0.0f, 0.0f };
	XMFLOAT4 m_originalLightAmbientColor = { 0.9f, 0.9f, 0.9f, 1.0f };
	XMFLOAT4 m_originalLightDiffuseColor = { 0.6f, 0.6f, 0.6f, 1.0f };
	XMFLOAT4 m_originalLightSpecularColor = { 0.6f, 0.6f, 0.6f, 1.0f };
	float m_originalLightSpecularPower = 32.0f;
	float m_originalPointLightRange = 6.0f;
	bool m_originalShadows = true;
	bool m_shadows = m_originalShadows;
	UINT m_originalShadowRayCount = 100;

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
