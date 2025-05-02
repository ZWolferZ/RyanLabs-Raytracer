#pragma once

#pragma region Includes
//Include{s}
#include <map>
#include <unordered_map>
#include "DXRApp.h"
#pragma endregion

/// <summary>
/// The DXRRuntime class. This class holds the runtime data for the application.
/// </summary>
class DXRRuntime
{
private:
#pragma region Private Variables
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
	bool m_hideWindows = true;
#pragma endregion

#pragma region Render / Update Methods
	/// <summary>
	/// Populates the command list with rendering commands.
	/// </summary>
	void PopulateCommandList();

public:

	/// <summary>
	/// Renders the current frame.
	/// </summary>
	void Render();

	/// <summary>
	/// Updates the runtime state.
	/// </summary>
	void Update();

#pragma endregion

#pragma region Constructors and Destructors
	/// <summary>
	/// Initializes a new instance of the DXRRuntime class.
	/// </summary>
	/// <param name="app">Pointer to the DXR application instance.</param>
	DXRRuntime(DXRApp* app);
#pragma endregion

#pragma region Control Methods

	/// <summary>
	/// Handles key release events.
	/// </summary>
	/// <param name="key">The key that was released.</param>
	void OnKeyUp(UINT8 key);

	/// <summary>
	/// Handles key press events.
	/// </summary>
	/// <param name="key">The key that was pressed.</param>
	void OnKeyDown(UINT8 key);

	/// <summary>
	/// Holds the key inputs.
	/// </summary>
	/// <param name="context">Pointer to the DXR context.</param>
	// I dont know why I pass the pointer to the context here, I was a young boy okay.
	void KeyInputs(DXRContext* context);

#pragma endregion

#pragma region IMGUI Methods

	// Okay some of these summaries are a bit on the nose, but I am commited now.

	/// <summary>
	/// Draws the ImGui UI.
	/// </summary>
	void DrawIMGUI();

	/// <summary>
	/// Draws the version information window.
	/// </summary>
	void DrawVersionWindow();

	/// <summary>
	/// Draws the performance statistics window.
	/// </summary>
	void DrawPerformanceWindow();

	/// <summary>
	/// Draws the object selection window.
	/// </summary>
	void DrawObjectSelectionWindow();

	/// <summary>
	/// Draws the object movement window.
	/// </summary>
	void DrawObjectMovementWindow();

	/// <summary>
	/// Draws the camera statistics window.
	/// </summary>
	void DrawCameraStatsWindow();

	/// <summary>
	/// Draws the object material properties window.
	/// </summary>
	void DrawObjectMaterialWindow();

	/// <summary>
	/// Draws the camera spline animation window.
	/// </summary>
	void DrawCameraSplineWindow();

	/// <summary>
	/// Draws the lighting settings window.
	/// </summary>
	void DrawLightingWindow();

	/// <summary>
	/// Draws the secret trans window.
	/// </summary>
	void DrawSecretWindow();

	/// <summary>
	/// Draws the reflection settings window.
	/// </summary>
	void DrawReflectionWindow();

	/// <summary>
	/// Draws the texture settings window.
	/// </summary>
	void DrawTextureWindow();

	/// <summary>
	/// Draws the sampler settings window.
	/// </summary>
	void DrawSamplerWindow();

	/// <summary>
	/// Toggles the visibility of all windows, except this one.
	/// </summary>
	void DrawHideAllWindows();
#pragma endregion
};