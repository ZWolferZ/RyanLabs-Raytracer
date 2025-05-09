#include "stdafx.h"

#pragma region Includes
//Include{s}
#include "DXRRuntime.h"
#include <chrono>
#include "DXRContext.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "DrawableGameObject.h"
#include "DXRSetup.h"
#include "imgui_internal.h"
#pragma endregion

#pragma region Constructors and Destructors

DXRRuntime::DXRRuntime(DXRApp* app)
{
	m_app = app;
	m_device = m_app->GetContext()->m_device;
	wstring windowName = m_app->GetTitle();
	m_windowName.assign(windowName.begin(), windowName.end());
}
#pragma endregion

#pragma region Render / Update Methods
void DXRRuntime::Render()
{
	DXRContext* context = m_app->GetContext();

	// Draw all the UI elements.
	DrawIMGUI();

	// Update the window title with the current performance statistics.
	DynamicWindowTitle();

	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { context->m_commandList.Get() };
	context->m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(context->m_swapChain->Present(1, 0));

	m_app->WaitForPreviousFrame();
}

void DXRRuntime::Update()
{
	DXRContext* context = m_app->GetContext();

	//Static initializes this value only once
	static ULONGLONG frameStart = GetTickCount64();
	static float deltaTime = 0.0f;

	// Calculate the time since the last frame.
	ULONGLONG frameNow = GetTickCount64();
	deltaTime = (frameNow - frameStart) / 1000.0f;
	m_totalTime += deltaTime;
	m_currentDeltaTime = deltaTime;
	frameStart = frameNow;

	// Update all the key inputs.
	KeyInputs(context);

	// Use the camera splines to animate the camera.
	if (m_playCameraSplineAnimation)
	{
		context->m_pCamera->CameraSplineAnimation(deltaTime, m_controlPoints, m_totalSplineAnimation);
	}

	// Update the camera position and rotation.
	m_app->m_DXSetup->UpdateCamera(m_rayXWidth, m_rayYWidth);

	// Update all drawable objects.
	for (size_t i = 0; i < m_app->m_drawableObjects.size(); ++i)
	{
		m_app->m_drawableObjects[i]->update(deltaTime);
		m_app->m_DXSetup->UpdateMaterialBuffers();
		m_app->m_instances[i].second = m_app->m_drawableObjects[i]->getTransform();
	}
}

