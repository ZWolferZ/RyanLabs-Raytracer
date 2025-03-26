#include "stdafx.h"
#include "DXRSetup.h"
#include "DXRContext.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "nv_helpers_dx12/BottomLevelASGenerator.h"

#include "nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "nv_helpers_dx12/RootSignatureGenerator.h"

#include "DXRHelper.h"

#include "DrawableGameObject.h"

constexpr auto CUBE1_INDEX = 0;
constexpr auto CUBE2_INDEX = 1;
constexpr auto PLANE_INDEX = 2;
constexpr auto PLANE2_INDEX = 3;
constexpr auto OBJ_DONUT_INDEX = 4;
constexpr auto OBJ_BALL_INDEX = 5;
constexpr auto PLANEHITGROUP_INDEX = 1;
constexpr auto CUBEHITGROUP_INDEX = 0;

DXRSetup::DXRSetup(DXRApp* app)
{
	m_app = app;
	m_device = m_app->GetContext()->m_device;
}

void DXRSetup::initialise()
{
	LoadPipeline();
	LoadAssets();

	// Check the raytracing capabilities of the device
	CheckRaytracingSupport();

	// Setup the acceleration structures (AS) for raytracing. When setting up
	// geometry, each bottom-level AS has its own transform matrix.
	CreateAccelerationStructures();

	// Command lists are created in the recording state, but there is
	// nothing to record yet. The main loop expects it to be closed, so
	// close it now.
	ThrowIfFailed(m_app->GetContext()->m_commandList->Close());

	// Create the raytracing pipeline, associating the shader code to symbol names
	// and to their root signatures, and defining the amount of memory carried by
	// rays (ray payload)
	CreateRaytracingPipeline(); // #DXR

	// Allocate the buffer storing the raytracing output, with the same dimensions
	// as the target image
	CreateRaytracingOutputBuffer(); // #DXR

	CreateCamera();

	CreateColourBuffer();

	CreateLightingBuffer();

	// Create the buffer containing the raytracing result (always output in a
	// UAV), and create the heap referencing the resources used by the raytracing,
	// such as the acceleration structure
	CreateShaderResourceHeap(); // #DXR

	// Create the shader binding table and indicating which shaders
	// are invoked for each instance in the  AS
	CreateShaderBindingTable();

	SetupIMGUI();
}

void DXRSetup::CreateCamera()
{
	DXRContext* context = m_app->GetContext();

	context->m_pCamera = new Camera(XMFLOAT3(0, 0, 5), XMFLOAT3(0, 0, -1.0f), XMFLOAT3(0, 1, 0));

	context->m_cameraBuffer = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), context->m_cameraBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
}

void DXRSetup::UpdateCamera(float rX, float rY)
{
	DXRContext* context = m_app->GetContext();

	XMMATRIX view = context->m_pCamera->GetViewMatrix();

	XMMATRIX perspective = XMMatrixPerspectiveFovLH(m_fovAngleY, m_app->GetAspectRatio(), 0.1f, 1000.0f);

	XMMATRIX invView = XMMatrixInverse(nullptr, view);
	XMMATRIX invProj = XMMatrixInverse(nullptr, perspective);

	CameraBuffer cb;
	cb.invView = invView;
	cb.invProj = invProj;
	cb.rX = rX;
	cb.rY = rY;
	cb.transBackgroundMode = 0;

	if (m_transBackgroundMode)
	{
		cb.transBackgroundMode = 1;
	}

	// Map and update the constant buffer
	uint8_t* pData = nullptr;
	ThrowIfFailed(context->m_cameraBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
	memcpy(pData, &cb, sizeof(CameraBuffer));
	context->m_cameraBuffer->Unmap(0, nullptr);
}

void DXRSetup::CreateColourBuffer()
{
	DXRContext* context = m_app->GetContext();

	context->m_colourBuffer = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), context->m_colourBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	ColourBuffer cb;

	cb.objectColour = m_originalObjectColour;
	cb.planeColour = m_originalPlaneColour;

	m_objectColour = cb.objectColour;
	m_planeColour = cb.planeColour;

	uint8_t* pData;
	ThrowIfFailed(context->m_colourBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &cb, sizeof(ColourBuffer));
	context->m_colourBuffer->Unmap(0, nullptr);
}

