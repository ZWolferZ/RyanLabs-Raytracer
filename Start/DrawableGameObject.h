#pragma once

#pragma region Includes
//Include{s}
#include "common.h"
#include "OBJLoader.h"
using Microsoft::WRL::ComPtr;
#pragma endregion

/// <summary>
/// The GameObject class. This class is used to represent a drawable object in the game.
/// </summary>
class DrawableGameObject
{
public:
#pragma region Constructors and Destructors
	/// <summary>
	/// Initializes a new instance of the GameObject class.
	/// </summary>
	/// <param name="position">The initial position of the object.</param>
	/// <param name="rotation">The initial rotation of the object.</param>
	/// <param name="scale">The initial scale of the object.</param>
	/// <param name="objectName">The name of the object. (FOR UI)</param>
	DrawableGameObject(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale, string objectName);

	/// <summary>
	/// Destructor for the GameObject class.
	/// </summary>
	~DrawableGameObject();
#pragma endregion

#pragma region Public Methods
	/// <summary>
	/// Cleans up resources used by the object.
	/// </summary>
	void cleanup();

	/// <summary>
	/// Creates a copy of the current GameObject.
	/// </summary>
	/// <returns>A new GameObject instance.</returns>
	DrawableGameObject* createCopy();

	/// <summary>
	/// Initializes a cube mesh for the object.
	/// </summary>
	/// <param name="device">The Direct3D device.</param>
	/// <returns>HRESULT indicating success or failure.</returns>
	HRESULT initCubeMesh(ComPtr<ID3D12Device5> device);

	/// <summary>
	/// Initializes a plane mesh for the object.
	/// </summary>
	/// <param name="device">The Direct3D device.</param>
	/// <returns>HRESULT indicating success or failure.</returns>
	HRESULT initPlaneMesh(ComPtr<ID3D12Device5> device);

	/// <summary>
	/// Initializes an OBJ mesh for the object.
	/// </summary>
	/// <param name="device">The Direct3D device.</param>
	/// <param name="szOBJName">The name of the OBJ file.</param>
	/// <returns>HRESULT indicating success or failure.</returns>
	HRESULT initOBJMesh(ComPtr<ID3D12Device5> device, char* szOBJName);

	/// <summary>
	/// Updates the object based on the elapsed time.
	/// </summary>
	/// <param name="t">The elapsed time.</param>
	void update(float t);

	/// <summary>
	/// Resets the object's transform to its original values.
	/// </summary>
	void resetTransform();
#pragma endregion

#pragma region Getters and Setters
	/// <summary>
	/// Provides access to the object's properties, such as position, rotation, scale, and mesh data.
	/// </summary>

	ComPtr<ID3D12Resource> getVertexBuffer() { return m_meshData.VertexBuffer; }
	ComPtr<ID3D12Resource> getIndexBuffer() { return m_meshData.IndexBuffer; }
	XMMATRIX getTransform() { return XMLoadFloat4x4(&m_World); }
	void setPosition(XMFLOAT3 position);
	XMFLOAT3 getPosition() { return m_position; }
	void setRotation(XMFLOAT3 rotation);
	XMFLOAT3 getRotation() { return m_rotation; }
	void setScale(XMFLOAT3 scale);
	XMFLOAT3 getScale() { return m_scale; }
	unsigned int getVertexCount() { return m_meshData.VertexCount; }
	unsigned int getIndexCount() { return m_meshData.IndexCount; }
	string getObjectName() { return m_objectName; }
	void setObjectName(string name) { m_objectName = name; }
	void setOrginalTransformValues(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale);
#pragma endregion

#pragma region Public Variables
	string m_objectName;
	bool m_autoRotateX = false;
	bool m_autoRotateY = false;
	bool m_autoRotateZ = false;
	bool m_planeMesh = false;
	bool m_objMesh = false;
	bool m_cubeMesh = false;
	bool m_textMesh = false;
	wstring m_objectHitGroupName;
	float m_autoRotationSpeed = 50.0f;
	bool m_reflection = false;
	bool m_triOutline = true;
	bool m_texture = false;
	ComPtr<ID3D12Resource> m_textureResource;
	D3D12_RESOURCE_DESC m_textureDesc;
	ComPtr<ID3D12Resource> m_textureUploadHeap;
	wstring m_textureFile = L"NULL";
	int m_heapTextureNumber = -1;
	ComPtr<ID3D12Resource> m_materialBuffer;
	uint32_t m_materialBufferSize = 256;
	MaterialBuffer m_materialBufferData;
#pragma endregion

private:
#pragma region Private Variables
	XMFLOAT4X4 m_World;
	XMFLOAT3 m_position;
	XMFLOAT3 m_rotation;
	XMFLOAT3 m_scale;
	XMFLOAT3 m_orginalPosition;
	XMFLOAT3 m_orginalRotation;
	XMFLOAT3 m_orginalScale;
	MeshData m_meshData;
#pragma endregion
};