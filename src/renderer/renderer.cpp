#include "renderer.h"
#include "../application/application.h"
#include <helpers.h>

Renderer::Renderer(UINT width, UINT height) :
	m_width(width),
	m_height(height),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorSize(0)
{
	m_assetsPath = L"assets/";
	m_aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
}

UINT Renderer::GetHeight() { return m_height; }

UINT Renderer::GetWidth() { return m_width; }

float Renderer::GetAspectRatio() { return m_aspectRatio; }

void Renderer::ParseCommandLineArgs()
{
	int argc;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	for (size_t i = 0; i < argc; ++i)
	{
		if (wcscmp(argv[i], L"-w") == 0 || wcscmp(argv[i], L"--width") == 0)
		{
			m_width = ::wcstol(argv[++i], nullptr, 10);
		}
		if (wcscmp(argv[i], L"-h") == 0 || wcscmp(argv[i], L"--height") == 0)
		{
			m_height = wcstol(argv[++i], nullptr, 10);
		}
	}

	LocalFree(argv);
}

void Renderer::OnDestroy() {}

void Renderer::OnInit()
{
	LoadPipeline();
	//LoadTrianglePSO();
	LoadQuadPSO();
	LoadQuadMesh();

	LoadCubePSO();
	LoadCubeMesh();

	LoadSceneCB();

	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
	m_commandList->Close();

	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		WaitForPreviousFrame();
	}
}

void Renderer::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter4> hardwareAdapter = GetAdapter(factory.Get());
	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)));

	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));


	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = frameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
		));

	ThrowIfFailed(factory->MakeWindowAssociation(Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
	
	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors = frameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		//create a RTV for each frame
		for (UINT n = 0; n < frameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	{
		D3D12_RESOURCE_DESC depthDesc{};
		depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Alignment = 0;
		depthDesc.Width = m_width;
		depthDesc.Height = m_height;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.SampleDesc.Quality = 0;
		depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;

		CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		m_device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&m_depthStencilBuffer));

		//create descriptor heap for depth buffer
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));

		m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void Renderer::LoadTrianglePSO()
{
	//create root signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	SUCCEEDED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	//create PSO
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		std::wstring shaderPath(L"C:\\repos\\jayrenderX12\\out\\build\\x64-Debug\\shader.hlsl");

		ThrowIfFailed(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, 
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}

	LoadTriangleMesh();
}

void Renderer::LoadCubePSO()
{
	CD3DX12_ROOT_PARAMETER rootParams[2];
	rootParams[0].InitAsConstantBufferView(0);//(b0)Scene CB
	rootParams[1].InitAsConstantBufferView(1);//(b1)Cube CB

	D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(_countof(rootParams), rootParams, 0U, nullptr, rootSigFlags);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	SUCCEEDED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	SUCCEEDED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_cubeRootSignature)));

	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		ComPtr<ID3DBlob> error;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		std::wstring shaderPath(L"C:\\repos\\jayrenderX12\\out\\build\\x64-Debug\\cube_shaders.hlsl"); //TODO: add copy script to build

		if (FAILED(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
		{
			if (error) OutputDebugString(static_cast<LPCWSTR>(error->GetBufferPointer()));
		}
		error = nullptr;

		//ThrowIfFailed(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
		if (FAILED(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
		{
			if (error) OutputDebugString(static_cast<LPCWSTR>(error->GetBufferPointer()));
		}

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilDesc.StencilEnable = FALSE;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_cubeRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_cubePSO)));
	}

}

void Renderer::LoadQuadPSO()
{
	//create root signature
	CD3DX12_ROOT_PARAMETER rootParams[2];
	rootParams[0].InitAsConstantBufferView(0);
	rootParams[1].InitAsConstantBufferView(1);

	// Allow input layout and deny uneccessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(_countof(rootParams), rootParams, 0U, nullptr, rootSigFlags);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	SUCCEEDED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	SUCCEEDED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_quadRootSignature)));

	//create PSO
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		ComPtr<ID3DBlob> error;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		std::wstring shaderPath(L"C:\\repos\\jayrenderX12\\out\\build\\x64-Debug\\quad_shaders.hlsl"); //TODO: add copy script to build

		//ThrowIfFailed(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		if (FAILED(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
		{
			if (error) OutputDebugStringA(static_cast<LPCSTR>(error->GetBufferPointer()));
		}
		error = nullptr;

		//ThrowIfFailed(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
		if (FAILED(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
		{
			if (error) OutputDebugString(static_cast<LPCWSTR>(error->GetBufferPointer()));
		}

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_quadRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_quadPSO)));
	}

}

