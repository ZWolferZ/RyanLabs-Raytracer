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

#include "Win32Application.h"

#include "DXRApp.h"
#include "resource.h"

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(DXSample* pSample, HINSTANCE hInstance,
	int nCmdShow) {
	// Parse the command line parameters
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	pSample->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"DXSampleClass";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(pSample->GetWidth()),
					   static_cast<LONG>(pSample->GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	m_hwnd = CreateWindow(windowClass.lpszClassName, pSample->GetTitle(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, // We have no parent window.
		nullptr, // We aren't using menus.
		hInstance, pSample);

	// Initialize the sample. OnInit is defined in each child-implementation of
	// DXSample.
	pSample->OnInit();

	ShowWindow(m_hwnd, nCmdShow);

	// Main sample loop.
	MSG msg = {};
	while (msg.message != WM_QUIT) {
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	pSample->OnDestroy();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main message handler for the sample.
LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message,
	WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	DXSample* pSample =
		reinterpret_cast<DXSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	static bool mouseDown = false;

	switch (message) {
	case WM_CREATE: {
		// Save the DXSample* passed in to CreateWindow.
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
				  return 0;

	case WM_KEYDOWN:
		if (pSample) {
			pSample->OnKeyDown(static_cast<UINT8>(wParam));
		}
		if (static_cast<UINT8>(wParam) == VK_ESCAPE)
			PostQuitMessage(0);
		return 0;
	case WM_RBUTTONDOWN:
		mouseDown = true;
		ShowCursor(false);
		break;
	case WM_RBUTTONUP:
		ShowCursor(true);
		mouseDown = false;
		break;

	case WM_MOUSEMOVE:
	{
		if (!mouseDown)
		{
			break;
		}

		// Get the dimensions of the window
		RECT rect;
		GetClientRect(hWnd, &rect);

		// Calculate the center position of the window
		POINT windowCenter;
		windowCenter.x = (rect.right - rect.left) / 2;
		windowCenter.y = (rect.bottom - rect.top) / 2;

		// Convert the client area point to screen coordinates
		ClientToScreen(hWnd, &windowCenter);

		// Get the current cursor position
		POINTS mousePos = MAKEPOINTS(lParam);
		POINT cursorPos = { mousePos.x, mousePos.y };
		ClientToScreen(hWnd, &cursorPos);

		// Calculate the delta from the window center
		POINTS delta;
		delta.x = cursorPos.x - windowCenter.x;
		delta.y = cursorPos.y - windowCenter.y;

		reinterpret_cast<DXRApp*>(pSample)->OnMouseMoveDelta(delta);
		reinterpret_cast<DXRApp*>(pSample)->OnMouseMove(cursorPos.x, cursorPos.y);

		// Recenter the cursor
		SetCursorPos(windowCenter.x, windowCenter.y);
	}
	break;

	case WM_KEYUP:
		if (pSample) {
			pSample->OnKeyUp(static_cast<UINT8>(wParam));
		}
		return 0;

	case WM_PAINT:
		if (pSample) {
			pSample->OnUpdate();
			pSample->OnRender();
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}