void DXRSetup::UpdateColourBuffer(XMFLOAT4 objectColour, XMFLOAT4 planeColour)
{
	DXRContext* context = m_app->GetContext();

	ColourBuffer cb;

	cb.objectColour = objectColour;
	cb.planeColour = planeColour;

	m_objectColour = cb.objectColour;
	m_planeColour = cb.planeColour;

	uint8_t* pData;
	ThrowIfFailed(context->m_colourBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &cb, sizeof(ColourBuffer));
	context->m_colourBuffer->Unmap(0, nullptr);
}

void DXRSetup::CreateLightingBuffer()
{
	DXRContext* context = m_app->GetContext();

	context->m_lightingBuffer = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), context->m_lightingBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	LightParams cb;
	cb.lightPosition = m_originalLightPosition;
	cb.lightAmbientColor = m_originalLightAmbientColor;
	cb.lightDiffuseColor = m_originalLightDiffuseColor;
	cb.lightSpecularColor = m_originalLightSpecularColor;
	cb.lightSpecularPower = m_originalLightSpecularPower;
	cb.pointLightRange = m_originalPointLightRange;

	uint8_t* pData;

	ThrowIfFailed(context->m_lightingBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &cb, sizeof(LightParams));
	context->m_colourBuffer->Unmap(0, nullptr);
}

void DXRSetup::UpdateLightingBuffer(XMFLOAT4 lightPosition, XMFLOAT4 lightAmbientColor, XMFLOAT4 lightDiffuseColor, XMFLOAT4 lightSpecularColor, float lightSpecularPower, float pointLightRange)
{
	DXRContext* context = m_app->GetContext();

	LightParams cb;
	cb.lightPosition = lightPosition;
	cb.lightAmbientColor = lightAmbientColor;
	cb.lightDiffuseColor = lightDiffuseColor;
	cb.lightSpecularColor = lightSpecularColor;
	cb.lightSpecularPower = lightSpecularPower;
	cb.pointLightRange = pointLightRange;

	uint8_t* pData;

	ThrowIfFailed(context->m_lightingBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &cb, sizeof(LightParams));
	context->m_colourBuffer->Unmap(0, nullptr);
}

void DXRSetup::SetupIMGUI()
{
	// Setup Dear ImGui context

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 10000;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_app->GetContext()->m_IMGUIDescHeap)));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(Win32Application::GetHwnd());

	ImGui_ImplDX12_Init(m_device.Get(), FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		m_app->GetContext()->m_IMGUIDescHeap.Get(),
		// You'll need to designate a descriptor from your descriptor heap for Dear ImGui to use internally for its font texture's SRV
		m_app->GetContext()->m_IMGUIDescHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
		m_app->GetContext()->m_IMGUIDescHeap.Get()->GetGPUDescriptorHandleForHeapStart()
	);

	io.IniFilename = nullptr;
}

void DXRSetup::CheckRaytracingSupport() {
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
		&options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		throw std::runtime_error("Raytracing not supported on device");
}

// Load the rendering pipeline dependencies.
void DXRSetup::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;
	DXRContext* context = m_app->GetContext();

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the
	// active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_app->m_useWarpDevice) {
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}
	else {
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		m_app->GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(
		m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&context->m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_app->m_width;
	swapChainDesc.Height = m_app->m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		context->m_commandQueue.Get(), // Swap chain needs the queue so that it can force a
		// flush on it.
		Win32Application::GetHwnd(), &swapChainDesc, nullptr, nullptr,
		&swapChain));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(),
		DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&context->m_swapChain));
	context->m_frameIndex = context->m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(
			m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&context->m_rtvHeap)));

		context->m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
			context->m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++) {
			ThrowIfFailed(
				context->m_swapChain->GetBuffer(n, IID_PPV_ARGS(&context->m_renderTargets[n])));
			m_device->CreateRenderTargetView(context->m_renderTargets[n].Get(), nullptr,
				rtvHandle);
			rtvHandle.Offset(1, context->m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&context->m_commandAllocator)));
}