void DXRRuntime::PopulateCommandList() {
	DXRContext* context = m_app->GetContext();
	// Command list allocators can only be reset when the associated
	// command lists have finished execution on the GPU; apps should use
	// fences to determine GPU execution progress.
	ThrowIfFailed(context->m_commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command
	// list, that command list can then be reset at any time and must be before
	// re-recording.
	ThrowIfFailed(context->m_commandList->Reset(context->m_commandAllocator.Get(), nullptr));

	// Set necessary state.
	context->m_commandList->SetGraphicsRootSignature(context->m_rootSignature.Get());
	context->m_commandList->RSSetViewports(1, &context->m_viewport);
	context->m_commandList->RSSetScissorRects(1, &context->m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		context->m_renderTargets[context->m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		context->m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), context->m_frameIndex,
		context->m_rtvDescriptorSize);
	context->m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// #DXR
	// Bind the descriptor heap giving access to the top-level acceleration
	// structure, as well as the raytracing output
	std::vector<ID3D12DescriptorHeap*> heaps = { context->m_srvUavHeap.Get() };
	context->m_commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()),
		heaps.data());

	// On the last frame, the raytracing output was used as a copy source, to
	// copy its contents into the render target. Now we need to transition it to
	// a UAV so that the shaders can write in it.
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
		context->m_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	context->m_commandList->ResourceBarrier(1, &transition);

	// Setup the raytracing task
	D3D12_DISPATCH_RAYS_DESC desc = {};
	// The layout of the SBT is as follows: ray generation shader, miss
	// shaders, hit groups. As described in the CreateShaderBindingTable method,
	// all SBT entries of a given type have the same size to allow a fixed
	// stride.

	// The ray generation shaders are always at the beginning of the SBT.
	uint32_t rayGenerationSectionSizeInBytes =
		context->m_sbtHelper.GetRayGenSectionSize();
	desc.RayGenerationShaderRecord.StartAddress =
		context->m_sbtStorage->GetGPUVirtualAddress();
	desc.RayGenerationShaderRecord.SizeInBytes =
		rayGenerationSectionSizeInBytes;

	// The miss shaders are in the second SBT section, right after the ray
	// generation shader. We have one miss shader for the camera rays and one
	// for the shadow rays, so this section has a size of 2*m_sbtEntrySize. We
	// also indicate the stride between the two miss shaders, which is the size
	// of a SBT entry
	uint32_t missSectionSizeInBytes = context->m_sbtHelper.GetMissSectionSize();
	desc.MissShaderTable.StartAddress =
		context->m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
	desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
	desc.MissShaderTable.StrideInBytes = context->m_sbtHelper.GetMissEntrySize();

	// The hit groups section start after the miss shaders. In this sample we
	// have one 1 hit group for the triangle
	uint32_t hitGroupsSectionSize = context->m_sbtHelper.GetHitGroupSectionSize();
	desc.HitGroupTable.StartAddress = context->m_sbtStorage->GetGPUVirtualAddress() +
		rayGenerationSectionSizeInBytes +
		missSectionSizeInBytes;
	desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
	desc.HitGroupTable.StrideInBytes = context->m_sbtHelper.GetHitGroupEntrySize();

	// Dimensions of the image to render, identical to a kernel launch dimension
	desc.Width = m_app->GetWidth();
	desc.Height = m_app->GetHeight();
	desc.Depth = 1;

	m_app->m_DXSetup->CreateTopLevelAS(m_app->m_instances, true);

	// Bind the raytracing pipeline
	context->m_commandList->SetPipelineState1(context->m_rtStateObject.Get());
	// Dispatch the rays and write to the raytracing output
	context->m_commandList->DispatchRays(&desc);

	// The raytracing output needs to be copied to the actual render target used
	// for display. For this, we need to transition the raytracing output from a
	// UAV to a copy source, and the render target buffer to a copy destination.
	// We can then do the actual copy, before transitioning the render target
	// buffer into a render target, that will be then used to display the image
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		context->m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	context->m_commandList->ResourceBarrier(1, &transition);

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		context->m_renderTargets[context->m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_DEST);
	context->m_commandList->ResourceBarrier(1, &transition);

	context->m_commandList->CopyResource(context->m_renderTargets[context->m_frameIndex].Get(),
		context->m_outputResource.Get());

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		context->m_renderTargets[context->m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	context->m_commandList->ResourceBarrier(1, &transition);

	context->m_commandList->SetDescriptorHeaps(1, context->m_IMGUIDescHeap.GetAddressOf());
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context->m_commandList.Get());

	// Indicate that the back buffer will now be used to present.
	context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		context->m_renderTargets[context->m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(context->m_commandList->Close());
}

void DXRRuntime::DynamicWindowTitle()
{
	// Calculate the number of rays per second
	int raysPerSecond = ((m_app->GetWidth() / m_rayXWidth) * (m_app->GetHeight() / m_rayYWidth) * ImGui::GetIO().Framerate);

	static std::time_t date;

	date = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	char buffer[26];

	ctime_s(buffer, sizeof(buffer), &date);

	string dateAndTime(buffer);

	dateAndTime = dateAndTime.substr(0, 20);

	// Put all the information into a horrible string.
	std::string dynamicWindowName = m_windowName +
		"       Time Running: " + std::to_string(m_totalTime) + "s" +
		"       " + "FPS: " + std::to_string(ImGui::GetIO().Framerate) +
		"       " + "Frame Time: " + std::to_string(1000.0f / ImGui::GetIO().Framerate) + "ms" +
		"       " + "Rays Per Second: " + std::to_string(raysPerSecond) +
		"       " + "Date and Time: " + dateAndTime;

	// Set the window title to the new string.
	SetWindowTextA(Win32Application::GetHwnd(), dynamicWindowName.c_str());
}

#pragma endregion

#pragma region Control Methods
void DXRRuntime::OnKeyUp(UINT8 key)
{
	// Set the key to false in the inputs array.
	inputs[key] = false;
}

void DXRRuntime::OnKeyDown(UINT8 key)
{
	// Set the key to true in the inputs array.
	inputs[key] = true;
}

