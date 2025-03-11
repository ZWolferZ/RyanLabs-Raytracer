#pragma once

#include "common.h"
#include "OBJLoader.h"

using Microsoft::WRL::ComPtr;

class DrawableGameObject
{
public:
	DrawableGameObject();
	~DrawableGameObject();

	void cleanup();

	DrawableGameObject* createCopy(); // creates a copy

	HRESULT								initMesh(ComPtr<ID3D12Device5> device);
	HRESULT								initPlaneMesh(ComPtr<ID3D12Device5> device);
	HRESULT 							initOBJMesh(ComPtr<ID3D12Device5> device, char* szOBJName);
	void								update(float t);
	//void								draw(ID3D11DeviceContext* pContext);
	ComPtr<ID3D12Resource>						getVertexBuffer() { return m_meshData.VertexBuffer; }
	ComPtr<ID3D12Resource>						getIndexBuffer() { return m_meshData.IndexBuffer; }

	//ID3D11ShaderResourceView**			getTextureResourceView() { return &m_pTextureResourceView; 	}
	XMMATRIX							getTransform() { return XMLoadFloat4x4(&m_World); }
	//ID3D11SamplerState**				getTextureSamplerState() { return &m_pSamplerLinear; }
	//ID3D11Buffer*						getMaterialConstantBuffer() { return m_pMaterialConstantBuffer;}
	void								setPosition(XMFLOAT3 position);
	void 								setRotation(XMFLOAT3 rotation);
	void 								setScale(XMFLOAT3 scale);
	unsigned int						getVertexCount() { return m_meshData.VertexCount; }
	unsigned int						getIndexCount() { return m_meshData.IndexCount; }

private:

	XMFLOAT4X4							m_World;

	/*ID3D11ShaderResourceView* m_pTextureResourceView;
	ID3D11SamplerState *				m_pSamplerLinear;
	MaterialPropertiesConstantBuffer	m_material;
	ID3D11Buffer*						m_pMaterialConstantBuffer = nullptr;*/
	XMFLOAT3							m_position;
	XMFLOAT3							m_rotation;
	XMFLOAT3							m_scale;

	MeshData 						m_meshData;
};