// Load the sample assets.
void DXRSetup::LoadAssets()
{
	DXRContext* context = m_app->GetContext();

	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(
			0, nullptr, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(
			&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&context->m_rootSignature)));
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
	}

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, context->m_commandAllocator.Get(),
		nullptr, IID_PPV_ARGS(&context->m_commandList)));

	DrawableGameObject* pDrawableObject = new DrawableGameObject();
	pDrawableObject->initCubeMesh(m_device);
	pDrawableObject->setPosition(XMFLOAT3(-0.5f, 0.0f, 0.0f));
	pDrawableObject->setRotation(XMFLOAT3(0, 0, 0));
	pDrawableObject->setScale(XMFLOAT3(0.25f, 0.25f, 0.25f));
	pDrawableObject->setOrginalTransformValues(pDrawableObject->getPosition(), pDrawableObject->getRotation(), pDrawableObject->getScale());
	pDrawableObject->m_autoRotateX = true;
	pDrawableObject->m_autoRotateY = true;
	pDrawableObject->update(0.0f);
	pDrawableObject->setObjectName("Cube 1");
	m_app->m_drawableObjects.push_back(pDrawableObject);

	DrawableGameObject* pCopy = pDrawableObject->createCopy();
	pCopy->setPosition(XMFLOAT3(0.5f, 0.0f, 0.0f));
	pCopy->setRotation(XMFLOAT3(0, 0, 0));
	pCopy->setScale(XMFLOAT3(0.25f, 0.25f, 0.25f));
	pCopy->setOrginalTransformValues(pCopy->getPosition(), pCopy->getRotation(), pCopy->getScale());
	pCopy->update(0.0f);
	pCopy->setObjectName("Cube 2");
	m_app->m_drawableObjects.push_back(pCopy);

	DrawableGameObject* pPlane = new DrawableGameObject();
	pPlane->initPlaneMesh(m_device);
	pPlane->setPosition(XMFLOAT3(0.0f, -1.5f, 0.0f));
	pPlane->setScale(XMFLOAT3(20.0f, 20.0f, 1.0f));
	pPlane->setRotation(XMFLOAT3(-90.0f, 0.0f, 0.0f));
	pPlane->setOrginalTransformValues(pPlane->getPosition(), pPlane->getRotation(), pPlane->getScale());
	pPlane->update(0.0f);
	pPlane->setObjectName("Plane 1");
	m_app->m_drawableObjects.push_back(pPlane);

	DrawableGameObject* pPlaneCopy = pPlane->createCopy();
	pPlaneCopy->setPosition(XMFLOAT3(0.0f, 0.0f, -2.0f));
	pPlaneCopy->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
	pPlaneCopy->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
	pPlaneCopy->setOrginalTransformValues(pPlaneCopy->getPosition(), pPlaneCopy->getRotation(), pPlaneCopy->getScale());
	pPlaneCopy->update(0.0f);
	pPlaneCopy->setObjectName("Plane 2");
	m_app->m_drawableObjects.push_back(pPlaneCopy);

	DrawableGameObject* pOBJ = new DrawableGameObject();
	pOBJ->initOBJMesh(m_device, R"(Objects\donut.obj)");
	pOBJ->setPosition(XMFLOAT3(0.8f, -0.8f, 0.0f));
	pOBJ->setRotation(XMFLOAT3(90, 0, 0));
	pOBJ->setScale(XMFLOAT3(0.15f, 0.15f, 0.15f));
	pOBJ->setOrginalTransformValues(pOBJ->getPosition(), pOBJ->getRotation(), pOBJ->getScale());
	pOBJ->update(0.0f);
	pOBJ->setObjectName("Donut 1");
	m_app->m_drawableObjects.push_back(pOBJ);

	DrawableGameObject* pOBJ2 = new DrawableGameObject();
	pOBJ2->initOBJMesh(m_device, R"(Objects\ball.obj)");
	pOBJ2->setPosition(XMFLOAT3(-0.8f, -0.8f, 0.0f));
	pOBJ2->setRotation(XMFLOAT3(0, 0, 0));
	pOBJ2->setScale(XMFLOAT3(0.15f, 0.15f, 0.15f));
	pOBJ2->setOrginalTransformValues(pOBJ2->getPosition(), pOBJ2->getRotation(), pOBJ2->getScale());
	pOBJ2->update(0.0f);
	pOBJ2->setObjectName("Ball 1");
	m_app->m_drawableObjects.push_back(pOBJ2);

	// Create synchronization objects and wait until assets have been uploaded to
	// the GPU.
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&context->m_fence)));
		context->m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		context->m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (context->m_fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command
		// list in our main loop but for now, we just want to wait for setup to
		// complete before continuing.
		m_app->WaitForPreviousFrame();
	}
}

