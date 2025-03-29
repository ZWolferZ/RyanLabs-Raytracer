#include "stdafx.h"
#include "DXRRuntime.h"
#include "DXRContext.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "DrawableGameObject.h"
#include "DXRSetup.h"

DXRRuntime::DXRRuntime(DXRApp* app)
{
	m_app = app;
	m_device = m_app->GetContext()->m_device;
}

void DXRRuntime::Render()
{
	DXRContext* context = m_app->GetContext();

	DrawIMGUI();

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

	ULONGLONG frameNow = GetTickCount64();
	deltaTime = (frameNow - frameStart) / 1000.0f;
	m_totalTime += deltaTime;
	m_currentDeltaTime = deltaTime;
	frameStart = frameNow;

	KeyInputs(context);

	if (m_playCameraSplineAnimation)
	{
		context->m_pCamera->CameraSplineAnimation(deltaTime, m_controlPoints, m_totalSplineAnimation);
	}

	m_app->m_DXSetup->UpdateCamera(m_rayXWidth, m_rayYWidth);

	for (size_t i = 0; i < m_app->m_drawableObjects.size(); ++i)
	{
		DrawableGameObject* dgo = m_app->m_drawableObjects[i];
		dgo->update(deltaTime);
		m_app->m_instances[i].second = dgo->getTransform();
	}
}

void DXRRuntime::OnKeyUp(UINT8 key)
{
	inputs[key] = false;
}

void DXRRuntime::OnKeyDown(UINT8 key)
{
	inputs[key] = true;
}

void DXRRuntime::KeyInputs(DXRContext* context)
{
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

void DXRRuntime::DrawIMGUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Draw the UI Windows
	DrawVersionWindow();
	DrawPerformanceWindow();
	DrawObjectSelectionWindow();
	DrawObjectMovementWindow();
	DrawCameraStatsWindow();
	DrawHitColourWindow();
	DrawCameraSplineWindow();
	DrawLightingWindow();
	DrawSecretWindow();
}

void DXRRuntime::DrawPerformanceWindow()
{
	static float fpsHistory[100] = {};
	static int fpsIndex = 0;
	float currentFPS = ImGui::GetIO().Framerate;

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
		ImGui::SetNextWindowPos(ImVec2(10, 240), ImGuiCond_FirstUseEver);
		ImGui::Begin("Object Movement", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

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

void DXRRuntime::DrawHitColourWindow()
{
	ImGui::SetNextWindowPos(ImVec2(10, 550), ImGuiCond_FirstUseEver);
	ImGui::Begin("Hit Colour", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Change the colour of the object and plane hit shader!");
	ImGui::Separator();
	XMFLOAT4 objectColour = m_app->m_DXSetup->m_objectColour;
	XMFLOAT4 planeColour = m_app->m_DXSetup->m_planeColour;
	if (ImGui::DragFloat3("Object Hit Group Colour", reinterpret_cast<float*>(&objectColour), 0.01f, 0.0f, 1.0f))
	{
	}
	if (ImGui::DragFloat3("Plane Hit Group Colour", reinterpret_cast<float*>(&planeColour), 0.01f, 0.0f, 1.0f))
	{
	}
	ImGui::Text("(Drag the box or enter a number)");
	ImGui::Separator();
	m_app->m_DXSetup->UpdateColourBuffer(objectColour, planeColour);

	if (ImGui::Button("Reset Colours"))
	{
		m_app->m_DXSetup->UpdateColourBuffer(m_app->m_DXSetup->m_originalObjectColour, m_app->m_DXSetup->m_originalPlaneColour);
	}

	ImGui::End();
}

void DXRRuntime::DrawCameraSplineWindow()
{
	ImGui::SetNextWindowPos(ImVec2(875, 200), ImGuiCond_FirstUseEver);
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

void DXRRuntime::DrawLightingWindow()
{
	ImGui::SetNextWindowPos(ImVec2(875, 475), ImGuiCond_FirstUseEver);
	ImGui::Begin("Point Light Editor:", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	static XMFLOAT4 lightPosition = m_app->m_DXSetup->m_originalLightPosition;
	static XMFLOAT4 ambientColor = m_app->m_DXSetup->m_originalLightAmbientColor;
	static XMFLOAT4 diffuseColor = m_app->m_DXSetup->m_originalLightDiffuseColor;
	static XMFLOAT4 specularColor = m_app->m_DXSetup->m_originalLightSpecularColor;
	static float specularPower = m_app->m_DXSetup->m_originalLightSpecularPower;
	static float pointLightRange = m_app->m_DXSetup->m_originalPointLightRange;
	ImGui::Separator();

	ImGui::DragFloat3("Light Position", reinterpret_cast<float*>(&lightPosition), 0.01f, -INFINITY, INFINITY);

	ImGui::DragFloat("Point Light Range", &pointLightRange, 0.05f, 1.0f, 20.0f);
	ImGui::Separator();

	ImGui::ColorEdit4("Ambient Color", reinterpret_cast<float*>(&ambientColor));

	ImGui::ColorEdit4("Diffuse Color", reinterpret_cast<float*>(&diffuseColor));

	ImGui::ColorEdit4("Specular Color", reinterpret_cast<float*>(&specularColor));
	ImGui::DragFloat("Specular Power", &specularPower, 1.0f, 1.0f, 256.0f);

	m_app->m_DXSetup->UpdateLightingBuffer(lightPosition, ambientColor, diffuseColor, specularColor, specularPower, pointLightRange);
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
			m_app->m_DXSetup->m_originalPointLightRange
		);

		lightPosition = m_app->m_DXSetup->m_originalLightPosition;
		ambientColor = m_app->m_DXSetup->m_originalLightAmbientColor;
		diffuseColor = m_app->m_DXSetup->m_originalLightDiffuseColor;
		specularColor = m_app->m_DXSetup->m_originalLightSpecularColor;
		specularPower = m_app->m_DXSetup->m_originalLightSpecularPower;
		pointLightRange = m_app->m_DXSetup->m_originalPointLightRange;
	}

	ImGui::End();
}
void DXRRuntime::DrawSecretWindow()
{
	ImGui::SetNextWindowPos(ImVec2(520, 800), ImGuiCond_FirstUseEver);
	ImGui::Begin("Secret Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	if (ImGui::Checkbox("Activate Trans Mode?", &m_app->m_DXSetup->m_transBackgroundMode))
	{
		XMFLOAT4 transObjectColour = { 1.0f, 0.75f, 0.8f, 1.0f };
		XMFLOAT4 transPlaneColour = { 0.68f, 0.85f, 0.90f, 1.0f };

		if (m_app->m_DXSetup->m_transBackgroundMode)
		{
			m_app->m_DXSetup->UpdateColourBuffer(transObjectColour, transPlaneColour);
		}
		else
		{
			m_app->m_DXSetup->UpdateColourBuffer(m_app->m_DXSetup->m_originalObjectColour, m_app->m_DXSetup->m_originalPlaneColour);
		}
	}

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