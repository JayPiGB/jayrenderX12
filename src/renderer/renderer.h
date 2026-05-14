#pragma once

#include <std_includes.h>

static const UINT frameCount = 2;
class Renderer
{
public:
	Renderer(UINT width, UINT height);
	 ~Renderer();

	void OnInit();
	void OnUpdate();
	void OnRender();
	void OnDestroy();

	void ParseCommandLineArgs();
	
	UINT GetWidth();
	UINT GetHeight();
	float GetAspectRatio();
	

	//pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[frameCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	//synchronization objects
	UINT m_frameIndex;
	UINT m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();

private:
	UINT m_width;
	UINT m_height;
	float m_aspectRatio;

	std::wstring m_assetsPath;

	ComPtr<IDXGIAdapter4> GetAdapter(IDXGIFactory4* factory);
};
