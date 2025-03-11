#include "stdafx.h"
#include "DXRRuntime.h"
#include "DXRContext.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "DrawableGameObject.h"

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
	//Static initializes this value only once
	static ULONGLONG frameStart = GetTickCount64();
	static float deltaTime = 0.0f;
	ULONGLONG frameNow = GetTickCount64();
	deltaTime = (frameNow - frameStart) / 1000.0f;
	frameStart = frameNow;

	for (DrawableGameObject* dgo : m_app->m_drawableObjects)
	{
		dgo->update(deltaTime);
	}
}

void DXRRuntime::OnKeyUp(UINT8 key)
{
	if (key == VK_ESCAPE)
	{
		PostQuitMessage(0);
	}
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

	ImGui::SetNextWindowPos(ImVec2(850, 10), ImGuiCond_FirstUseEver);

	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Current FPS: %.3f", currentFPS);
	ImGui::Text("Current Frame-Time: %.3f ms", 1000 / currentFPS);
	ImGui::Separator();
	ImGui::PlotLines("FPS History", fpsHistory, std::size(fpsHistory), fpsIndex, "FPS",
		0, 100, ImVec2(300, 100));
	ImGui::Separator();
	ImGui::End();
}

void DXRRuntime::DrawVersionWindow()
{
	ImGui::Begin("DXR Path Tracer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("ImGUI version: (%s)", IMGUI_VERSION);
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