//-----------------------------------------------------------------------------
//
// Combine the BLAS and TLAS builds to construct the entire acceleration
// structure required to raytrace the scene
//
void DXRSetup::CreateAccelerationStructures()
{
	DXRContext* context = m_app->GetContext();

	// Build the bottom AS from the Triangle vertex buffer
	AccelerationStructureBuffers cubelBuffers =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[CUBE1_INDEX]->getVertexBuffer().Get(), m_app->m_drawableObjects[CUBE1_INDEX]->getVertexCount()} },
			{ {m_app->m_drawableObjects[CUBE1_INDEX]->getIndexBuffer().Get(), m_app->m_drawableObjects[CUBE1_INDEX]->getIndexCount()} });

	AccelerationStructureBuffers planeBuffer =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[PLANE_INDEX]->getVertexBuffer().Get(), m_app->m_drawableObjects[PLANE_INDEX]->getVertexCount()} },
			{ {m_app->m_drawableObjects[PLANE_INDEX]->getIndexBuffer().Get(), m_app->m_drawableObjects[PLANE_INDEX]->getIndexCount()} });

	AccelerationStructureBuffers objDonutBuffer =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[OBJ_DONUT_INDEX]->getVertexBuffer().Get(), m_app->m_drawableObjects[OBJ_DONUT_INDEX]->getVertexCount()} },
			{ {m_app->m_drawableObjects[OBJ_DONUT_INDEX]->getIndexBuffer().Get(), m_app->m_drawableObjects[OBJ_DONUT_INDEX]->getIndexCount()} });

	AccelerationStructureBuffers objBallBuffer =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[OBJ_BALL_INDEX]->getVertexBuffer().Get(), m_app->m_drawableObjects[OBJ_BALL_INDEX]->getVertexCount()} },
			{ {m_app->m_drawableObjects[OBJ_BALL_INDEX]->getIndexBuffer().Get(), m_app->m_drawableObjects[OBJ_BALL_INDEX]->getIndexCount()} });

	// Just one instance for now
	m_app->m_instances.push_back(std::make_pair(cubelBuffers.pResult, m_app->m_drawableObjects[CUBE1_INDEX]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(cubelBuffers.pResult, m_app->m_drawableObjects[CUBE2_INDEX]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(planeBuffer.pResult, m_app->m_drawableObjects[PLANE_INDEX]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(planeBuffer.pResult, m_app->m_drawableObjects[PLANE2_INDEX]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(objDonutBuffer.pResult, m_app->m_drawableObjects[OBJ_DONUT_INDEX]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(objBallBuffer.pResult, m_app->m_drawableObjects[OBJ_BALL_INDEX]->getTransform()));

	CreateTopLevelAS(m_app->m_instances, false);

	// Flush the command list and wait for it to finish
	context->m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { context->m_commandList.Get() };
	context->m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	context->m_fenceValue++;
	context->m_commandQueue->Signal(context->m_fence.Get(), context->m_fenceValue);

	context->m_fence->SetEventOnCompletion(context->m_fenceValue, context->m_fenceEvent);
	WaitForSingleObject(context->m_fenceEvent, INFINITE);

	// Once the command list is finished executing, reset it to be reused for
	// rendering
	ThrowIfFailed(
		context->m_commandList->Reset(context->m_commandAllocator.Get(), nullptr));

	// Store the AS buffers. The rest of the buffers will be released once we exit
	// the function
//	context->m_bottomLevelAS = objBallBuffer.pResult;
}

//-----------------------------------------------------------------------------
// The ray generation shader needs to access 2 resources: the raytracing output
// and the top-level acceleration structure
//

ComPtr<ID3D12RootSignature> DXRSetup::CreateRayGenSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{ {0 /*u0*/, 1 /*1 descriptor */, 0 /*use the implicit register space 0*/,
		  D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/,
		  0 /*heap slot where the UAV is defined*/},
		 {0 /*t0*/, 1, 0,
		  D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,
		  1},{0 /*b0*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Camera parameters*/, 2} });

	return rsc.Generate(m_device.Get(), true);
}

//-----------------------------------------------------------------------------
// The hit shader communicates only through the ray payload, and therefore does
// not require any resources
//
ComPtr<ID3D12RootSignature> DXRSetup::CreateHitSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0 /*t0*/); // vertex data
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1 /*t1*/); // indices
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0 /*b0*/); // Colour buffer
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 1 /*b1*/); // Lighting buffer
	return rsc.Generate(m_device.Get(), true);
}

