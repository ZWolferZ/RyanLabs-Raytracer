#pragma once

#include "DXRApp.h"

class DXRRuntime
{
private:
	ComPtr<ID3D12Device5> m_device;
	DXRApp* m_app;
	float m_currentDeltaTime;
	DrawableGameObject* m_selectedObject = nullptr;

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
};
