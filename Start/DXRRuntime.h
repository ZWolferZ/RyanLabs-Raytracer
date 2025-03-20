#pragma once

#include <map>
#include <unordered_map>

#include "DXRApp.h"

class DXRRuntime
{
private:

	std::unordered_map<UINT8, bool > inputs;

	ComPtr<ID3D12Device5> m_device;
	DXRApp* m_app;
	float m_currentDeltaTime;
	DrawableGameObject* m_selectedObject = nullptr;

	float rayXWidth = 1;
	float rayYWidth = 1;

	float m_cameraMoveSpeed = 2.0f;
	float m_cameraRotateSpeed = 1.0f;

private:
	void PopulateCommandList();

public:
	DXRRuntime(DXRApp* app);

	void Render();
	void Update();
	void OnKeyUp(UINT8 key);
	void OnKeyDown(UINT8 key);

	void DrawIMGUI();

	void DrawVersionWindow();
	void DrawPerformanceWindow();
	void DrawObjectSelectionWindow();
	void DrawObjectMovementWindow();
	void DrawCameraStatsWindow();
	void DrawHitColourWindow();
};
