

#include "d3d12renderer.h"
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QFile>

#include "D3dx12.h"

#include <d3d11on12.h>
#include <d3dcompiler.h>

//#if QT_CONFIG(d3d12)

struct QQuickConstantBuffer
{
	QMatrix4x4 model_view;
	QMatrix4x4 projection;
	float opacity;
};

D3D12RenderNode::D3D12RenderNode(QQuickItem *item)
    : m_item(item)
{
}

D3D12RenderNode::~D3D12RenderNode()
{
    releaseResources();
}

void D3D12RenderNode::releaseResources()
{
    if (vbPtr) {
        vertexBuffer->Unmap(0, nullptr);
        vbPtr = nullptr;
    }
    if (cbPtr) {
        constantBuffer->Unmap(0, nullptr);
        cbPtr = nullptr;
    }
    constantBuffer = nullptr;
    vertexBuffer = nullptr;
    rootSignature = nullptr;
    pipelineState = nullptr;
    m_device = nullptr;
}

void D3D12RenderNode::EnableDebg()
{
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();

		// Enable additional debug layers.
		//dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
}

void D3D12RenderNode::init()
{
	HRESULT hr;

    QSGRendererInterface *rif = m_item->window()->rendererInterface();
    m_device = static_cast<ID3D12Device *>(rif->getResource(m_item->window(), QSGRendererInterface::DeviceResource));
    Q_ASSERT(m_device);

	m_commandList = static_cast<ID3D12GraphicsCommandList *>(
		rif->getResource(m_item->window(), QSGRendererInterface::CommandListResource));
	Q_ASSERT(m_commandList);

	m_commandQueue = static_cast<ID3D12CommandQueue *>(
		rif->getResource(m_item->window(), QSGRendererInterface::CommandQueueResource));
	Q_ASSERT(m_commandQueue);

	/*********************************************************************************/

	// 创建纹理描述符堆
	//D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
	//srv_heap_desc.NumDescriptors = 1;
	//srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//hr = m_device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&m_srvHeap));
	//if (FAILED(hr))
	//{
	//	Q_ASSERT(false);
	//}

	//D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	//featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	//if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	//{
	//	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	//}

	//{
	//	CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
	//	//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	//	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	//	// todo: 待定是否能多个rootParameters
	//	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	//	//rootParameters[0].InitAsConstantBufferView(0);
	//	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);   //D3D12_SHADER_VISIBILITY_PIXEL

	//	D3D12_ROOT_SIGNATURE_FLAGS r_flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
	//		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
	//		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
	//		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
	//		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	//	// 创建根描述符
	//	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	//	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//	ComPtr<ID3DBlob> signature;
	//	ComPtr<ID3DBlob> error;
	//	hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
	//	if (FAILED(hr))
	//	{
	//		Q_ASSERT(false);
	//		return;
	//	}

	//	if (m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))
	//	{
	//		Q_ASSERT(false);
	//		return;
	//	}
	//}
	/*********************************************************************************/

	{
		// 创建描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc = {};
		cbv_heap_desc.NumDescriptors = 2;
		cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		hr = m_device->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(&m_cvvHeap));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// 创建描述符   常量描述符  和  纹理描述符          该描述符可以 用来 createconstantbufferview 和 shaderview
		// 需要两个   range1 表示常量槽    range2表示纹理槽    用这个试试D3D12_DESCRIPTOR_RANGE
		//CD3DX12_DESCRIPTOR_RANGE ranges[1];
		//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		//ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		D3D12_DESCRIPTOR_RANGE ranges[2];
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[0].NumDescriptors = 1;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;

		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[1].NumDescriptors = 1;
		ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[1].BaseShaderRegister = 0;
		ranges[1].RegisterSpace = 0;

		// 采样器描述
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// 这里描述符是用来指定shader如何访问数据的    slot槽与哪个参数对应啥的
		D3D12_ROOT_DESCRIPTOR_TABLE table_des;
		table_des.NumDescriptorRanges = 2;
		table_des.pDescriptorRanges = &ranges[0];

		D3D12_ROOT_PARAMETER rootParameter;
		//rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameter.DescriptorTable = table_des;
		//rootParameter.Descriptor.ShaderRegister = 0; // b0
		//rootParameter.Descriptor.RegisterSpace = 0;

		D3D12_ROOT_SIGNATURE_DESC desc;
		desc.NumParameters = 1;
		desc.pParameters = &rootParameter;
		desc.NumStaticSamplers = 1;
		desc.pStaticSamplers = &sampler;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
			qWarning("Failed to serialize root signature");
			return;
		}
		if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature)))) {
			qWarning("Failed to create root signature");
			return;
		}
	}

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	UINT compileFlags = 0; // D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	hr = D3DCompileFromFile(QString("./shader.hlsl").toStdWString().c_str(), 
		nullptr, nullptr, "VS_Simple", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
	if (FAILED(hr))
	{
		Q_ASSERT(false);
		return;
	}

	hr = D3DCompileFromFile(QString("./shader.hlsl").toStdWString().c_str(),
		nullptr, nullptr, "PS_Simple", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
	if (FAILED(hr))
	{
		Q_ASSERT(false);
		return;
	}

    D3D12_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rastDesc.CullMode = D3D12_CULL_MODE_BACK;
    rastDesc.FrontCounterClockwise = TRUE; // Vertices are given CCW

    // Enable color write and blending (premultiplied alpha). The latter is
    // needed because the example changes the item's opacity and we pass
    // inheritedOpacity() into the pixel shader. If that wasn't the case,
    // blending could have stayed disabled.
    const D3D12_RENDER_TARGET_BLEND_DESC premulBlendDesc = {
        TRUE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    };
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0] = premulBlendDesc;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = rootSignature.Get();
    //psoDesc.VS = vshader;
    //psoDesc.PS = pshader;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = rastDesc;
    psoDesc.BlendState = blendDesc;
    // No depth. The correct stacking of the item is ensured by the projection matrix.
    // Do not bother with stencil since we do not apply clipping in the
    // example. If clipping is desired, render() needs to set a different PSO
    // with stencil enabled whenever the RenderState indicates so.
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // not in use due to !DepthEnable, but this would be the correct format otherwise
    // We are rendering on the default render target so if the QuickWindow/View
    // has requested samples > 0 then we have to follow suit.
	// todo:这里的是指渲染对象   和
    const uint samples = qMax(1, m_item->window()->format().samples());
    psoDesc.SampleDesc.Count = samples;
    if (samples > 1) {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaInfo = {};
        msaaInfo.Format = psoDesc.RTVFormats[0];
        msaaInfo.SampleCount = samples;
        if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaInfo, sizeof(msaaInfo)))) {
            if (msaaInfo.NumQualityLevels > 0)
                psoDesc.SampleDesc.Quality = msaaInfo.NumQualityLevels - 1;
        }
    }

	hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr))
	{
        qWarning("Failed to create graphics pipeline state");
        return;
    }

    const UINT vertexBufferSize = (2 + 3) * 3 * sizeof(float);

    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufDesc;
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Alignment = 0;
    bufDesc.Width = vertexBufferSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.SampleDesc.Quality = 0;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (FAILED(m_device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &bufDesc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                 IID_PPV_ARGS(&vertexBuffer)))) {
        qWarning("Failed to create committed resource (vertex buffer)");
        return;
    }

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = vertexBufferSize / 3;
    vertexBufferView.SizeInBytes = vertexBufferSize;

	const D3D12_RANGE readRange = { 0, 0 };
	if (FAILED(vertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&vbPtr)))) {
		qWarning("Map failed");
		return;
	}

    bufDesc.Width = 256;
    if (FAILED(m_device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &bufDesc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                 IID_PPV_ARGS(&constantBuffer)))) {
        qWarning("Failed to create committed resource (constant buffer)");
        return;
    }

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(QQuickConstantBuffer) + 255) & ~255;
	m_device->CreateConstantBufferView(&cbvDesc, m_cvvHeap->GetCPUDescriptorHandleForHeapStart());

    if (FAILED(constantBuffer->Map(0, &readRange, reinterpret_cast<void **>(&cbPtr)))) {
        qWarning("Map failed (constant buffer)");
        return;
    }

    float *vp = reinterpret_cast<float *>(vbPtr);
    //vp += 2;
    //*vp++ = 1.0f; *vp++ = 0.0f; *vp++ = 0.0f;
    //vp += 2;
    //*vp++ = 0.0f; *vp++ = 1.0f; *vp++ = 0.0f;
    //vp += 2;
    //*vp++ = 0.0f; *vp++ = 0.0f; *vp++ = 1.0f;
	vp += 2;
	*vp++ = 1.0f; *vp++ = 0.0f; *vp++ = 0.0f;
	vp += 2;
	*vp++ = 0.0f; *vp++ = 1.0f; *vp++ = 0.0f;
	vp += 2;
	*vp++ = 0.0f; *vp++ = 0.0f; *vp++ = 0.0f;

	/*************************************************************************/
	
	ComPtr<ID3D12Resource> textureUploadHeap;

	// Create the texture.
	{
		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = TextureWidth;
		textureDesc.Height = TextureHeight;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		hr = (m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_texture)));

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

		// Create the GPU upload buffer.
		hr = (m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureUploadHeap)));

		std::vector<UINT8> texture = GenerateTextureData();
		
		// 提交纹理**************************************************************************
		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = &texture[0];
		textureData.RowPitch = TextureWidth * TexturePixelSize;
		textureData.SlicePitch = textureData.RowPitch * TextureHeight;
		//UpdateSubresources(m_commandList, m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);

		UINT64 n64RequiredSize = 0u;
		UINT nNumSubresources = 1u;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT foot_print = {};
		UINT64 n64TextureRowSizes = 0u;
		UINT nTextureRowNum = 0u;

		D3D12_RESOURCE_DESC dest_desc = m_texture->GetDesc();

		m_device->GetCopyableFootprints(&dest_desc, 0, nNumSubresources, 0, &foot_print, &nTextureRowNum, &n64TextureRowSizes, &n64RequiredSize);
		BYTE* pData = nullptr;
		hr = textureUploadHeap->Map(0, NULL, reinterpret_cast<void**>(&pData));

		BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + foot_print.Offset;
		const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(&texture[0]);
		for (UINT y = 0; y < nTextureRowNum; ++y)
		{
			memcpy(pDestSlice + static_cast<SIZE_T>(foot_print.Footprint.RowPitch) * y,
				pSrcSlice + static_cast<SIZE_T>(textureData.RowPitch) * y,
				textureData.RowPitch);
		}

		textureUploadHeap->Unmap(0, NULL);

		CD3DX12_TEXTURE_COPY_LOCATION Dst(m_texture.Get(), 0);
		CD3DX12_TEXTURE_COPY_LOCATION Src(textureUploadHeap.Get(), foot_print);
		m_commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

		// ***************************************************************************

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_cvvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_rtvDescriptorSize);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, srvHandle);

		hr = m_commandList->Close();
		ID3D12CommandList* ppcl[] = {m_commandList};
		m_commandQueue->ExecuteCommandLists(_countof(ppcl), ppcl);
	}
}

