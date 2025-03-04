#pragma once

#include "DXRApp.h"

class DXRRuntime
{
private:
	ComPtr<ID3D12Device5> m_device;
	DXRApp* m_app;

private:
	void PopulateCommandList();

public:
	DXRRuntime(DXRApp* app);

	void Render();
	void Update();
	void OnKeyUp(UINT8 key);
};