void Renderer::LoadCubeMesh()
{

	Vertex cubeVertexData[]
	{
		//front red
		{ {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
		{ {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
		{ {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
		{ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },

		//right blue
		{ {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
		{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
		{ {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
		{ {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },

		//back orange
		{ {1.0f, 1.0f, 1.0f}, {0.9f, 0.35f, 0.0f} },
		{ {0.0f, 1.0f, 1.0f}, {0.9f, 0.35f, 0.0f} },
		{ {0.0f, 0.0f, 1.0f}, {0.9f, 0.35f, 0.0f} },
		{ {1.0f, 0.0f, 1.0f}, {0.9f, 0.35f, 0.0f} },

		//left green
		//{ {0.0f, 1.0f, 0.0f}, {0.2f, 0.8f, 0.2f} },
		//{ {0.0f, 1.0f, 1.0f}, {0.2f, 0.8f, 0.2f} },
		//{ {0.0f, 0.0f, 1.0f}, {0.2f, 0.8f, 0.2f} },
		//{ {0.0f, 0.0f, 0.0f}, {0.2f, 0.8f, 0.2f} },
		{ {0.0f, 1.0f, 1.0f}, {0.2f, 0.8f, 0.2f} },
		{ {0.0f, 1.0f, 0.0f}, {0.2f, 0.8f, 0.2f} },
		{ {0.0f, 0.0f, 0.0f}, {0.2f, 0.8f, 0.2f} },
		{ {0.0f, 0.0f, 1.0f}, {0.2f, 0.8f, 0.2f} },

		//top white
		{ {0.0f, 1.0f, 0.0f}, {1.f, 1.f, 1.f} },
		{ {0.0f, 1.0f, 1.0f}, {1.f, 1.f, 1.f} },
		{ {1.0f, 1.0f, 1.0f}, {1.f, 1.f, 1.f} },
		{ {1.0f, 1.0f, 0.0f}, {1.f, 1.f, 1.f} },

		//bottom yellow
		//{ {0.0f, 0.0f, 0.0f}, {1.f, 0.6f, 0.f} },
		//{ {0.0f, 0.0f, 1.0f}, {1.f, 0.6f, 0.f} },
		//{ {1.0f, 0.0f, 1.0f}, {1.f, 0.6f, 0.f} },
		//{ {1.0f, 0.0f, 0.0f}, {1.f, 0.6f, 0.f} },

		{ {0.0f, 0.0f, 1.0f}, {1.f, 0.6f, 0.f} },
		{ {0.0f, 0.0f, 0.0f}, {1.f, 0.6f, 0.f} },
		{ {1.0f, 0.0f, 0.0f}, {1.f, 0.6f, 0.f} },
		{ {1.0f, 0.0f, 1.0f}, {1.f, 0.6f, 0.f} },
	};

	const UINT vertexBufferSize = sizeof(cubeVertexData);

	CD3DX12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_cubeVertexBuffer)
	));

	UINT8* pVertexDataBegin{};
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_cubeVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, cubeVertexData, sizeof(cubeVertexData));
	m_cubeVertexBuffer->Unmap(0, nullptr);

	m_cubeVertexBufferView.BufferLocation = m_cubeVertexBuffer->GetGPUVirtualAddress();
	m_cubeVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_cubeVertexBufferView.SizeInBytes = vertexBufferSize;


	uint16_t cubeIndices[] = 
	{
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4,
		8, 9, 10, 10, 11, 8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20
	};
	const UINT indexBufferSize = sizeof(cubeIndices);

	auto idBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&idBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_cubeIndexBuffer)
	));

	UINT8* pIndexDataBegin{};
	ThrowIfFailed(m_cubeIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, cubeIndices, indexBufferSize);
	m_cubeIndexBuffer->Unmap(0, nullptr);

	m_cubeIndexBufferView.BufferLocation = m_cubeIndexBuffer->GetGPUVirtualAddress();
	m_cubeIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_cubeIndexBufferView.SizeInBytes = indexBufferSize;

	//Create constant buffers
	{
		const UINT cubeCBSize = sizeof(QuadCB);
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(cubeCBSize);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_cubeCB)
		));

		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_cubeCB->Map(0, &readRange, reinterpret_cast<void**>(&m_pCubeCBResource)));
		memcpy(m_pCubeCBResource, &m_cubeCBData, sizeof(m_cubeCBData));
	}
}

void Renderer::LoadQuadMesh()
{

	Vertex quadVertexData[]
	{
		{ {-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
		{ {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
		{ {1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
		{ {-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f} }
	};

	const UINT vertexBufferSize = sizeof(quadVertexData);

	CD3DX12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_quadVertexBuffer)
		));

	UINT8* pVertexDataBegin{};
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_quadVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, quadVertexData, sizeof(quadVertexData));
	m_quadVertexBuffer->Unmap(0, nullptr);

	m_quadVertexBufferView.BufferLocation = m_quadVertexBuffer->GetGPUVirtualAddress();
	m_quadVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_quadVertexBufferView.SizeInBytes = vertexBufferSize;


	uint16_t quadIndices[] = { 0, 1, 2, 2, 3, 0 };
	const UINT indexBufferSize = sizeof(quadIndices);

	auto idBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&idBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_quadIndexBuffer)
		));

	UINT8* pIndexDataBegin{};
	ThrowIfFailed(m_quadIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, quadIndices, indexBufferSize);
	m_quadIndexBuffer->Unmap(0, nullptr);

	m_quadIndexBufferView.BufferLocation = m_quadIndexBuffer->GetGPUVirtualAddress();
	m_quadIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_quadIndexBufferView.SizeInBytes = indexBufferSize;

	//Create constant buffers
	{
		const UINT quadCBSize = sizeof(QuadCB);
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(quadCBSize);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_quadCB)
		));

		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_quadCB->Map(0, &readRange, reinterpret_cast<void**>(&m_pQuadCBResource)));
		memcpy(m_pQuadCBResource, &m_quadCBData, sizeof(m_quadCBData));
	}
}