//-----------------------------------------------------------------------------
// The miss shader communicates only through the ray payload, and therefore
// does not require any resources
//
ComPtr<ID3D12RootSignature> DXRSetup::CreateMissSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{ {0 /*u0*/, 1 /*1 descriptor */, 0 /*use the implicit register space 0*/,
		  D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/,
		  0 /*heap slot where the UAV is defined*/},
		 {0 /*t0*/, 1, 0,
		  D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,
		  1},{0 /*b0*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Camera parameters*/, 2} });
	return rsc.Generate(m_device.Get(), true);
}

//-----------------------------------------------------------------------------
//
// The raytracing pipeline binds the shader code, root signatures and pipeline
// characteristics in a single structure used by DXR to invoke the shaders and
// manage temporary memory during raytracing
//
//
void DXRSetup::CreateRaytracingPipeline()
{
	DXRContext* context = m_app->GetContext();

	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(m_device.Get());

	// The pipeline contains the DXIL code of all the shaders potentially executed
	// during the raytracing process. This section compiles the HLSL code into a
	// set of DXIL libraries. We chose to separate the code in several libraries
	// by semantic (ray generation, hit, miss) for clarity. Any code layout can be
	// used.
	context->m_rayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"RayGen.hlsl");
	context->m_missLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Miss.hlsl");
	context->m_hitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Hit.hlsl");

	// In a way similar to DLLs, each library is associated with a number of
	// exported symbols. This
	// has to be done explicitly in the lines below. Note that a single library
	// can contain an arbitrary number of symbols, whose semantic is given in HLSL
	// using the [shader("xxx")] syntax
	pipeline.AddLibrary(context->m_rayGenLibrary.Get(), { L"RayGen" });
	pipeline.AddLibrary(context->m_missLibrary.Get(), { L"Miss" });
	pipeline.AddLibrary(context->m_hitLibrary.Get(), { L"ClosestHit",L"AnyHit",L"PlaneClosestHit" });

	// To be used, each DX12 shader needs a root signature defining which
	// parameters and buffers will be accessed.
	context->m_rayGenSignature = CreateRayGenSignature();
	context->m_missSignature = CreateMissSignature();
	context->m_hitSignature = CreateHitSignature();

	// 3 different shaders can be invoked to obtain an intersection: an
	// intersection shader is called
	// when hitting the bounding box of non-triangular geometry. This is beyond
	// the scope of this tutorial. An any-hit shader is called on potential
	// intersections. This shader can, for example, perform alpha-testing and
	// discard some intersections. Finally, the closest-hit program is invoked on
	// the intersection point closest to the ray origin. Those 3 shaders are bound
	// together into a hit group.

	// Note that for triangular geometry the intersection shader is built-in. An
	// empty any-hit shader is also defined by default, so in our simple case each
	// hit group contains only the closest hit shader. Note that since the
	// exported symbols are defined above the shaders can be simply referred to by
	// name.

	// Hit group for the triangles, with a shader simply interpolating vertex
	// colors
	pipeline.AddHitGroup(L"CubeHitGroup", L"ClosestHit", L"AnyHit");
	pipeline.AddHitGroup(L"PlaneHitGroup", L"PlaneClosestHit");

	// The following section associates the root signature to each shader. Note
	// that we can explicitly show that some shaders share the same root signature
	// (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred
	// to as hit groups, meaning that the underlying intersection, any-hit and
	// closest-hit shaders share the same root signature.
	pipeline.AddRootSignatureAssociation(context->m_rayGenSignature.Get(), { L"RayGen" });
	pipeline.AddRootSignatureAssociation(context->m_missSignature.Get(), { L"Miss" });
	pipeline.AddRootSignatureAssociation(context->m_hitSignature.Get(), { L"CubeHitGroup" , L"PlaneHitGroup" });

	// The payload size defines the maximum size of the data carried by the rays,
	// ie. the the data
	// exchanged between shaders, such as the HitInfo structure in the HLSL code.
	// It is important to keep this value as low as possible as a too high value
	// would result in unnecessary memory consumption and cache trashing.
	pipeline.SetMaxPayloadSize(4 * sizeof(float)); // RGB + distance

	// Upon hitting a surface, DXR can provide several attributes to the hit. In
	// our sample we just use the barycentric coordinates defined by the weights
	// u,v of the last two vertices of the triangle. The actual barycentrics can
	// be obtained using float3 barycentrics = float3(1.f-u-v, u, v);
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // barycentric coordinates

	// The raytracing process can shoot rays from existing hit points, resulting
	// in nested TraceRay calls. Our sample code traces only primary rays, which
	// then requires a trace depth of 1. Note that this recursion depth should be
	// kept to a minimum for best performance. Path tracing algorithms can be
	// easily flattened into a simple loop in the ray generation.
	pipeline.SetMaxRecursionDepth(1);

	// Compile the pipeline for execution on the GPU
	context->m_rtStateObject = pipeline.Generate();

	// Cast the state object into a properties object, allowing to later access
	// the shader pointers by name
	ThrowIfFailed(
		context->m_rtStateObject->QueryInterface(IID_PPV_ARGS(&context->m_rtStateObjectProps)));
}