void DXRRuntime::KeyInputs(DXRContext* context)
{
	// Handle the key inputs for the camera and other controls.

	if (inputs['W'] == true) context->m_pCamera->MoveForward(m_cameraMoveSpeed * m_currentDeltaTime);
	if (inputs['A'] == true) context->m_pCamera->StrafeLeft(m_cameraMoveSpeed * m_currentDeltaTime);
	if (inputs['S'] == true) context->m_pCamera->MoveBackward(m_cameraMoveSpeed * m_currentDeltaTime);
	if (inputs['D'] == true) context->m_pCamera->StrafeRight(m_cameraMoveSpeed * m_currentDeltaTime);
	if (inputs['E'] == true) context->m_pCamera->MoveUp(m_cameraMoveSpeed * m_currentDeltaTime);
	if (inputs[VK_SPACE] == true) context->m_pCamera->MoveUp(m_cameraMoveSpeed * m_currentDeltaTime);
	if (inputs['Q'] == true) context->m_pCamera->MoveDown(m_cameraMoveSpeed * m_currentDeltaTime);
	if (inputs[VK_SHIFT] == true) context->m_pCamera->MoveDown(m_cameraMoveSpeed * m_currentDeltaTime);
	if (inputs[VK_NUMPAD8] == true) context->m_pCamera->RotatePitch(-m_cameraRotateSpeed * m_currentDeltaTime);
	if (inputs[VK_NUMPAD5] == true) context->m_pCamera->RotatePitch(m_cameraRotateSpeed * m_currentDeltaTime);
	if (inputs[VK_NUMPAD4] == true) context->m_pCamera->RotateYaw(-m_cameraRotateSpeed * m_currentDeltaTime);
	if (inputs[VK_NUMPAD6] == true) context->m_pCamera->RotateYaw(m_cameraRotateSpeed * m_currentDeltaTime);
	if (inputs[VK_NUMPAD9] == true) context->m_pCamera->RotateRoll(-m_cameraRotateSpeed * m_currentDeltaTime);
	if (inputs[VK_NUMPAD7] == true) context->m_pCamera->RotateRoll(m_cameraRotateSpeed * m_currentDeltaTime);
	if (inputs['R'] == true) context->m_pCamera->Reset();
	if (inputs[VK_ESCAPE] == true) PostQuitMessage(0);
	if (inputs[VK_TAB] == true) m_selectedObject = nullptr;
}
#pragma endregion

#pragma region IMGUI Methods
void DXRRuntime::DrawIMGUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Draw the UI Windows
	if (!m_hideWindows)
	{
		DrawVersionWindow();
		DrawPerformanceWindow();
		DrawObjectSelectionWindow();
		DrawObjectMovementWindow();
		DrawCameraStatsWindow();
		DrawObjectMaterialWindow();
		DrawCameraSplineWindow();
		DrawLightingWindow();
		DrawSecretWindow();
		DrawReflectionWindow();
		DrawTextureWindow();
		DrawSamplerWindow();
	}

	DrawHideAllWindows();
}

// Most of the UI is pretty self-explanatory, so I won't comment on it.

void DXRRuntime::DrawPerformanceWindow()
{
	static float fpsHistory[100] = {};
	static int fpsIndex = 0;
	float currentFPS = ImGui::GetIO().Framerate;

	// Looping array for FPS history
	if (fpsIndex >= std::size(fpsHistory))
	{
		fpsIndex = 0;
	}

	fpsHistory[fpsIndex] = currentFPS;
	fpsIndex++;

	ImGui::SetNextWindowPos(ImVec2(875, 10), ImGuiCond_FirstUseEver);

	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Current FPS: %.3f", currentFPS);
	ImGui::Text("Current Frame-Time: %.3f ms", 1000 / currentFPS);
	ImGui::Separator();
	ImGui::PlotLines("FPS History", fpsHistory, std::size(fpsHistory), fpsIndex, "FPS",
		0, 100, ImVec2(300, 100));
	ImGui::Separator();
	ImGui::End();
}