void Renderer::LoadTriangleMesh()
{
	//create vertex buffer
	Vertex triangleVerticesData[]
	{
		{ {10.0f, -10.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
		{ {-10.0f, -10.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
		{ {0.0f, 10.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }
	};
	
	const UINT vertexBufferSize = sizeof(triangleVerticesData);

	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize); 
	ThrowIfFailed(m_device->CreateCommittedResource(
	    &uploadHeapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(&m_vertexBuffer)));

	//copy data to vertex buffer
	UINT8* pVertexDataBegin{};
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVerticesData, sizeof(triangleVerticesData));
	m_vertexBuffer->Unmap(0, nullptr);

	//initialize the vertex buffer view
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;
}

void Renderer::PopulateCommandList()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	auto toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &toRenderTarget);
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	//
	// m_commandList->DrawInstanced(3, 1, 0, 0);
	//DrawQuad();
	DrawCube();

	auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &toPresent);

	ThrowIfFailed(m_commandList->Close());
}

void Renderer::DrawCube()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), 0, 0);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_commandList->SetGraphicsRootSignature(m_cubeRootSignature.Get());
	m_commandList->SetPipelineState(m_cubePSO.Get());

	m_commandList->SetGraphicsRootConstantBufferView(0, m_sceneCB->GetGPUVirtualAddress());

	UpdateCubeCB();
	m_commandList->SetGraphicsRootConstantBufferView(1, m_cubeCB->GetGPUVirtualAddress());

	m_commandList->IASetVertexBuffers(0, 1, &m_cubeVertexBufferView);
	m_commandList->IASetIndexBuffer(&m_cubeIndexBufferView);

	m_commandList->DrawIndexedInstanced(6 * 6, 1, 0, 0, 0);
}