//-----------------------------------------------------------------------------
//
// Allocate the buffer holding the raytracing output, with the same size as the
// output image
//
void DXRSetup::CreateRaytracingOutputBuffer()
{
	DXRContext* context = m_app->GetContext();

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB
	// formats cannot be used with UAVs. For accuracy we should convert to sRGB
	// ourselves in the shader
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = m_app->GetWidth();
	resDesc.Height = m_app->GetHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(&context->m_outputResource)));
}

//-----------------------------------------------------------------------------
//
// Create the main heap used by the shaders, which will give access to the
// raytracing output and the top-level acceleration structure
//
void DXRSetup::CreateShaderResourceHeap()
{
	DXRContext* context = m_app->GetContext();

	// Create a SRV/UAV/CBV descriptor heap. We need 2 entries - 1 UAV for the
	// raytracing output and 1 SRV for the TLAS
	context->m_srvUavHeap = nv_helpers_dx12::CreateDescriptorHeap(
		m_device.Get(), 3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	// Get a handle to the heap memory on the CPU side, to be able to write the
	// descriptors directly
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		context->m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

	// Create the UAV. Based on the root signature we created it is the first
	// entry. The Create*View methods write the view information directly into
	// srvHandle
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_device->CreateUnorderedAccessView(context->m_outputResource.Get(), nullptr, &uavDesc,
		srvHandle);

	// Add the Top Level AS SRV right after the raytracing output buffer
	srvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location =
		context->m_topLevelASBuffers.pResult->GetGPUVirtualAddress();

	// Write the acceleration structure view in the heap
	m_device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

	// Constant Buffer Increment Size
	srvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = context->m_cameraBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = context->m_cameraBufferSize;
	m_device->CreateConstantBufferView(&cbvDesc, srvHandle);
}

//-----------------------------------------------------------------------------
//
// The Shader Binding Table (SBT) is the cornerstone of the raytracing setup:
// this is where the shader resources are bound to the shaders, in a way that
// can be interpreted by the raytracer on GPU. In terms of layout, the SBT
// contains a series of shader IDs with their resource pointers. The SBT
// contains the ray generation shader, the miss shaders, then the hit groups.
// Using the helper class, those can be specified in arbitrary order.
//
void DXRSetup::CreateShaderBindingTable()
{
	DXRContext* context = m_app->GetContext();

	// The SBT helper class collects calls to Add*Program.  If called several
	// times, the helper must be emptied before re-adding shaders.
	context->m_sbtHelper.Reset();

	// The pointer to the beginning of the heap is the only parameter required by
	// shaders without root parameters
	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		context->m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();

	// The helper treats both root parameter pointers and heap pointers as void*,
	// while DX12 uses the
	// D3D12_GPU_DESCRIPTOR_HANDLE to define heap pointers. The pointer in this
	// struct is a UINT64, which then has to be reinterpreted as a pointer.
	auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	// The ray generation only uses heap data
	context->m_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });

	// The miss and hit shaders do not access any external resources: instead they
	// communicate their results through the ray payload
	context->m_sbtHelper.AddMissProgram(L"Miss", { heapPointer });

	// Adding the triangle hit shader
	context->m_sbtHelper.AddHitGroup(L"CubeHitGroup",
		{ (void*)(m_app->m_drawableObjects[CUBEHITGROUP_INDEX]->getVertexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->m_drawableObjects[CUBE1_INDEX]->getIndexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->m_DXRContext->m_colourBuffer->GetGPUVirtualAddress()),
			(void*)(m_app->m_DXRContext->m_lightingBuffer->GetGPUVirtualAddress())
		});

	context->m_sbtHelper.AddHitGroup(L"PlaneHitGroup",
		{ (void*)(m_app->m_drawableObjects[PLANEHITGROUP_INDEX]->getVertexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->m_drawableObjects[PLANEHITGROUP_INDEX]->getIndexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->m_DXRContext->m_colourBuffer->GetGPUVirtualAddress()),
			(void*)(m_app->m_DXRContext->m_lightingBuffer->GetGPUVirtualAddress())
		});

	// Compute the size of the SBT given the number of shaders and their
	// parameters
	uint32_t sbtSize = context->m_sbtHelper.ComputeSBTSize();

	// Create the SBT on the upload heap. This is required as the helper will use
	// mapping to write the SBT contents. After the SBT compilation it could be
	// copied to the default heap for performance.
	context->m_sbtStorage = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	if (!context->m_sbtStorage) {
		throw std::logic_error("Could not allocate the shader binding table");
	}
	// Compile the SBT from the shader and parameters info
	context->m_sbtHelper.Generate(context->m_sbtStorage.Get(), context->m_rtStateObjectProps.Get());
}

