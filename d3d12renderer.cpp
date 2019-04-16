

#include "d3d12renderer.h"
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QFile>

#include <d3d12.h>

#include "d3dx12.h"

#include <d3dcompiler.h>

#if QT_CONFIG(d3d12)

struct QQuickConstantBuffer
{
	QMatrix4x4 model_view;
	QMatrix4x4 projection;
	float opacity;
};

void EnableDebugLayer()
{
#if defined(_DEBUG)
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ComPtr<ID3D12Debug> debugInterface;
	HRESULT hr = (D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	if (FAILED(hr))
	{
		Q_ASSERT(false);
	}
	debugInterface->EnableDebugLayer();
#endif
}

D3D12RenderNode::D3D12RenderNode(QQuickItem *item)
    : m_item(item)
{
	//EnableDebugLayer();
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

	

	// heap
	D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc = {};
	cbv_heap_desc.NumDescriptors = 2;
	cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = m_device->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(&m_srvHeap));

	m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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

	// descriptor table
	D3D12_DESCRIPTOR_RANGE ranges[2];
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 1;
	ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	ranges[0].BaseShaderRegister = 0;
	ranges[0].RegisterSpace = 0;

	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[1].NumDescriptors = 1;
	ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	ranges[1].BaseShaderRegister = 0;
	ranges[1].RegisterSpace = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE table_des;
	table_des.NumDescriptorRanges = 2;
	table_des.pDescriptorRanges = &ranges[0];

	D3D12_ROOT_PARAMETER rootParameter;
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter.DescriptorTable = table_des;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootParameter;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &sampler;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
		qWarning("Failed to serialize root signature");
		return;
	}
	if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)))) {
		qWarning("Failed to create root signature");
		return;
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
    rastDesc.CullMode = D3D12_CULL_MODE_FRONT;
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

    if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)))) {
        qWarning("Failed to create graphics pipeline state");
        return;
    }

    const UINT vertexBufferSize = (2 + 3) * draw_point_count * sizeof(float);

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
    vertexBufferView.StrideInBytes = vertexBufferSize / draw_point_count;
    vertexBufferView.SizeInBytes = vertexBufferSize;

    bufDesc.Width = 256;
    if (FAILED(m_device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &bufDesc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                 IID_PPV_ARGS(&constantBuffer)))) {
        qWarning("Failed to create committed resource (constant buffer)");
        return;
    }

    const D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(vertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&vbPtr)))) {
        qWarning("Map failed");
        return;
    }

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(QQuickConstantBuffer) + 255) & ~255;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_descriptorSize);
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

    if (FAILED(constantBuffer->Map(0, &readRange, reinterpret_cast<void **>(&cbPtr)))) {
        qWarning("Map failed (constant buffer)");
        return;
    }

    float *vp = reinterpret_cast<float *>(vbPtr);
	vp += 2;
	*vp++ = 1.0f; *vp++ = 0.0f; *vp++ = 0.0f;
	vp += 2;
	*vp++ = 0.0f; *vp++ = 1.0f; *vp++ = 0.0f;
	vp += 2;
	*vp++ = 0.0f; *vp++ = 0.0f; *vp++ = 1.0f;

	vp += 2;
	*vp++ = 1.0f; *vp++ = 0.0f; *vp++ = 0.0f;
	vp += 2;
	*vp++ = 0.0f; *vp++ = 0.0f; *vp++ = 1.0f;
	vp += 2;
	*vp++ = 1.0f; *vp++ = 1.0f; *vp++ = 1.0f;

	/////////////////////////////////////////////////////////////////////

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = TextureWidth;
	textureDesc.Height = TextureHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	hr = (m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_SHARED,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_texture)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());

	//SECURITY_ATTRIBUTES temp;
	//temp.bInheritHandle = true;
	//temp.lpSecurityDescriptor = nullptr;
	//temp.nLength = sizeof(SECURITY_ATTRIBUTES);
	//HANDLE handle = 0;
	//hr = m_device->CreateSharedHandle(m_texture.Get(), &temp, GENERIC_ALL, L"fucker", &handle);
	//qDebug() << hr << handle;

	hr = m_device->OpenSharedHandle((HANDLE)(0x0000000000000434), __uuidof(ID3D12Resource), (void**)(&share_resource));
	qDebug() << hr;
}

void D3D12RenderNode::render(const RenderState *state)
{
    if (!m_device)
        init();

    QSGRendererInterface *rif = m_item->window()->rendererInterface();
    ID3D12GraphicsCommandList *commandList = static_cast<ID3D12GraphicsCommandList *>(
        rif->getResource(m_item->window(), QSGRendererInterface::CommandListResource));
    Q_ASSERT(commandList);

    const int msize = 16 * sizeof(float);
    memcpy(cbPtr, matrix()->constData(), msize);
    memcpy(cbPtr + msize, state->projectionMatrix()->constData(), msize);
    const float opacity = inheritedOpacity();
    memcpy(cbPtr + 2 * msize, &opacity, sizeof(float));

    const QPointF p0(0, m_item->height() - 1);
    const QPointF p1(0, 0);
	const QPointF p2(m_item->width() - 1, 0);
    const QPointF p3(m_item->width() - 1, m_item->height() - 1);

	// todo: copy
	//commandList->CopyResource();

    float *vp = reinterpret_cast<float *>(vbPtr);
	*vp++ = p0.x();
	*vp++ = p0.y();
	vp += 3;
	*vp++ = p1.x();
	*vp++ = p1.y();
	vp += 3;
	*vp++ = p2.x();
	*vp++ = p2.y();

	vp += 3;
	*vp++ = p0.x();
	*vp++ = p0.y();
	vp += 3;
	*vp++ = p2.x();
	*vp++ = p2.y();
	vp += 3;
	*vp++ = p3.x();
	*vp++ = p3.y();

    commandList->SetPipelineState(pipelineState.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    commandList->DrawInstanced(6, 1, 0, 0);
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

#endif // d3d12
