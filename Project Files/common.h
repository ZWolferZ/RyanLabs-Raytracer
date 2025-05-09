#pragma once

#pragma region Includes
//Include{s}
#include "DXSample.h"
#include "DXRApp.h"
using namespace DirectX;
#pragma endregion

#pragma region Data Structures
/// <summary>
/// Represents a vertex with position, normal, and texture coordinates.
/// </summary>
struct Vertex { // IMPORTANT - the hlsl version of this is STriVertex
	XMFLOAT3 position;
	XMFLOAT4 normal;
	XMFLOAT2 tex;
};

/// <summary>
/// Holds resources for building and storing acceleration structures.
/// </summary>
struct AccelerationStructureBuffers {
	ComPtr<ID3D12Resource> pScratch;      // Scratch memory for AS builder
	ComPtr<ID3D12Resource> pResult;       // Where the AS is
	ComPtr<ID3D12Resource> pInstanceDesc; // Hold the matrices of the instances
};

/// <summary>
/// Represents a texture resource and its associated metadata.
/// </summary>
struct Texture
{
	ComPtr<ID3D12Resource> textureResource;
	D3D12_RESOURCE_DESC textureDesc;
	ComPtr<ID3D12Resource> textureUploadHeap;
	wstring textureFile;
	int heapTextureNumber = -1;
};

/// <summary>
/// Stores camera-related matrices and parameters for rendering.
/// </summary>
struct CameraBuffer
{
	XMMATRIX invView;
	XMMATRIX invProj;
	float rX;
	float rY;
	XMFLOAT2 padding;
	float transBackgroundMode;
	XMFLOAT3 morePadding;
};

/// <summary>
/// Defines the properties of a light source, including position, color, and shadow settings.
/// </summary>
struct LightParams
{
	XMFLOAT4 lightPosition;
	XMFLOAT4 lightAmbientColor;
	XMFLOAT4 lightDiffuseColor;
	XMFLOAT4 lightSpecularColor;
	float lightSpecularPower;
	float pointLightRange;
	UINT shadows;
	UINT shawdowRayCount;
	XMFLOAT4 padding;
};

/// <summary>
/// Contains material properties such as reflection, shininess, and color.
/// </summary>
struct MaterialBuffer
{
	UINT reflection = 0;
	float shininess = 0.2f;
	int maxRecursionDepth = 20;
	UINT triOutline = 0;
	float triThickness = 0.01f;
	XMFLOAT3 triColour = { 0,0,0 };
	XMFLOAT4 objectColour = { 1,1,1,1 };
	float roughness = 0.0f;
	UINT texture = 0;
	XMFLOAT2 padding = { 0,0 };
};

/// <summary>
/// Stores the types of texture sampling methods.
/// </summary>
enum SamplerType
{
	ANISOTROPIC,
	LINEAR,
	POINTY,
};
#pragma endregion