//-----------------------------------------------------------------------------
//
// Create a bottom-level acceleration structure based on a list of vertex
// buffers in GPU memory along with their vertex count. The build is then done
// in 3 steps: gathering the geometry, computing the sizes of the required
// buffers, and building the actual AS
//
AccelerationStructureBuffers DXRSetup::CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers)
{
	DXRContext* context = m_app->GetContext();
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	// Adding all vertex buffers and not transforming their position.
	for (size_t i = 0; i < vVertexBuffers.size(); i++) {
		if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
		{
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0,
				vVertexBuffers[i].second, sizeof(Vertex),
				vIndexBuffers[i].first.Get(), 0,
				vIndexBuffers[i].second, nullptr, 0, true);
		}
		else
		{
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second,
				sizeof(Vertex), 0, 0);
		}
	}

	// The AS build requires some scratch space to store temporary information.
	// The amount of scratch memory is dependent on the scene complexity.
	UINT64 scratchSizeInBytes = 0;
	// The final AS also needs to be stored in addition to the existing vertex
	// buffers. It size is also dependent on the scene complexity.
	UINT64 resultSizeInBytes = 0;

	bottomLevelAS.ComputeASBufferSizes(m_device.Get(), false, &scratchSizeInBytes,
		&resultSizeInBytes);

	// Once the sizes are obtained, the application is responsible for allocating
	// the necessary buffers. Since the entire generation will be done on the GPU,
	// we can directly allocate those on the default heap
	AccelerationStructureBuffers buffers;
	buffers.pScratch = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		nv_helpers_dx12::kDefaultHeapProps);
	buffers.pResult = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), resultSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps);

	// Build the acceleration structure. Note that this call integrates a barrier
	// on the generated AS, so that it can be used to compute a top-level AS right
	// after this method.
	bottomLevelAS.Generate(context->m_commandList.Get(), buffers.pScratch.Get(),
		buffers.pResult.Get(), false, nullptr);

	return buffers;
}