void DXRRuntime::DrawObjectSelectionWindow()
{
	ImGui::SetNextWindowPos(ImVec2(10, 80), ImGuiCond_FirstUseEver);
	ImGui::Begin("Object Selection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Choose an object to select!");
	ImGui::Separator();

	for (DrawableGameObject* dgo : m_app->m_drawableObjects)
	{
		bool isSelected = (m_selectedObject == dgo);

		if (ImGui::Selectable(dgo->getObjectName().c_str(), isSelected))
		{
			if (isSelected)
			{
				m_selectedObject = nullptr;
			}
			else
			{
				m_selectedObject = dgo;
			}
		}
	}

	ImGui::End();
}

void DXRRuntime::DrawObjectMovementWindow()
{
	if (m_selectedObject != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 280), ImGuiCond_FirstUseEver);
		ImGui::Begin("Object Movement Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Manipulate the selected object:");
		ImGui::Text("Selected Object: %s", m_selectedObject->getObjectName().c_str());
		ImGui::Separator();

		XMFLOAT3 position = m_selectedObject->getPosition();
		if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&position), 0.005f))
		{
			m_selectedObject->setPosition(position);
		}

		XMFLOAT3 rotation = m_selectedObject->getRotation();
		if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&rotation), 0.5f, -361, 361))
		{
			m_selectedObject->setRotation(rotation);
		}

		XMFLOAT3 scale = m_selectedObject->getScale();
		if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.01f, -INFINITY, INFINITY))
		{
			m_selectedObject->setScale(scale);
		}

		ImGui::Text("(Drag the box or enter a number)");
		ImGui::Separator();

		ImGui::Text("Auto Rotate:");

		ImGui::SliderFloat("Rotation Speed", &m_selectedObject->m_autoRotationSpeed, 0.0f, 360.0f);

		ImGui::Checkbox("Auto Rotate (X+)", &m_selectedObject->m_autoRotateX);

		ImGui::Checkbox("Auto Rotate (Y+)", &m_selectedObject->m_autoRotateY);

		ImGui::Checkbox("Auto Rotate (Z+)", &m_selectedObject->m_autoRotateZ);

		ImGui::Separator();

		if (ImGui::Button("Reset Transform"))
		{
			m_selectedObject->resetTransform();
		}

		ImGui::End();
	}
}

