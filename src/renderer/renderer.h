#pragma once

#include <std_includes.h>

using namespace DirectX;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 color;
	XMFLOAT3 uv;
};

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
	ComPtr<ID3D12Resource> m_depthStencilBuffer;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	UINT m_rtvDescriptorSize;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	//synchronization objects
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	void LoadPipeline();
	void LoadTrianglePSO();
	void LoadTriangleMesh();
	void PopulateCommandList();
	void WaitForPreviousFrame();

	void LoadQuadPSO();
	void LoadQuadMesh();
	void DrawQuad();

	struct alignas(256) SceneCB
	{
		XMFLOAT4X4 view;
		XMFLOAT4X4 proj;
	};

	SceneCB m_sceneCBData;
	ComPtr<ID3D12Resource> m_sceneCB;
	UINT8* m_pSceneCBResource;

	void LoadSceneCB();
	void UpdateSceneCB();


	ComPtr<ID3D12Resource> m_quadVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_quadVertexBufferView;

	ComPtr<ID3D12Resource> m_quadIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_quadIndexBufferView;

	ComPtr<ID3D12PipelineState> m_quadPSO;
	ComPtr<ID3D12RootSignature> m_quadRootSignature;

	struct alignas(256) QuadCB
	{
		XMFLOAT4X4 model;
	};

	QuadCB m_quadCBData;
	ComPtr<ID3D12Resource> m_quadCB;
	UINT8* m_pQuadCBResource;

	void UpdateQuadCB();

#pragma region CUBE
	ComPtr<ID3D12Resource> m_cubeVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_cubeVertexBufferView;

	ComPtr<ID3D12Resource> m_cubeIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_cubeIndexBufferView;

	ComPtr<ID3D12PipelineState> m_cubePSO;
	ComPtr<ID3D12RootSignature> m_cubeRootSignature;

	struct alignas(256) CubeCB
	{
		XMFLOAT4X4 model;
	};

	CubeCB m_cubeCBData;
	ComPtr<ID3D12Resource> m_cubeCB;
	UINT8* m_pCubeCBResource;

	void UpdateCubeCB();

	void LoadCubePSO();
	void LoadCubeMesh();
	void DrawCube();
#pragma endregion

private:
	UINT m_width;
	UINT m_height;
	float m_aspectRatio;

	std::wstring m_assetsPath;

	ComPtr<IDXGIAdapter4> GetAdapter(IDXGIFactory4* factory);
};
