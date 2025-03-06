#include "stdafx.h"
#include "DrawableGameObject.h"

using namespace std;

//#define NUM_VERTICES 36

DrawableGameObject::DrawableGameObject()
{
	//m_pVertexBuffer = nullptr;
	//m_pIndexBuffer = nullptr;
	//m_pTextureResourceView = nullptr;
	//m_pSamplerLinear = nullptr;
	m_position = { 0, 0, 0 };
	m_rotation = { 45, 45, 0 };
	m_scale = { 0.25f, 0.25f,0.25f };

	XMMATRIX rotation = XMMatrixRotationX(XMConvertToRadians(m_rotation.x)) * XMMatrixRotationY(XMConvertToRadians(m_rotation.y)) * XMMatrixRotationZ(m_rotation.z);
	XMMATRIX translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	// Initialize the world matrix
	XMStoreFloat4x4(&m_World, scale * rotation * translation);
	
}

DrawableGameObject::~DrawableGameObject()
{
	cleanup();
}

void DrawableGameObject::cleanup()
{
	// TODO
	//if (m_pVertexBuffer)
	//	m_pVertexBuffer->Release();
	//m_pVertexBuffer = nullptr;

	//if (m_pIndexBuffer)
	//	m_pIndexBuffer->Release();
	//m_pIndexBuffer = nullptr;

	//if (m_pTextureResourceView)
	//	m_pTextureResourceView->Release();
	//m_pTextureResourceView = nullptr;

	//if (m_pSamplerLinear)
	//	m_pSamplerLinear->Release();
	//m_pSamplerLinear = nullptr;

	//if (m_pMaterialConstantBuffer)
	//	m_pMaterialConstantBuffer->Release();
	//m_pMaterialConstantBuffer = nullptr;
}

HRESULT DrawableGameObject::initMesh(ComPtr<ID3D12Device5> device)
{
	// Create the vertex buffer.
	{
		//// Define the geometry for a triangle.
		//Vertex triangleVertices[] = {
		//	{{0.0f, 0.25f * 1.0f, 0.0f}, {1.0f,0.0f,0.0f,1.0f}},
		//	{{0.25f, -0.25f * 1.0f, 0.0f},{0.0f,1.0f,0.0f,1.0f}},
		//	{{-0.25f, -0.25f * 1.0f, 0.0f},{0.0f,0.0f,1.0f,1.0f}} };

		Vertex cubeVertices[] = {
			{{1.0f, 1.0f, 1.0f,},{1.0f,0.0f,0.0f,1.0f}},
			{{-1.0f, 1.0f, 1.0f,},{0.0f,1.0f,0.0f,1.0f}},
			{{-1.0f, -1.0f, 1.0f,},{0.0f,0.0f,1.0f,1.0f}},
			{{1.0f, -1.0f, 1.0f,},{1.0f,0.0f,0.0f,1.0f}},
			{{1.0f, -1.0f, -1.0f,},{0.0f,1.0f,0.0f,1.0f}},
			{{1.0f, 1.0f, -1.0f,},{0.0f,0.0f,1.0f,1.0f}},
			{{-1.0f, 1.0f, -1.0f,},{1.0f,0.0f,0.0f,1.0f}},
			{{-1.0f, -1.0f, -1.0f,},{0.0f,1.0f,0.0f,1.0f}}

		};

		m_vertexCount = 8;

		const UINT vertexBufferSize = sizeof(cubeVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not
		// recommended. Every time the GPU needs it, the upload heap will be
		// marshalled over. Please read up on Default Heap usage. An upload heap is
		// used here for code simplicity and because there are very few verts to
		// actually transfer.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(
			0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, cubeVertices, sizeof(cubeVertices));
		m_vertexBuffer->Unmap(0, nullptr);
	}

	// create the index buffer
	{
		// indices.
		UINT indices[] =
		{
			0, 1, 2, 2, 3, 0, // FRONT FACE
			0, 3, 4, 4, 5, 0, // RIGHT FACE
			0, 5, 6, 6, 1, 0, // TOP FACE
			1, 6, 7, 7, 2, 1, // LEFT FACE
			7, 4, 3, 3, 2, 7, // BOTTOM FACE
			4, 7, 6, 6, 5, 4 // BACK FACE
		};

		m_indexCount = sizeof(indices) / sizeof(UINT);

		const UINT indexBufferSize = sizeof(indices);
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer)));

		// Copy the triangle data to the index buffer.
		CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.

		UINT8* pIndexDataBegin;
		ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices, indexBufferSize);
		m_indexBuffer->Unmap(0, nullptr);
	}

	return S_OK;
}

DrawableGameObject* DrawableGameObject::createCopy()
{
	DrawableGameObject* pobj = new DrawableGameObject();
	*pobj = *this;
	return pobj;
}

void DrawableGameObject::setPosition(XMFLOAT3 position)
{
	m_position = position;
}

void DrawableGameObject::update(float t)
{
	static float cummulativeTime = 0;
	cummulativeTime += t;

	if (m_rotation.x > 360.0f) m_rotation.x = 0.0f;

	if (m_rotation.y > 360.0f) m_rotation.y = 0.0f;

	if (m_rotation.z > 360.0f) m_rotation.z = 0.0f;

	XMMATRIX rotation = XMMatrixRotationX(XMConvertToRadians(m_rotation.x)) * XMMatrixRotationY(XMConvertToRadians(m_rotation.y)) * XMMatrixRotationZ(XMConvertToRadians(m_rotation.z + t));
	XMMATRIX translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	XMStoreFloat4x4(&m_World, scale * rotation * translation);
}