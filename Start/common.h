#pragma once

#include "DXSample.h"

#include "DXRApp.h"

using namespace DirectX;

struct Vertex { // IMPORTANT - the hlsl version of this is STriVertex
	XMFLOAT3 position;
	XMFLOAT4 normal;
	XMFLOAT2 tex;
};

struct AccelerationStructureBuffers {
	ComPtr<ID3D12Resource> pScratch;      // Scratch memory for AS builder
	ComPtr<ID3D12Resource> pResult;       // Where the AS is
	ComPtr<ID3D12Resource> pInstanceDesc; // Hold the matrices of the instances
};

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

struct ColourBuffer
{
	XMFLOAT4 objectColour;
	XMFLOAT4 planeColour;
};

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
	UINT reflection;
	float shininess;
	UINT maxRecursionDepth;
	float padding;
};
