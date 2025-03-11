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