void DXRRuntime::DrawCameraStatsWindow()
{
	DXRContext* context = m_app->GetContext();

	XMFLOAT3 cameraPosition = context->m_pCamera->GetPosition();

	ImGui::SetNextWindowPos(ImVec2(250, 10), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Separator();
	ImGui::Text("Camera Position: X - %.3f", cameraPosition.x);
	ImGui::Text("Camera Position: Y - %.3f", cameraPosition.y);
	ImGui::Text("Camera Position: Z - %.3f", cameraPosition.z);
	ImGui::Separator();

	if (ImGui::DragFloat3("Camera Position", reinterpret_cast<float*>(&cameraPosition), 0.01f, -INFINITY, INFINITY))
	{
		context->m_pCamera->SetPosition(cameraPosition);
	}

	float rayWidth[2] = { m_rayXWidth, m_rayYWidth };
	if (ImGui::DragFloat2("Ray Launch Index (X & Y)", reinterpret_cast<float*>(&rayWidth), 1.0f, 1, 10))
	{
		m_rayXWidth = rayWidth[0];
		m_rayYWidth = rayWidth[1];
	}

	ImGui::DragFloat("Camera FOV", &m_app->m_DXSetup->m_fovAngleY, 0.01f, 0.1f, 2.0f);
	ImGui::SliderFloat("Camera Move Speed", &m_cameraMoveSpeed, 0.5f, 4.0f);

	ImGui::Text("(Drag the box or enter a number)");
	ImGui::Separator();
	if (ImGui::Button("Reset Camera"))
	{
		context->m_pCamera->Reset();
		m_app->m_DXSetup->m_fovAngleY = 0.785f;
		m_rayXWidth = 1;
		m_rayYWidth = 1;
		m_cameraMoveSpeed = 2.0f;
	}

	ImGui::End();
}

void DXRRuntime::DrawObjectMaterialWindow()
{
	if (m_selectedObject != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(440, 510), ImGuiCond_FirstUseEver);
		ImGui::Begin("Object Material Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Checkbox("Draw Tri-Outlines", &m_selectedObject->m_triOutline);
		ImGui::SliderFloat("Tri-Outline Thickness", &m_selectedObject->m_materialBufferData.triThickness, 0.01f, 0.1f);
		ImGui::ColorEdit3("Tri-Outline Colour", reinterpret_cast<float*>(&m_selectedObject->m_materialBufferData.triColour));

		ImGui::Separator();
		ImGui::Text("Change the colour of the object!");

		ImGui::ColorEdit4("Object Colour", reinterpret_cast<float*>(&m_selectedObject->m_materialBufferData.objectColour));

		ImGui::Text("Change roughness the selected object:");
		ImGui::SliderFloat("Roughness", &m_selectedObject->m_materialBufferData.roughness, 0.0f, 0.2f);
		ImGui::Separator();

		ImGui::End();
	}
}

void DXRRuntime::DrawCameraSplineWindow()
{
	ImGui::SetNextWindowPos(ImVec2(875, 187), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera Spline Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Checkbox("Start / Stop Camera Animation", &m_playCameraSplineAnimation);
	ImGui::Separator();
	ImGui::SliderFloat("Animation Duration", &m_totalSplineAnimation, 1.0f, 10.0f);
	ImGui::SliderFloat("Current Animation Time", &m_app->GetContext()->m_pCamera->m_splineTransition, 0.0f, 0.98f);
	ImGui::Separator();
	if (ImGui::Button("Add Point"))
	{
		if (m_controlPoints.size() < 10) m_controlPoints.push_back(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f));
	}

	if (ImGui::Button("Remove Point"))
	{
		if (m_controlPoints.size() > 4) m_controlPoints.erase(m_controlPoints.end() - 1);
	}

	// Okay, IMGUI is pretty cool
	for (size_t i = 0; i < m_controlPoints.size(); i++)
	{
		XMFLOAT3 point;
		XMStoreFloat3(&point, m_controlPoints[i]);

		string pointName = "Spline Point " + to_string(i);

		if (i == 0) pointName = "Initial Velocity";

		if (i == m_controlPoints.size() - 1) pointName = "Final Velocity";

		if (ImGui::DragFloat3(pointName.c_str(), reinterpret_cast<float*>(&point), 0.1f))
		{
			m_controlPoints[i] = XMLoadFloat3(&point);
		}
	}
	ImGui::Text("(Drag the box or enter a number)");

	ImGui::End();
}

// Okay this is pretty gross.
void DXRRuntime::DrawLightingWindow()
{
	ImGui::SetNextWindowPos(ImVec2(875, 455), ImGuiCond_FirstUseEver);
	ImGui::Begin("Point Light Editor:", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	static XMFLOAT4 lightPosition = m_app->m_DXSetup->m_originalLightPosition;
	static XMFLOAT4 ambientColor = m_app->m_DXSetup->m_originalLightAmbientColor;
	static XMFLOAT4 diffuseColor = m_app->m_DXSetup->m_originalLightDiffuseColor;
	static XMFLOAT4 specularColor = m_app->m_DXSetup->m_originalLightSpecularColor;
	static float specularPower = m_app->m_DXSetup->m_originalLightSpecularPower;
	static float pointLightRange = m_app->m_DXSetup->m_originalPointLightRange;
	static UINT shadowRayCount = m_app->m_DXSetup->m_originalShadowRayCount;
	ImGui::Separator();

	ImGui::DragFloat3("Light Position", reinterpret_cast<float*>(&lightPosition), 0.01f, -INFINITY, INFINITY);

	ImGui::DragFloat("Point Light Range", &pointLightRange, 0.05f, 1.0f, 20.0f);
	ImGui::Separator();

	ImGui::Checkbox("Toggle Shadows", &m_app->m_DXSetup->m_shadows);

	ImGui::DragInt("Soft Shadow Ray Count", reinterpret_cast<int*>(&shadowRayCount), 1, 1, 2000);

	ImGui::Separator();

	ImGui::ColorEdit4("Ambient Color", reinterpret_cast<float*>(&ambientColor));

	ImGui::ColorEdit4("Diffuse Color", reinterpret_cast<float*>(&diffuseColor));

	ImGui::ColorEdit4("Specular Color", reinterpret_cast<float*>(&specularColor));
	ImGui::DragFloat("Specular Power", &specularPower, 1.0f, 1.0f, 256.0f);

	m_app->m_DXSetup->UpdateLightingBuffer(lightPosition, ambientColor, diffuseColor, specularColor, specularPower, pointLightRange, shadowRayCount);
	ImGui::Text("(Drag the box or enter a number)");
	ImGui::Separator();

	if (ImGui::Button("Reset Light"))
	{
		m_app->m_DXSetup->UpdateLightingBuffer(
			m_app->m_DXSetup->m_originalLightPosition,
			m_app->m_DXSetup->m_originalLightAmbientColor,
			m_app->m_DXSetup->m_originalLightDiffuseColor,
			m_app->m_DXSetup->m_originalLightSpecularColor,
			m_app->m_DXSetup->m_originalLightSpecularPower,
			m_app->m_DXSetup->m_originalPointLightRange,
			m_app->m_DXSetup->m_originalShadowRayCount
		);

		lightPosition = m_app->m_DXSetup->m_originalLightPosition;
		ambientColor = m_app->m_DXSetup->m_originalLightAmbientColor;
		diffuseColor = m_app->m_DXSetup->m_originalLightDiffuseColor;
		specularColor = m_app->m_DXSetup->m_originalLightSpecularColor;
		specularPower = m_app->m_DXSetup->m_originalLightSpecularPower;
		pointLightRange = m_app->m_DXSetup->m_originalPointLightRange;
		m_app->m_DXSetup->m_shadows = m_app->m_DXSetup->m_originalShadows;
		shadowRayCount = m_app->m_DXSetup->m_originalShadowRayCount;
	}

	ImGui::End();
}
void DXRRuntime::DrawSecretWindow()
{
	ImGui::SetNextWindowPos(ImVec2(550, 800), ImGuiCond_FirstUseEver);
	ImGui::Begin("Secret Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	static vector<XMFLOAT4> originalObjectColour;

	if (ImGui::Checkbox("Activate Trans Mode?", &m_app->m_DXSetup->m_transBackgroundMode))
	{
		if (m_app->m_DXSetup->m_transBackgroundMode)
		{
			originalObjectColour.clear();

			for (int i = 0; i < m_app->m_drawableObjects.size(); i++)
			{
				if (!m_app->m_drawableObjects[i]->m_textMesh)
				{
					originalObjectColour.push_back(m_app->m_drawableObjects[i]->m_materialBufferData.objectColour);
					if (i % 3 == 0)
						m_app->m_drawableObjects[i]->m_materialBufferData.objectColour = { 1.0f, 0.58f, 0.9f, 1.0f };
					else if (i % 3 == 1)
						m_app->m_drawableObjects[i]->m_materialBufferData.objectColour = { 0.5f, 0.8f, 1.0f, 1.0f };
					else if (i % 3 == 2)
						m_app->m_drawableObjects[i]->m_materialBufferData.objectColour = { 1.0f, 1.0f, 1.0f, 1.0f };
				}
			}
		}
		else
		{
			for (int i = 0; i < m_app->m_drawableObjects.size(); i++)
			{
				if (!m_app->m_drawableObjects[i]->m_textMesh)
				{
					m_app->m_drawableObjects[i]->m_materialBufferData.objectColour = originalObjectColour[i];
				}
			}

			originalObjectColour.clear();
		}
	}
	ImGui::End();
}
void DXRRuntime::DrawReflectionWindow()
{
	ImGui::SetNextWindowPos(ImVec2(10, 580), ImGuiCond_FirstUseEver);

	if (m_selectedObject != nullptr)
	{
		ImGui::Begin("Object Reflection Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Change reflective-ness the selected object:");
		ImGui::Text("Selected Object: %s", m_selectedObject->getObjectName().c_str());
		ImGui::Separator();
		ImGui::Checkbox("Enable Reflection", &m_selectedObject->m_reflection);
		ImGui::SliderFloat("Reflection Shininess", &m_selectedObject->m_materialBufferData.shininess, 0.1f, 1.0f);
		ImGui::SliderInt("Max Recursion Depth", &m_selectedObject->m_materialBufferData.maxRecursionDepth, 1, 20);

		ImGui::End();
	}
}
void DXRRuntime::DrawTextureWindow()
{
	ImGui::SetNextWindowPos(ImVec2(350, 280), ImGuiCond_FirstUseEver);

	if (m_selectedObject != nullptr)
	{
		ImGui::Begin("Object Texture Selection Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		for (DrawableGameObject* dgo : m_app->m_drawableObjects)
		{
			string textureName;

			textureName.assign(dgo->m_textureFile.begin(), dgo->m_textureFile.end());

			if (dgo == m_selectedObject)
			{
				ImGui::Text("Texture Number: %i", dgo->m_heapTextureNumber);
				ImGui::Text("Texture File: %s", textureName.c_str());
				ImGui::Separator();
				if (ImGui::BeginListBox("Available Textures"))
				{
					// This is why I stored the textures in a vector of pairs.
					for (int i = 0; i < m_app->m_DXSetup->m_textures.size(); i++)
					{
						string textureLabel;

						textureLabel.assign(m_app->m_DXSetup->m_textures[i].first.begin(), m_app->m_DXSetup->m_textures[i].first.end());

						bool isSelected = false;
						if (m_selectedObject)
						{
							isSelected = (m_selectedObject->m_textureFile == m_app->m_DXSetup->m_textures[i].first &&
								m_selectedObject->m_heapTextureNumber == m_app->m_DXSetup->m_textures[i].second);
						}

						if (ImGui::Selectable(textureLabel.c_str(), isSelected))
						{
							if (m_selectedObject)
							{
								m_selectedObject->m_textureFile = m_app->m_DXSetup->m_textures[i].first;
								m_selectedObject->m_heapTextureNumber = m_app->m_DXSetup->m_textures[i].second;
								m_selectedObject->m_texture = true;
								m_app->m_DXSetup->UpdateShaderBindingTable();
							}
						}
					}
					ImGui::EndListBox();
				}

				if (ImGui::Button("Remove Texture"))
				{
					m_selectedObject->m_textureFile = L"NULL";
					m_selectedObject->m_heapTextureNumber = -1;
					m_selectedObject->m_texture = false;
					m_app->m_DXSetup->UpdateShaderBindingTable();
				}
			}
		}

		if (m_selectedObject->m_heapTextureNumber != -1)
		{
			if (ImGui::Checkbox("Enable Texture", &m_selectedObject->m_texture))
			{
				if (m_selectedObject->m_materialBufferData.texture == 1)
				{
					m_selectedObject->m_materialBufferData.texture = 0;
				}
				else
				{
					m_selectedObject->m_materialBufferData.texture = 1;
				}
			}
		}

		ImGui::End();
	}
}
void DXRRuntime::DrawSamplerWindow()
{
	ImGui::SetNextWindowPos(ImVec2(250, 240), ImGuiCond_FirstUseEver);

	ImGui::Begin("Texture Sampler Selection Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Choose a Sampler Type for the Textures!");

	string samplerTypeNames[3] = { "Linear Sampling", "Anisotropic Sampling" , "Point Sampling" };

	int currentSamplerIndex = m_app->m_DXSetup->m_samplerType;

	if (ImGui::BeginCombo("Sampler Type", samplerTypeNames[currentSamplerIndex].c_str()))
	{
		for (int i = 0; i < 3; i++)
		{
			bool isSelected = (currentSamplerIndex == i);

			if (ImGui::Selectable(samplerTypeNames[i].c_str(), isSelected))
			{
				m_app->m_DXSetup->m_samplerType = static_cast<SamplerType>(i);
				m_app->m_DXSetup->UpdateRaytracingPipeline();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::End();
}
void DXRRuntime::DrawHideAllWindows()
{
	ImGui::SetNextWindowPos(ImVec2(680, 10), ImGuiCond_FirstUseEver);
	ImGui::Begin("Show/Hide UI", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Checkbox("Hide All Windows", &m_hideWindows);
	ImGui::End();
}
void DXRRuntime::DrawVersionWindow()
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::Begin("RyanLabs Path Tracer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("ImGUI version: (%s)", IMGUI_VERSION);
	ImGui::Text("Application Runtime (%f)", m_totalTime);

	ImGui::End();
}

#pragma endregion