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
#include "stdafx.h"

#pragma region Includes
//Include{s}
#include "DXRApp.h"
#include "DXRSetup.h"
#include "DXRContext.h"
#include "DXRRuntime.h"
#include "DrawableGameObject.h"
#pragma endregion

#pragma region Constructors and Destructors
DXRApp::DXRApp(UINT width, UINT height,
	std::wstring name)
	: DXSample(width, height, name)

{
	// Set a random seed for the random number generator.
	srand(static_cast<unsigned int>(time(0)));

	m_DXRContext = new DXRContext(width, height);
	m_DXSetup = new DXRSetup(this);
	m_DXRuntime = new DXRRuntime(this);
}
#pragma endregion

#pragma region Main Application Methods
void DXRApp::OnInit()
{
	m_DXSetup->initialise();
}

// Update frame-based values.
void DXRApp::OnUpdate()
{
	m_DXRuntime->Update();
}

void DXRApp::OnDestroy() {
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_DXRContext->m_fenceEvent);
}
#pragma endregion

#pragma region Render Methods
// Render the scene.
void DXRApp::OnRender() {
	m_DXRuntime->Render();
}

void DXRApp::WaitForPreviousFrame() {
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The
	// D3D12HelloFrameBuffering sample illustrates how to use fences for efficient
	// resource usage and to maximize GPU utilization.
	// Signal and increment the fence value.
	const UINT64 fence = m_DXRContext->m_fenceValue;
	ThrowIfFailed(m_DXRContext->m_commandQueue->Signal(m_DXRContext->m_fence.Get(), fence));
	m_DXRContext->m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_DXRContext->m_fence->GetCompletedValue() < fence) {
		ThrowIfFailed(m_DXRContext->m_fence->SetEventOnCompletion(fence, m_DXRContext->m_fenceEvent));
		WaitForSingleObject(m_DXRContext->m_fenceEvent, INFINITE);
	}

	m_DXRContext->m_frameIndex = m_DXRContext->m_swapChain->GetCurrentBackBufferIndex();
}
#pragma endregion

#pragma region Control Methods
void DXRApp::OnKeyUp(UINT8 key) {
	m_DXRuntime->OnKeyUp(key);
}

void DXRApp::OnKeyDown(UINT8 key)
{
	m_DXRuntime->OnKeyDown(key);
}

void DXRApp::OnMouseMoveDelta(POINTS Delta)
{
	m_DXRContext->m_pCamera->UpdateLookAt(Delta);
}

void DXRApp::OnMouseMove(int x, int y)
{
}
#pragma endregion