void Renderer::DrawQuad()
{
	UpdateQuadCB();

	m_commandList->SetGraphicsRootSignature(m_quadRootSignature.Get());
	m_commandList->SetPipelineState(m_quadPSO.Get());

	m_commandList->SetGraphicsRootConstantBufferView(0, m_sceneCB->GetGPUVirtualAddress());
	m_commandList->SetGraphicsRootConstantBufferView(1, m_quadCB->GetGPUVirtualAddress());

	m_commandList->IASetVertexBuffers(0, 1, &m_quadVertexBufferView);
	m_commandList->IASetIndexBuffer(&m_quadIndexBufferView);
	m_commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Renderer::LoadSceneCB()
{
	//Create Scene constant buffer
	{
		const UINT scneneCBSize = sizeof(QuadCB);
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(scneneCBSize);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_sceneCB)
		));

		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_sceneCB->Map(0, &readRange, reinterpret_cast<void**>(&m_pSceneCBResource)));
		memcpy(m_pSceneCBResource, &m_sceneCBData, sizeof(m_sceneCBData));
	}
}

void Renderer::UpdateSceneCB()
{
	XMVECTOR eyePos{ 0.0f, 0.0f, -3.0f };
	XMVECTOR focusPos{ 0.0f, 0.0f, 0.0 };
	XMVECTOR upDir{ 0.0f, 1.0f, 0.0f };

	XMMATRIX view = XMMatrixLookAtLH(eyePos, focusPos, upDir);

	XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.f), m_viewport.Width / m_viewport.Height, 0.1f, 100.f);
	//XMMATRIX proj = XMMatrixOrthographicLH(10.f, 10.f, 0.1f, 50.f);

	XMStoreFloat4x4(&m_sceneCBData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_sceneCBData.proj, XMMatrixTranspose(proj));

	memcpy(m_pSceneCBResource, &m_sceneCBData, sizeof(SceneCB));
}

void Renderer::UpdateQuadCB()
{

	XMMATRIX model = XMMatrixIdentity();

	XMMATRIX scale = XMMatrixScaling(1.2f, 1.2f, 1.2f);

	XMMATRIX rotation = XMMatrixRotationY(XMConvertToRadians(45.0f));

	XMMATRIX translation = XMMatrixTranslation(-0.25f, 0.25f, 3.0f);

	model = model * scale * rotation * translation;

	XMStoreFloat4x4(&m_quadCBData.model, XMMatrixTranspose(model));


	memcpy(m_pQuadCBResource, &m_quadCBData, sizeof(QuadCB));
}

void Renderer::UpdateCubeCB()
{
	static float rotationDelta;

	XMMATRIX model = XMMatrixIdentity();

	XMMATRIX scale = XMMatrixScaling(1.2f, 1.2f, 1.2f);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(30.0f), XMConvertToRadians(45.0f - rotationDelta), 0.0f);
	//XMMATRIX rotation = XMMatrixRotationY(XMConvertToRadians(45.0f - rotationDelta));

	//rotation * XMMatrixRotationX(XMConvertToRadians(30.0f));

	XMMATRIX translation = XMMatrixIdentity();
	translation = XMMatrixTranslation(-0.25f, 0.25f, 3.0f);

	model = model * scale * rotation * translation;

	XMStoreFloat4x4(&m_cubeCBData.model, XMMatrixTranspose(model));


	memcpy(m_pCubeCBResource, &m_cubeCBData, sizeof(QuadCB));

	rotationDelta += 0.25f;
}

void Renderer::OnRender()
{
	UpdateSceneCB();

	PopulateCommandList();

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void Renderer::OnUpdate()
{

}

ComPtr<IDXGIAdapter4> Renderer::GetAdapter(IDXGIFactory4* factory)
{
	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; factory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			/*Check to see if the adapter can create a D3D12 device without actually creating it.
			The adapter with the largest dedicated video memory is favored*/
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}

		}

	return dxgiAdapter4;
}

void Renderer::WaitForPreviousFrame()
{
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

Renderer::~Renderer()
{
}
