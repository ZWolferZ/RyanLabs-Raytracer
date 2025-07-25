#include "stdafx.h"

#pragma region Includes
//Include{s}
#include "DXRContext.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"
#pragma endregion

#pragma region Constructors

DXRContext::DXRContext(UINT width, UINT height) : m_frameIndex(0), m_viewport(0.0f, 0.0f, static_cast<float>(width),
	static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorSize(0)
{
}
#pragma endregion