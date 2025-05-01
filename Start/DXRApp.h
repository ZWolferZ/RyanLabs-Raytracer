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

/// <summary>
/// Manages the initialization, rendering, and cleanup of DXR resources.
/// </summary>
class DXRApp : public DXSample {
	friend class DXRSetup;
	friend class DXRRuntime;

public:
#pragma region Constructors and Destructors
	/// <summary>
	/// Initializes a new instance of the DXRApp class.
	/// </summary>
	/// <param name="width">The width of the application window.</param>
	/// <param name="height">The height of the application window.</param>
	/// <param name="name">The name of the application.</param>
	DXRApp(UINT width, UINT height, std::wstring name);
#pragma endregion

#pragma region Public Methods
	/// <summary>
	/// Initializes the application.
	/// </summary>
	virtual void OnInit();

	/// <summary>
	/// Updates the application state.
	/// </summary>
	virtual void OnUpdate();

	/// <summary>
	/// Renders the application.
	/// </summary>
	virtual void OnRender();

	/// <summary>
	/// Cleans up resources used by the application.
	/// </summary>
	virtual void OnDestroy();

	/// <summary>
	/// Gets the DXR context.
	/// </summary>
	/// <returns>A pointer to the DXRContext.</returns>
	DXRContext* GetContext() { return m_DXRContext; }

	/// <summary>
	/// Gets the aspect ratio of the application window.
	/// </summary>
	/// <returns>The aspect ratio as a float.</returns>
	float GetAspectRatio() const { return m_aspectRatio; }

	/// <summary>
	/// Handles key release events.
	/// </summary>
	/// <param name="key">The key code of the released key.</param>
	virtual void OnKeyUp(UINT8 key);

	/// <summary>
	/// Handles key press events.
	/// </summary>
	/// <param name="key">The key code of the pressed key.</param>
	virtual void OnKeyDown(UINT8 key);

	/// <summary>
	/// Handles mouse movement with delta values.
	/// </summary>
	/// <param name="Delta">The delta movement of the mouse.</param>
	void OnMouseMoveDelta(POINTS Delta);

	/// <summary>
	/// Handles mouse movement.
	/// </summary>
	/// <param name="x">The x-coordinate of the mouse.</param>
	/// <param name="y">The y-coordinate of the mouse.</param>
	void OnMouseMove(int x, int y);
#pragma endregion

private:
#pragma region Private Methods
	/// <summary>
	/// Waits for the GPU to finish processing the previous frame.
	/// </summary>
	void WaitForPreviousFrame();
#pragma endregion

#pragma region Private Variables
	DXRContext* m_DXRContext;
	DXRRuntime* m_DXRuntime;
	DXRSetup* m_DXSetup;

	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> m_instances;

	vecDrawables m_drawableObjects;
#pragma endregion
};