void D3D12RenderNode::render(const RenderState *state)
{
    if (!m_device)
        init();

    const int msize = 16 * sizeof(float);
    memcpy(cbPtr, matrix()->constData(), msize);
    memcpy(cbPtr + msize, state->projectionMatrix()->constData(), msize);
    const float opacity = inheritedOpacity();
    memcpy(cbPtr + 2 * msize, &opacity, sizeof(float));

    const QPointF p0(m_item->width() - 1, m_item->height() - 1);
    const QPointF p1(0, 0);
    const QPointF p2(0, m_item->height() - 1);

    float *vp = reinterpret_cast<float *>(vbPtr);
    *vp++ = p0.x();
    *vp++ = p0.y();
    vp += 3;
    *vp++ = p1.x();
    *vp++ = p1.y();
    vp += 3;
    *vp++ = p2.x();
    *vp++ = p2.y();

	m_commandList->SetPipelineState(pipelineState.Get());
	m_commandList->SetGraphicsRootSignature(rootSignature.Get());

	// test --------------------------- todo:假如能用  看看qml里使用图片会不会受这个影响
	ID3D12DescriptorHeap* ppHeaps[] = { m_cvvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cvvHeap->GetGPUDescriptorHandleForHeapStart());
	// -----------------------

	// 改用constantbufferview
	//m_commandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	m_commandList->DrawInstanced(3, 1, 0, 0);
}