//-----------------------------------------------------------------------------
// Create the main acceleration structure that holds all instances of the scene.
// Similarly to the bottom-level AS generation, it is done in 3 steps: gathering
// the instances, computing the memory requirements for the AS, and building the
// AS itself
//
void DXRSetup::CreateTopLevelAS(
	const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>
	& instances, bool update // pair of bottom level AS and matrix of the instance
) {
	DXRContext* context = m_app->GetContext();

	context->m_topLevelASGenerator.RemoveAllInstances();

	// Gather all the instances into the builder helper
	for (size_t i = 0; i < instances.size(); i++) {
		if (i == PLANE_INDEX || i == PLANE2_INDEX)
		{
			context->m_topLevelASGenerator.AddInstance(instances[i].first.Get(),
				instances[i].second, static_cast<UINT>(i),
				static_cast<UINT>(PLANEHITGROUP_INDEX));
		}
		else if (i == CUBE1_INDEX || i == CUBE2_INDEX)
		{
			context->m_topLevelASGenerator.AddInstance(instances[i].first.Get(),
				instances[i].second, static_cast<UINT>(i),
				static_cast<UINT>(CUBEHITGROUP_INDEX));
		}
	}

	if (!update)
	{
		// As for the bottom-level AS, the building the AS requires some scratch space
		// to store temporary data in addition to the actual AS. In the case of the
		// top-level AS, the instance descriptors also need to be stored in GPU
		// memory. This call outputs the memory requirements for each (scratch,
		// results, instance descriptors) so that the application can allocate the
		// corresponding memory
		UINT64 scratchSize, resultSize, instanceDescsSize;

		context->m_topLevelASGenerator.ComputeASBufferSizes(m_device.Get(), true, &scratchSize,
			&resultSize, &instanceDescsSize);

		// Create the scratch and result buffers. Since the build is all done on GPU,
		// those can be allocated on the default heap
		context->m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(
			m_device.Get(), scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nv_helpers_dx12::kDefaultHeapProps);
		context->m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(
			m_device.Get(), resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nv_helpers_dx12::kDefaultHeapProps);

		// The buffer describing the instances: ID, shader binding information,
		// matrices ... Those will be copied into the buffer by the helper through
		// mapping, so the buffer has to be allocated on the upload heap.
		context->m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(
			m_device.Get(), instanceDescsSize, D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	}
	// After all the buffers are allocated, or if only an update is required, we
	// can build the acceleration structure. Note that in the case of the update
	// we also pass the existing AS as the 'previous' AS, so that it can be
	// refitted in place.
	context->m_topLevelASGenerator.Generate(context->m_commandList.Get(),
		context->m_topLevelASBuffers.pScratch.Get(),
		context->m_topLevelASBuffers.pResult.Get(),
		context->m_topLevelASBuffers.pInstanceDesc.Get());
}