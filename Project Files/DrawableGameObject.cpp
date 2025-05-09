#include "stdafx.h"

#pragma region Includes
//Include{s}
#include "DrawableGameObject.h"
using namespace std;
#pragma endregion

#pragma region Constructors and Destructors
DrawableGameObject::DrawableGameObject(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale, string objectName)
{
	m_position = position;
	m_rotation = rotation;
	m_scale = scale;

	this->setObjectName(objectName); // This is used for UI and debugging purposes

	objectName += " Hit Group";

	m_objectHitGroupName.assign(objectName.begin(), objectName.end()); // This is used for the hit group name per object.

	XMMATRIX newRotation = XMMatrixRotationX(XMConvertToRadians(m_rotation.x)) * XMMatrixRotationY(XMConvertToRadians(m_rotation.y)) * XMMatrixRotationZ(m_rotation.z);
	XMMATRIX newTranslation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX newScale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	// Initialize the world matrix
	XMStoreFloat4x4(&m_World, newTranslation * newRotation * newScale);

	this->setOrginalTransformValues(this->getPosition(), this->getRotation(), this->getScale()); // Record the original transform values

	this->update(0.0f);
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
#pragma endregion

#pragma region Init Methods

// If I had a nickel for every time I had to write cube data for a graphics module, I would have three nickels.

HRESULT DrawableGameObject::initCubeMesh(ComPtr<ID3D12Device5> device)
{
	// Create the vertex buffer.
	{
		Vertex cubeVertices[] = {
			// Front face
			{{1.0f,1.0f,1.0f},{0.0f,0.0f,1.0f,1.0f},{1.0f,1.0f}},
			{{-1.0f,1.0f,1.0f},{0.0f,0.0f,1.0f,1.0f},{0.0f,1.0f}},
			{{-1.0f,-1.0f,1.0f},{0.0f,0.0f,1.0f,1.0f},{0.0f,0.0f}},
			{{1.0f,-1.0f,1.0f},{0.0f,0.0f,1.0f,1.0f},{1.0f,0.0f}},

			// Right face
			{{1.0f,1.0f,1.0f},{1.0f,0.0f,0.0f,1.0f},{1.0f,1.0f}},
			{{1.0f,-1.0f,1.0f},{1.0f,0.0f,0.0f,1.0f},{0.0f,1.0f}},
			{{1.0f,-1.0f,-1.0f},{1.0f,0.0f,0.0f,1.0f},{0.0f,0.0f}},
			{{1.0f,1.0f,-1.0f},{1.0f,0.0f,0.0f,1.0f},{1.0f,0.0f}},

			// Back face
			{{1.0f,1.0f,-1.0f},{0.0f,0.0f,-1.0f,1.0f},{1.0f,1.0f}},
			{{-1.0f,1.0f,-1.0f},{0.0f,0.0f,-1.0f,1.0f},{0.0f,1.0f}},
			{{-1.0f,-1.0f,-1.0f},{0.0f,0.0f,-1.0f,1.0f},{0.0f,0.0f}},
			{{1.0f,-1.0f,-1.0f},{0.0f,0.0f,-1.0f,1.0f},{1.0f,0.0f}},

			// Left face
			{{-1.0f,1.0f,1.0f},{-1.0f,0.0f,0.0f,1.0f},{1.0f,1.0f}},
			{{-1.0f,-1.0f,1.0f},{-1.0f,0.0f,0.0f,1.0f},{0.0f,1.0f}},
			{{-1.0f,-1.0f,-1.0f},{-1.0f,0.0f,0.0f,1.0f},{0.0f,0.0f}},
			{{-1.0f,1.0f,-1.0f},{-1.0f,0.0f,0.0f,1.0f},{1.0f,0.0f}},

			// Top face
			{{1.0f,1.0f,1.0f},{0.0f,1.0f,0.0f,1.0f},{1.0f,1.0f}},
			{{-1.0f,1.0f,1.0f},{0.0f,1.0f,0.0f,1.0f},{0.0f,1.0f}},
			{{-1.0f,1.0f,-1.0f},{0.0f,1.0f,0.0f,1.0f},{0.0f,0.0f}},
			{{1.0f,1.0f,-1.0f},{0.0f,1.0f,0.0f,1.0f},{1.0f,0.0f}},

			// Bottom face
			{{1.0f,-1.0f,1.0f},{0.0f,-1.0f,0.0f,1.0f},{1.0f,1.0f}},
			{{-1.0f,-1.0f,1.0f},{0.0f,-1.0f,0.0f,1.0f},{0.0f,1.0f}},
			{{-1.0f,-1.0f,-1.0f},{0.0f,-1.0f,0.0f,1.0f},{0.0f,0.0f}},
			{{1.0f,-1.0f,-1.0f},{0.0f,-1.0f,0.0f,1.0f},{1.0f,0.0f}}
		};

		m_meshData.VertexCount = 24;

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
			IID_PPV_ARGS(&m_meshData.VertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(
			0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_meshData.VertexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, cubeVertices, sizeof(cubeVertices));
		m_meshData.VertexBuffer->Unmap(0, nullptr);
	}

	// create the index buffer
	{
		// indices.
		UINT indices[] = {
			// Front face
			0,1,2,2,3,0,

			// Right face
			4,5,6,6,7,4,

			// Top face
			8,9,10,10,11,8,

			// Left face
			12,13,14,14,15,12,

			// Bottom face
			16,17,18,18,19,16,

			// Back face
			20,21,22,22,23,20
		};

		m_meshData.IndexCount = sizeof(indices) / sizeof(UINT);

		const UINT indexBufferSize = sizeof(indices);
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_meshData.IndexBuffer)));

		// Copy the triangle data to the index buffer.
		CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.

		UINT8* pIndexDataBegin;
		ThrowIfFailed(m_meshData.IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices, indexBufferSize);
		m_meshData.IndexBuffer->Unmap(0, nullptr);
	}

	m_cubeMesh = true;
	return S_OK;
}

