#pragma once

#pragma region Includes
//Include{s}
#include "DXRApp.h"
#include "common.h"
#pragma endregion

/// <summary>
/// The DXRSetup class. This class handles initializing and setting up the DXR pipeline.
/// </summary>
class DXRSetup
{
	friend class DXRRuntime; // HAha freind

private:
#pragma region Private Variables
	static const UINT FrameCount = FRAME_COUNT;
	DXRApp* m_app;
	ComPtr<ID3D12Device5> m_device;

	float m_fovAngleY = 45.0f * XM_PI / 180.0f;
	bool m_transBackgroundMode = false;

	XMFLOAT4 m_originalLightPosition = { 0.0f, 2.0f, 0.0f, 0.0f };
	XMFLOAT4 m_originalLightAmbientColor = { 0.9f, 0.9f, 0.9f, 1.0f };
	XMFLOAT4 m_originalLightDiffuseColor = { 0.6f, 0.6f, 0.6f, 1.0f };
	XMFLOAT4 m_originalLightSpecularColor = { 0.6f, 0.6f, 0.6f, 1.0f };
	float m_originalLightSpecularPower = 32.0f;
	float m_originalPointLightRange = 15.0f;
	bool m_originalShadows = true;
	bool m_shadows = m_originalShadows;
	UINT m_originalShadowRayCount = 100;

	// Add Paths to static textures here.
	wstring m_staticTextures[3] = { L"Textures/staticTexture1.png", L"Textures/staticTexture2.png", L"Textures/staticTexture3.png" };
	vector<Texture> m_staticTexturesVector;

	UINT16 m_textureNumber = 0;
	vector<std::pair<wstring, int>> m_textures;

	SamplerType m_samplerType = POINTY;
#pragma endregion

#pragma region Init Methods
	/// <summary>
	/// Sets up the ImGui interface.
	/// </summary>
	void SetupIMGUI();

	/// <summary>
	/// Checks if the current hardware supports raytracing.
	/// </summary>
	void CheckRaytracingSupport();

	/// <summary>
	/// Loads the rendering pipeline.
	/// </summary>
	void LoadPipeline();

public:
	/// <summary>
	/// Initializes the raytracing setup.
	/// </summary>
	void initialise();
private:

#pragma endregion

#pragma region Object / Asset Methods

	/// <summary>
	/// Loads assets required for the objects.
	/// </summary>
	void LoadAssets();

	/// <summary>
	/// Loads texture resources.
	/// </summary>
	void LoadTextures();

	/// <summary>
	/// Creates all acceleration structures (bottom and top).
	/// </summary>
	void CreateAccelerationStructures();

	/// <summary>
	/// Creates the top-level acceleration structure that holds all instances of the scene.
	/// </summary>
	/// <param name="instances">A vector of pairs containing BLAS and transform matrices.</param>
	/// <param name="update">Indicates whether to update the TLAS.</param>
	void CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool update);

	/// <summary>
	/// Creates the bottom-level acceleration structure for an instance.
	/// </summary>
	/// <param name="vVertexBuffers">A vector of pairs containing vertex buffers.</param>
	/// <param name="vIndexBuffers">A vector of pairs containing index buffers.</param>
	/// <returns>AccelerationStructureBuffers for the BLAS.</returns>
	AccelerationStructureBuffers CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
		std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers);
#pragma endregion

#pragma region Shader Signature Methods
	/// <summary>
	/// Creates the root signature for the ray generation shader.
	/// </summary>
	/// <returns>A ComPtr to the root signature.</returns>
	ComPtr<ID3D12RootSignature> CreateRayGenSignature();

	/// <summary>
	/// Creates the root signature for the miss shader.
	/// </summary>
	/// <returns>A ComPtr to the root signature.</returns>
	ComPtr<ID3D12RootSignature> CreateMissSignature();

	/// <summary>
	/// Creates the root signature for the hit shader.
	/// </summary>
	/// <returns>A ComPtr to the root signature.</returns>
	ComPtr<ID3D12RootSignature> CreateHitSignature();

#pragma endregion

#pragma region Sampler Methods
	/// <summary>
	/// Creates a static sampler descriptor for point sampling.
	/// </summary>
	/// <returns>A point sampler.</returns>
	D3D12_STATIC_SAMPLER_DESC CreateStaticSamplerPointDesc();

	/// <summary>
	/// Creates a static sampler descriptor for linear sampling.
	/// </summary>
	/// <returns>A linear sampler.</returns>
	D3D12_STATIC_SAMPLER_DESC CreateStaticSamplerLinearDesc();

	/// <summary>
	/// Creates a static sampler descriptor for anisotropic sampling.
	/// </summary>
	/// <returns>A anisotropic sampler.</returns>
	D3D12_STATIC_SAMPLER_DESC CreateStaticSamplerAnisotropicDesc();

#pragma endregion

#pragma region Raytracing Methods
	/// <summary>
	/// Creates the raytracing pipeline state object.
	/// </summary>
	void CreateRaytracingPipeline();

	/// <summary>
	/// Updates the raytracing pipeline state object.
	/// </summary>
	void UpdateRaytracingPipeline();

	/// <summary>
	/// Creates the output buffer for raytracing results.
	/// </summary>
	void CreateRaytracingOutputBuffer();

	/// <summary>
	/// Creates the shader resource heap.
	/// </summary>
	void CreateShaderResourceHeap();

	/// <summary>
	/// Creates the shader binding table.
	/// </summary>
	void CreateShaderBindingTable();

	/// <summary>
	/// Updates the shader binding table.
	/// </summary>
	void UpdateShaderBindingTable();
#pragma endregion

public:
#pragma region Constructors and Destructors
	/// <summary>
	/// Initializes a new instance of the DXRSetup class.
	/// </summary>
	/// <param name="app">Pointer to the DXR application instance.</param>
	DXRSetup(DXRApp* app);
#pragma endregion

#pragma region  CBuffer Methods

	/// <summary>
	/// Creates the camera buffer for the scene.
	/// </summary>
	void CreateCamera();

	/// <summary>
	/// Updates the camera's position,rotation and view matrix.
	/// </summary>
	/// <param name="rX">The amount of rays on the X axis, 1 is every pixel and 10 is ten times less.</param>
	/// <param name="rY">The amount of rays on the Y axis, 1 is every pixel and 10 is ten times less</param>
	void UpdateCamera(float rX, float rY);

	/// <summary>
	/// Creates the lighting buffer.
	/// </summary>
	void CreateLightingBuffer();

	/// <summary>
	/// Updates the lighting buffer with new light data.
	/// </summary>
	/// <param name="lightPosition">The position of the light.</param>
	/// <param name="lightAmbientColor">The ambient color of the light.</param>
	/// <param name="lightDiffuseColor">The diffuse color of the light.</param>
	/// <param name="lightSpecularColor">The specular color of the light.</param>
	/// <param name="lightSpecularPower">The specular power of the light.</param>
	/// <param name="pointLightRange">The range/power of the point light.</param>
	/// <param name="shadowRayCount">The number of shadow rays that should be fired.</param>
	void UpdateLightingBuffer(XMFLOAT4 lightPosition, XMFLOAT4 lightAmbientColor, XMFLOAT4 lightDiffuseColor, XMFLOAT4 lightSpecularColor, float lightSpecularPower, float pointLightRange, UINT shadowRayCount);

	/// <summary>
	/// Creates all the object material buffers for the scene.
	/// </summary>
	void CreateMaterialBuffers();

	/// <summary>
	/// Updates the material buffers with new material data.
	/// </summary>
	void UpdateMaterialBuffers();
#pragma endregion
};