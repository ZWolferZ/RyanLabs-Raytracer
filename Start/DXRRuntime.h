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
	string m_windowName;
	float m_currentDeltaTime;
	float m_totalTime = 0.0f;
	DrawableGameObject* m_selectedObject = nullptr;

	bool m_playCameraSplineAnimation = false;

	float m_totalSplineAnimation = 3.0f;

	std::vector<XMVECTOR> m_controlPoints = {
	XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),  // Initial velocity
	XMVectorSet(0.0f,  0.0f, 5.0f, 0.0f),
	XMVectorSet(1.0f,  0.0f, 5.0f, 0.0f),
	XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f)   // Final velocity
	};

	float m_rayXWidth = 1;
	float m_rayYWidth = 1;

	float m_cameraMoveSpeed = 2.0f;
	float m_cameraRotateSpeed = 1.0f;
	bool m_hideWindows;

private:
	void PopulateCommandList();

public:
	DXRRuntime(DXRApp* app);

	void Render();
	void Update();
	void OnKeyUp(UINT8 key);
	void OnKeyDown(UINT8 key);
	void KeyInputs(DXRContext* context);

	void DrawIMGUI();

	void DrawVersionWindow();
	void DrawPerformanceWindow();
	void DrawObjectSelectionWindow();
	void DrawObjectMovementWindow();
	void DrawCameraStatsWindow();
	void DrawObjectMaterialWindow();
	void DrawCameraSplineWindow();
	void DrawLightingWindow();
	void DrawSecretWindow();
	void DrawReflectionWindow();
	void DrawHideAllWindows();
};
