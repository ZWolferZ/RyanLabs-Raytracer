//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#pragma region Includes
//Include{s}
#include <stdexcept>
#include "DXSample.h"
#include <dxcapi.h>
#include <vector>
class DrawableGameObject;
using namespace DirectX;
using namespace std;
class DXRContext;
class DXRSetup;
class DXRRuntime;
typedef std::vector<DrawableGameObject*> vecDrawables;
#define FRAME_COUNT 2

// Note that while ComPtr is used to manage the lifetime of resources on the
// CPU, it has no understanding of the lifetime of resources on the GPU. Apps
// must account for the GPU lifetime of resources to avoid destroying objects
// that may still be referenced by the GPU. An example of this can be found in
// the class method: OnDestroy().
using Microsoft::WRL::ComPtr;
#pragma endregion

class DXRApp : public DXSample {
	friend class DXRSetup;
	friend class DXRRuntime;

public:
	DXRApp(UINT width, UINT height, std::wstring name);

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	DXRContext* GetContext() { return m_DXRContext; }

	float GetAspectRatio() const { return m_aspectRatio; }

	virtual void OnKeyUp(UINT8 key);
	virtual void OnKeyDown(UINT8 key);

	void OnMouseMoveDelta(POINTS Delta);
	void OnMouseMove(int x, int y);

private:
	DXRContext* m_DXRContext;
	DXRRuntime* m_DXRuntime;
	DXRSetup* m_DXSetup;

	void WaitForPreviousFrame();

	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> m_instances;

	vecDrawables m_drawableObjects;
};
