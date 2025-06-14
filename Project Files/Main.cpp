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
#pragma endregion

#pragma region Main Entry Point

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	DXRApp sample(1280, 720, L"RyanLabs Proprietary Real-Time Ray-Tracing Framework"); // Gotta represent the brand.
	return Win32Application::Run(&sample, hInstance, nCmdShow);
}
#pragma endregion