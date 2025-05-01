#pragma once

// This is not my file I ain't commenting this.

#pragma region Includes
//Include{s}
#include <windows.h>
#include "DXSample.h"
#include <directxmath.h>
#include <fstream>		//For loading in an external file
#include <vector>		//For storing the XMFLOAT3/2 variables
#include <map>			//For fast searching when re-creating the index buffer
#pragma endregion

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct MeshData
{
	ComPtr<ID3D12Resource> VertexBuffer;
	ComPtr<ID3D12Resource> IndexBuffer;
	UINT VBStride;
	UINT VBOffset;
	UINT IndexCount;
	UINT VertexCount;
};

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Normal;
	XMFLOAT2 TexC;

	bool operator<(const SimpleVertex other) const
	{
		return memcmp((void*)this, (void*)&other, sizeof(SimpleVertex)) > 0;
	};
};

namespace OBJLoader
{
	//The only method you'll need to call
	MeshData Load(char* filename, ID3D12Device* _pd3dDevice, bool invertTexCoords = true);

	//Helper methods for the above method
	//Searhes to see if a similar vertex already exists in the buffer -- if true, we re-use that index
	bool FindSimilarVertex(const SimpleVertex& vertex, std::map<SimpleVertex, unsigned short>& vertToIndexMap, unsigned short& index);

	//Re-creates a single index buffer from the 3 given in the OBJ file
	void CreateIndices(const std::vector<XMFLOAT3>& inVertices, const std::vector<XMFLOAT2>& inTexCoords, const std::vector<XMFLOAT4>& inNormals, std::vector<unsigned short>& outIndices, std::vector<XMFLOAT3>& outVertices, std::vector<XMFLOAT2>& outTexCoords, std::vector<XMFLOAT4>& outNormals);
};