HRESULT DrawableGameObject::initPlaneMesh(ComPtr<ID3D12Device5> device)
{
	// Create the vertex buffer.
	{
		float diameter = 0.5f;
		Vertex planeVertices[] = {
					{ XMFLOAT3(-diameter,  diameter, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 0:
	{ XMFLOAT3(-diameter, -diameter, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) }, // 1:
	{ XMFLOAT3(diameter,  diameter, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) }, // 2:
	{ XMFLOAT3(diameter, -diameter, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) }  // 3:
		};

		m_meshData.VertexCount = 4;

		const UINT vertexBufferSize = sizeof(planeVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not
		// recommended. Every time the GPU needs it, the upload heap will be
		// marshalled over. Please read up on Default Heap usage. An upload heap is
		// used here for code simplicity and because there are very few verts to
		// actually transfer.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_meshData.VertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(
			0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_meshData.VertexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, planeVertices, sizeof(planeVertices));
		m_meshData.VertexBuffer->Unmap(0, nullptr);
	}

	// create the index buffer
	{
		UINT indices[] =
		{
			0,1,2,
			2,1,3,
		};

		m_meshData.IndexCount = sizeof(indices) / sizeof(UINT);

		const UINT indexBufferSize = sizeof(indices);
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_meshData.IndexBuffer)));

		// Copy the triangle data to the index buffer.
		CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.

		UINT8* pIndexDataBegin;
		ThrowIfFailed(m_meshData.IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices, indexBufferSize);
		m_meshData.IndexBuffer->Unmap(0, nullptr);
	}

	m_planeMesh = true;
	return S_OK;
}

HRESULT DrawableGameObject::initOBJMesh(ComPtr<ID3D12Device5> device, char* szOBJName)
{
	m_meshData = OBJLoader::Load(szOBJName, device.Get());
	assert(m_meshData.VertexBuffer);
	m_objMesh = true;
	return S_OK;
}

DrawableGameObject* DrawableGameObject::createCopy()
{
	DrawableGameObject* pobj = new DrawableGameObject(this->getPosition(), this->getRotation(), this->getScale(), this->getObjectName());
	*pobj = *this;
	return pobj;
}
#pragma endregion

#pragma region Getters and Setters
void DrawableGameObject::setPosition(XMFLOAT3 position)
{
	m_position = position;
}

void DrawableGameObject::setRotation(XMFLOAT3 rotation)
{
	m_rotation = rotation;
}

void DrawableGameObject::setScale(XMFLOAT3 scale)
{
	m_scale = scale;
}

void DrawableGameObject::setOrginalTransformValues(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale)
{
	m_orginalPosition = position;
	m_orginalRotation = rotation;
	m_orginalScale = scale;
}

#pragma endregion

#pragma region Update Methods
void DrawableGameObject::resetTransform()
{
	m_position = m_orginalPosition;
	m_rotation = m_orginalRotation;
	m_scale = m_orginalScale;

	m_autoRotateX = false;
	m_autoRotateY = false;
	m_autoRotateZ = false;
}

void DrawableGameObject::update(float t)
{
	static float cummulativeTime = 0;
	cummulativeTime += t;

	// Some basic checks to make sure the object isn't going to explode.
	if (m_reflection)
	{
		m_materialBufferData.reflection = 1;
	}
	else
	{
		m_materialBufferData.reflection = 0;
	}

	if (m_triOutline)
	{
		m_materialBufferData.triOutline = 1;
	}
	else
	{
		m_materialBufferData.triOutline = 0;
	}

	if (m_texture)
	{
		m_materialBufferData.texture = 1;
	}
	else
	{
		m_materialBufferData.texture = 0;
	}

	// Don't overflow the rotation
	if (m_rotation.x > 360.0f) m_rotation.x = 0.0f;

	if (m_rotation.x < -360.0f) m_rotation.x = 0.0f;

	if (m_rotation.y > 360.0f) m_rotation.y = 0.0f;

	if (m_rotation.y < -360.0f) m_rotation.y = 0.0f;

	if (m_rotation.z > 360.0f) m_rotation.z = 0.0f;

	if (m_rotation.z < -360.0f) m_rotation.z = 0.0f;

	if (m_autoRotateX)
	{
		m_rotation.x += m_autoRotationSpeed * t;
	}

	if (m_autoRotateY)
	{
		m_rotation.y += m_autoRotationSpeed * t;
	}

	if (m_autoRotateZ)
	{
		m_rotation.z += m_autoRotationSpeed * t;
	}

	XMMATRIX rotation = XMMatrixRotationX(XMConvertToRadians(m_rotation.x)) * XMMatrixRotationY(XMConvertToRadians(m_rotation.y)) * XMMatrixRotationZ(XMConvertToRadians(m_rotation.z));
	XMMATRIX translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	XMStoreFloat4x4(&m_World, scale * rotation * translation);
}
#pragma endregion