// No need to reimplement changedStates() because no relevant commands are
// added to the command list in render().

QSGRenderNode::RenderingFlags D3D12RenderNode::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

QRectF D3D12RenderNode::rect() const
{
    return QRect(0, 0, m_item->width(), m_item->height());
}

void D3D12RenderNode::LoadPipeline()
{
	//D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	//srvHeapDesc.NumDescriptors = 1;
	//srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//if (FAILED(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap))))
	//{
	//	Q_ASSERT(false);
	//	return;
	//}
}

std::vector<UINT8> D3D12RenderNode::GenerateTextureData()
{
	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
	const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
	const UINT textureSize = rowPitch * TextureHeight;

	std::vector<UINT8> data(textureSize);
	UINT8* pData = &data[0];

	for (UINT n = 0; n < textureSize; n += TexturePixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0x00;        // R
			pData[n + 1] = 0x00;    // G
			pData[n + 2] = 0x00;    // B
			pData[n + 3] = 0xff;    // A
		}
		else
		{
			pData[n] = 0xff;        // R
			pData[n + 1] = 0xff;    // G
			pData[n + 2] = 0xff;    // B
			pData[n + 3] = 0xff;    // A
		}
	}

	return data;
}

void D3D12RenderNode::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	HRESULT hr = (m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		hr = (m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

//// 创建采样器描述符
//D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
//sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
//sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
//sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
//sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
//sampler_desc.MipLODBias = 0;
//sampler_desc.MaxAnisotropy = 0;
//sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
//sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
//sampler_desc.MinLOD = 0;
//sampler_desc.MaxLOD = 0;
//sampler_desc.ShaderRegister = 0;
//sampler_desc.RegisterSpace = 0;
//sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

//#endif // d3d12
