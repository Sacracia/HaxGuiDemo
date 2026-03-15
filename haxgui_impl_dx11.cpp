#include "haxgui_impl_dx11.h"

#include "haxgui_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

namespace Hax::Gui
{
    extern const char* g_VertexShaderText;
    extern const char* g_PixelShaderText;

    struct DirectX11Backend : IBackend
    {
        DirectX11Backend(Context& ctx) : m_Context(ctx) {}

        bool                    Init(ID3D11Device* device);
        void                    Render() override;

        Texture2D               CreateTexture(const uint8* pixels, int width, int height) override;
        void                    DestroyTexture(TexHandle handle) override;

    private:
        bool                    CompileVertexShader(const char* vertexShaderText);
        bool                    CompilePixelShader(const char* pixelShaderText);
        void                    CreateConstantBuffers();
        void                    CreateRenderStates();
        void                    SetupRenderState(const Vector2& viewportSize);
        void                    CopyRenderItems(Vector<Layer*>& layers);
        void                    RenderLayers(Vector<Layer*>& layers);

    private:
        Context&                m_Context;
        ID3D11Device*           m_Device                = nullptr;
        ID3D11DeviceContext*    m_DeviceContext         = nullptr;
        ID3D11VertexShader*     m_VertexShader          = nullptr;
        ID3D11InputLayout*      m_InputLayout           = nullptr;
        ID3D11Buffer*           m_MatrixContantBuffer   = nullptr;
        ID3D11Buffer*           m_FontConstantBuffer    = nullptr;
        ID3D11PixelShader*      m_PixelShader           = nullptr;
        ID3D11BlendState*       m_BlendState            = nullptr;
        ID3D11RasterizerState*  m_RasterizerState       = nullptr;
        ID3D11DepthStencilState* m_DepthStencilState    = nullptr;
        ID3D11SamplerState*     m_Sampler               = nullptr;
        ID3D11Buffer*           m_InstanceBuffer        = nullptr;
        size_t                  m_InstanceBufferSize    = 0;
    };

    bool DirectX11Backend::Init(ID3D11Device* device)
    {
        m_Device = device;
        m_Device->GetImmediateContext(&m_DeviceContext);

        if (!this->CompileVertexShader(g_VertexShaderText))
            return false;

        this->CreateConstantBuffers();

        if (!this->CompilePixelShader(g_PixelShaderText))
            return false;

        this->CreateRenderStates();
        return true;
    }

    void DirectX11Backend::Render()
    {
        HAX_ASSERT(m_Device != nullptr && "API not initialized");

        CopyRenderItems(m_Context.Layers);

        struct BACKUP_DX11_STATE
        {
            UINT                        ScissorRectsCount, ViewportsCount;
            D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
            D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
            ID3D11RasterizerState*      RS;
            ID3D11BlendState*           BlendState;
            FLOAT                       BlendFactor[4];
            UINT                        SampleMask;
            UINT                        StencilRef;
            ID3D11DepthStencilState*    DepthStencilState;
            ID3D11ShaderResourceView*   PSShaderResources[2];
            ID3D11SamplerState*         PSSampler;
            ID3D11PixelShader*          PS;
            ID3D11VertexShader*         VS;
            ID3D11GeometryShader*       GS;
            ID3D11HullShader*           HS;
            ID3D11DomainShader*         DS;
            ID3D11ComputeShader*        CS;
            D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
            ID3D11Buffer*               IndexBuffer, *VertexBuffer;
            ID3D11Buffer*               VSConstantBuffers[2];
            UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
            DXGI_FORMAT                 IndexBufferFormat;
            ID3D11InputLayout*          InputLayout;
        };

        BACKUP_DX11_STATE old = {};
        old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        m_DeviceContext->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
        m_DeviceContext->RSGetViewports(&old.ViewportsCount, old.Viewports);
        m_DeviceContext->RSGetState(&old.RS);
        m_DeviceContext->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
        m_DeviceContext->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
        m_DeviceContext->PSGetShaderResources(0, 2, old.PSShaderResources);
        m_DeviceContext->PSGetSamplers(0, 1, &old.PSSampler);

        UINT numInstances = 0;
        m_DeviceContext->PSGetShader(&old.PS, nullptr, &numInstances); HAX_ASSERT(numInstances == 0);
        m_DeviceContext->VSGetShader(&old.VS, nullptr, &numInstances); HAX_ASSERT(numInstances == 0);
        m_DeviceContext->GSGetShader(&old.GS, nullptr, &numInstances); HAX_ASSERT(numInstances == 0);
        m_DeviceContext->HSGetShader(&old.HS, nullptr, &numInstances); HAX_ASSERT(numInstances == 0);
        m_DeviceContext->DSGetShader(&old.DS, nullptr, &numInstances); HAX_ASSERT(numInstances == 0);
        m_DeviceContext->CSGetShader(&old.CS, nullptr, &numInstances); HAX_ASSERT(numInstances == 0);
        m_DeviceContext->VSGetConstantBuffers(0, 2, old.VSConstantBuffers);

        m_DeviceContext->IAGetPrimitiveTopology(&old.PrimitiveTopology);
        m_DeviceContext->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
        m_DeviceContext->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
        m_DeviceContext->IAGetInputLayout(&old.InputLayout);

        SetupRenderState(m_Context.Viewport.Size);
        RenderLayers(m_Context.Layers);

        m_DeviceContext->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
        m_DeviceContext->RSSetViewports(old.ViewportsCount, old.Viewports);
        m_DeviceContext->RSSetState(old.RS); if (old.RS) old.RS->Release();
        m_DeviceContext->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
        m_DeviceContext->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
        m_DeviceContext->PSSetShaderResources(0, 2, old.PSShaderResources); 
        if (old.PSShaderResources[0]) old.PSShaderResources[0]->Release();
        if (old.PSShaderResources[1]) old.PSShaderResources[1]->Release();
        m_DeviceContext->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
        m_DeviceContext->PSSetShader(old.PS, nullptr, 0); if (old.PS) old.PS->Release();
        m_DeviceContext->VSSetShader(old.VS, nullptr, 0); if (old.VS) old.VS->Release();
        m_DeviceContext->VSSetConstantBuffers(0, 2, old.VSConstantBuffers); 
        if (old.VSConstantBuffers[0]) old.VSConstantBuffers[0]->Release();
        if (old.VSConstantBuffers[1]) old.VSConstantBuffers[1]->Release();
        m_DeviceContext->GSSetShader(old.GS, nullptr, 0); if (old.GS) old.GS->Release();
        m_DeviceContext->HSSetShader(old.HS, nullptr, 0); if (old.HS) old.HS->Release();
        m_DeviceContext->DSSetShader(old.DS, nullptr, 0); if (old.DS) old.DS->Release();
        m_DeviceContext->CSSetShader(old.CS, nullptr, 0); if (old.CS) old.CS->Release();

        m_DeviceContext->IASetPrimitiveTopology(old.PrimitiveTopology);
        m_DeviceContext->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
        m_DeviceContext->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
        m_DeviceContext->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
    }

    Texture2D DirectX11Backend::CreateTexture(const uint8* pixels, int width, int height)
    {
        HAX_ASSERT(m_Device != nullptr && "API not initialized");

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* tex = nullptr;
        D3D11_SUBRESOURCE_DATA subResource{};
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = 4 * (uint32)width;
        subResource.SysMemSlicePitch = 0;
        m_Device->CreateTexture2D(&desc, &subResource, &tex);
        HAX_ASSERT(tex != nullptr);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        ID3D11ShaderResourceView* srv = nullptr;
        m_Device->CreateShaderResourceView(tex, &srvDesc, &srv);
        tex->Release();

        return Texture2D{.Handle = (Handle)srv, .Width = width, .Height = height};
    }

    void DirectX11Backend::DestroyTexture(TexHandle handle)
    {
        HAX_ASSERT(m_Device != nullptr && "API not initialized");

        if (handle)
            ((ID3D11ShaderResourceView*)handle)->Release();
    }

    bool DirectX11Backend::CompileVertexShader(const char* vertexShaderText)
    {
        ID3DBlob* vertexShaderBlob;
        bool failed = FAILED(D3DCompile(vertexShaderText, strlen(vertexShaderText), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vertexShaderBlob, nullptr));
        HAX_ASSERT(!failed && "Unable to compile vertex shader");

        if (m_Device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &m_VertexShader) != S_OK)
        {
            vertexShaderBlob->Release();
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC local_layout[] =
        {
            { "PARAMS",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (uint32)offsetof(RenderItem, Params14), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "PARAMS",   1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (uint32)offsetof(RenderItem, Params58), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "PARAM",   0, DXGI_FORMAT_R32_FLOAT, 0, (uint32)offsetof(RenderItem, Param9), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TYPE",     0, DXGI_FORMAT_R32_UINT,  0, (uint32)offsetof(RenderItem, Type), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR",   0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (uint32)offsetof(RenderItem, Color1), D3D11_INPUT_PER_INSTANCE_DATA, 1},
            { "COLOR",   1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (uint32)offsetof(RenderItem, Color2), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR",   2, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (uint32)offsetof(RenderItem, Color3), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR",   3, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (uint32)offsetof(RenderItem, Color4), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "SINCOS",   0, DXGI_FORMAT_R32G32_FLOAT, 0, (uint32)offsetof(RenderItem, Sin), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };
        HRESULT res = m_Device->CreateInputLayout(local_layout, _countof(local_layout), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &m_InputLayout);
        HAX_ASSERT(res == S_OK && "Unable to create input layout");

        vertexShaderBlob->Release();
        return true;
    }

    bool DirectX11Backend::CompilePixelShader(const char* pixelShaderText)
    {
        ID3DBlob* pixelShaderBlob;
        HRESULT res = D3DCompile(pixelShaderText, strlen(pixelShaderText), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &pixelShaderBlob, nullptr);
        HAX_ASSERT(res == S_OK && "Unable to compile pixel shader");

        if (m_Device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &m_PixelShader) != S_OK)
        {
            pixelShaderBlob->Release();
            return false;
        }
        pixelShaderBlob->Release();
        return true;
    }

    struct VERTEX_CONSTANT_BUFFER_DX11
    {
        float   mvp[4][4];
    };

    void DirectX11Backend::CreateConstantBuffers()
    {
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER_DX11);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            HRESULT res = m_Device->CreateBuffer(&desc, nullptr, &m_MatrixContantBuffer);
            HAX_ASSERT(res == S_OK);
        }
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(FontConstData);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            HRESULT res = m_Device->CreateBuffer(&desc, nullptr, &m_FontConstantBuffer);
            HAX_ASSERT(res == S_OK);
        }
    }

    void DirectX11Backend::CreateRenderStates()
    {
        {
            D3D11_BLEND_DESC desc{};
            desc.AlphaToCoverageEnable = false;
            desc.RenderTarget[0].BlendEnable = true;
            desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

            desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

            desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            HRESULT res = m_Device->CreateBlendState(&desc, &m_BlendState);
            HAX_ASSERT(res == S_OK);
        }

        // Create the rasterizer state
        {
            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            desc.ScissorEnable = true;
            desc.DepthClipEnable = true;
            HRESULT res = m_Device->CreateRasterizerState(&desc, &m_RasterizerState);
            HAX_ASSERT(res == S_OK);
        }

        // Create depth-stencil State
        {
            D3D11_DEPTH_STENCIL_DESC desc{};
            desc.DepthEnable = false;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
            desc.StencilEnable = false;
            desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            desc.BackFace = desc.FrontFace;
            HRESULT res = m_Device->CreateDepthStencilState(&desc, &m_DepthStencilState);
            HAX_ASSERT(res == S_OK);
        }

        {
            D3D11_SAMPLER_DESC desc{};
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.MipLODBias = 0.f;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.MinLOD = 0.f;
            desc.MaxLOD = 0.f;
            HRESULT res = m_Device->CreateSamplerState(&desc, &m_Sampler);
            HAX_ASSERT(res == S_OK);
        }
    }

    void DirectX11Backend::SetupRenderState(const Vector2& viewportSize)
    {
        D3D11_VIEWPORT vp = {};
        vp.Width = viewportSize.X;
        vp.Height = viewportSize.Y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0;
        m_DeviceContext->RSSetViewports(1, &vp);

        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        if (m_DeviceContext->Map(m_MatrixContantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) == S_OK)
        {
            VERTEX_CONSTANT_BUFFER_DX11* constant_buffer = (VERTEX_CONSTANT_BUFFER_DX11*)mapped_resource.pData;
            float L = 0;
            float R = viewportSize.X;
            float T = 0;
            float B = viewportSize.Y;
            float mvp[4][4] =
            {
                { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
                { 0.0f,         0.0f,           0.5f,       0.0f },
                { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
            };
            memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
            m_DeviceContext->Unmap(m_MatrixContantBuffer, 0);
        }

        unsigned int stride = sizeof(RenderItem);
        unsigned int offset = 0;
        m_DeviceContext->IASetInputLayout(m_InputLayout);
        m_DeviceContext->IASetVertexBuffers(0, 1, &m_InstanceBuffer, &stride, &offset);
        m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_DeviceContext->VSSetShader(m_VertexShader, nullptr, 0);
        m_DeviceContext->VSSetConstantBuffers(0, 1, &m_MatrixContantBuffer);
        m_DeviceContext->PSSetShader(m_PixelShader, nullptr, 0);
        m_DeviceContext->PSSetSamplers(0, 1, &m_Sampler);
        m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
        m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
        m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
        m_DeviceContext->CSSetShader(nullptr, nullptr, 0);

        const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
        m_DeviceContext->OMSetBlendState(m_BlendState, blend_factor, 0xffffffff);
        m_DeviceContext->OMSetDepthStencilState(m_DepthStencilState, 0);
        m_DeviceContext->RSSetState(m_RasterizerState);
    }

    void DirectX11Backend::CopyRenderItems(Vector<Layer*>& layers)
    {
        size_t totalItems = 0;
        for (Layer* layer : layers)
            totalItems += layer->RenderItems.Size();

        if (!m_InstanceBuffer || m_InstanceBufferSize < totalItems)
        {
            if (m_InstanceBuffer) 
            { 
                m_InstanceBuffer->Release();
                m_InstanceBuffer = nullptr;
            }

            m_InstanceBufferSize = totalItems + 100;

            D3D11_BUFFER_DESC desc{};
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = (uint32)m_InstanceBufferSize * (uint32)sizeof(RenderItem);
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (m_Device->CreateBuffer(&desc, nullptr, &m_InstanceBuffer) < 0)
                return;

            printf("Instance buffer resized to %zu\n", m_InstanceBufferSize);
        }

        D3D11_MAPPED_SUBRESOURCE resource;
        HRESULT res = m_DeviceContext->Map(m_InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        HAX_ASSERT(res == S_OK);

        size_t offset = 0;
        for (Layer* layer : layers)
        {
            memcpy((uint8*)resource.pData + offset, layer->RenderItems.Data(), layer->RenderItems.Size() * sizeof(RenderItem));
            offset += layer->RenderItems.Size() * sizeof(RenderItem);
        }
        m_DeviceContext->Unmap(m_InstanceBuffer, 0);
    }

    void DirectX11Backend::RenderLayers(Vector<Layer*>& layers)
    {
        size_t instanceOffset = 0;
        for (Layer* layer : layers)
        {
            for (RenderBatch& batch : layer->RenderBatches)
            {
                if (batch.ActionMask & RenderBatchAction_SetClipRect)
                {
                    Rect clipRect = batch.ClipRect.Value();
                    RECT rect
                    {
                        .left =   (LONG)clipRect.Min.X,
                        .top =    (LONG)clipRect.Min.Y,
                        .right =  (LONG)clipRect.Max.X,
                        .bottom = (LONG)clipRect.Max.Y
                    };
                    m_DeviceContext->RSSetScissorRects(1, &rect);
                }

                if (batch.ActionMask & RenderBatchAction_SetFont)
                {
                    D3D11_MAPPED_SUBRESOURCE mapped_resource;
                    if (m_DeviceContext->Map(m_FontConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) == S_OK)
                    {
                        FontConstData* data = (FontConstData*)mapped_resource.pData;
                        data->PxRange = batch.Font->DistanceRange;
                        data->TexWidth = (float)batch.Font->Atlas.Width;
                        data->TexHeight = (float)batch.Font->Atlas.Height;
                        m_DeviceContext->Unmap(m_FontConstantBuffer, 0);
                    }
                    m_DeviceContext->VSSetConstantBuffers(1, 1, &m_FontConstantBuffer);
                    m_DeviceContext->PSSetShaderResources(1, 1, (ID3D11ShaderResourceView**)&batch.Font->Atlas.Handle);
                }

                if (batch.ActionMask & RenderBatchAction_SetTexture)
                {
                    m_DeviceContext->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&batch.Texture);
                }

                m_DeviceContext->DrawInstanced(6, (uint32)batch.InstancesNum, 0, (uint32)instanceOffset);
                instanceOffset += batch.InstancesNum;
            }
        }
    }

    bool DirectX11_Init(ID3D11Device* device)
    {
        DirectX11Backend* graphicsApi = g_Context->GeneralAlloc.New<DirectX11Backend>(*g_Context);
        g_Context->Backend = graphicsApi;

        return graphicsApi->Init(device);
    }

    const char* g_VertexShaderText = R"(
cbuffer vertexBuffer : register(b0)
{
    float4x4 ProjectionMatrix;
};

cbuffer fontBuffer : register(b1)
{
    float PxRange;
    float2 TexSize;
};

struct VS_INPUT      
{
    float4 params14 : PARAMS0;
    float4 params58 : PARAMS1;
    float param9 : PARAM0;

    float4 color1 : COLOR0;
    float4 color2 : COLOR1;
    float4 color3 : COLOR2;
    float4 color4 : COLOR3;

    float2 sincos: SINCOS0;
    uint type : TYPE0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;

    float4 params14 : PARAMS0;
    float4 params58 : PARAMS1;
    float param9 : PARAM0;

    float4 color1 : COLOR0;
    float4 color2 : COLOR1;
    float4 color3 : COLOR2;
    float4 color4 : COLOR3;

    float2 pixPos : TEXCOORD0;
    float2 sincos: SINCOS0;
    uint type : TYPE0;
};

float2 rotate(float2 p, float2 c, float2 sc)
{
    float2 d = p - c;
    return c + float2(
        d.x * sc.y + d.y * sc.x,
       -d.x * sc.x + d.y * sc.y
    );
}

float2 calcEllipse(VS_INPUT input, uint vertId)
{
    float2 center = input.params14.xy;
    float2 r = input.params14.zw + 2.0;

    float2 tl = rotate(center - r, center, input.sincos);
    float2 tr = rotate(float2(center.x + r.x, center.y - r.y), center, input.sincos);
    float2 br = rotate(center + r, center, input.sincos);
    float2 bl = rotate(float2(center.x - r.x, center.y + r.y), center, input.sincos);

    float2 rotated[6] = { tl, tr, bl, bl, tr, br };
    return rotated[vertId];
}

float2 calcRect(VS_INPUT input, uint vertId)
{
    float2 tl = input.params14.xy - 2.0;
    float2 br = input.params14.zw + 2.0;

    float2 center = (tl + br) * 0.5;
    float2 tr = rotate(float2(br.x, tl.y), center, input.sincos);
    float2 bl = rotate(float2(tl.x, br.y), center, input.sincos);
    tl = rotate(tl, center, input.sincos);
    br = rotate(br, center, input.sincos);

    float2 rotated[6] = { tl, tr, bl, bl, tr, br };
    return rotated[vertId];
}

float2 calcTriangle(VS_INPUT input, uint vertId)
{
    if (vertId > 2) return float2(0,0);

    float2 a = input.params14.xy;
    float2 b = input.params14.zw;
    float2 c = input.params58.xy;

    float2 center = (a + b + c) / 3.0;
    a = rotate(a, center, input.sincos);
    b = rotate(b, center, input.sincos);
    c = rotate(c, center, input.sincos);

    float2 rotated[3] = { a, b, c };
    return rotated[vertId];
}

float2 calcLine(VS_INPUT input, uint vertId)
{
    float2 a = input.params14.xy;
    float2 b = input.params14.zw;
    float th = input.params58.x;

    float2 center = (a + b) / 2.0;
    a = rotate(a, center, input.sincos);
    b = rotate(b, center, input.sincos);

    float2 dir = normalize(b - a);
    float2 normal = float2(-dir.y, dir.x) * ((th + 2.0) / 2.0);

    float2 tl = a - normal;
    float2 bl = a + normal;
    float2 tr = b - normal;
    float2 br = b + normal;

    float2 rotated[6] = { tl, tr, bl, bl, tr, br };
    return rotated[vertId];
}

VS_OUTPUT main(VS_INPUT input, uint vertId : SV_VertexID)
{
    input.sincos.x = -input.sincos.x;

    VS_OUTPUT output;

    switch (input.type)
    {
        case 0: output.pixPos = calcEllipse(input, vertId); break;
        case 5:
        {
            float2 vert_to_uv[6] = { {0,0}, {1,0}, {0,1}, {0,1}, {1,0}, {1,1} };
    
            float2 uvMin = input.params58.xy;
            float2 uvMax = input.params58.zw;
    
            input.params58.xy = lerp(uvMin, uvMax, vert_to_uv[vertId]);
            input.params14.xy += 2.0;
            input.params14.zw -= 2.0;
            output.pixPos = calcRect(input, vertId);
            input.params14.xy = TexSize;
            input.params14.z = PxRange;
            break;
        }
        case 4:
        {
            float2 vert_to_uv[6] = { {0,0}, {1,0}, {0,1}, {0,1}, {1,0}, {1,1} };
    
            float2 uvMin = input.params58.xy;
            float2 uvMax = input.params58.zw;
    
            input.params58.xy = lerp(uvMin, uvMax, vert_to_uv[vertId]);
            input.params14.xy += 2.0;
            input.params14.zw -= 2.0;
            output.pixPos = calcRect(input, vertId);
            break;
        }
        case 1: 
            float2 a = input.params14.xy; float2 b = input.params14.zw;
            input.params14.xy = min(a,b); input.params14.zw = max(a,b);
            output.pixPos = calcRect(input, vertId); break;
        case 2: output.pixPos = calcTriangle(input, vertId); break;
        case 3: output.pixPos = calcLine(input, vertId); break;
    }

    output.params14 = input.params14;
    output.params58 = input.params58;
    output.param9 = input.param9;
    output.color1 = input.color1;
    output.color2 = input.color2;
    output.color3 = input.color3;
    output.color4 = input.color4;
    output.sincos = input.sincos;
    output.type = input.type;
    output.pos = mul(ProjectionMatrix, float4(output.pixPos, 0.f, 1.f));

    return output;
}
)";

    const char* g_PixelShaderText = R"(
struct PS_INPUT
{
    float4 pos : SV_POSITION;

    float4 params14 : PARAMS0;
    float4 params58 : PARAMS1;
    float param9 : PARAM0;

    float4 color1 : COLOR0;
    float4 color2 : COLOR1;
    float4 color3 : COLOR2;
    float4 color4 : COLOR3;

    float2 pixPos : TEXCOORD0;
    float2 sincos: SINCOS;
    uint type : TYPE0;
};

float2 fastMap(float2 p, float2 center, float2 sincos) 
{
    float2 d = p - center;
    return center + float2(
        d.x * sincos.y + d.y * sincos.x,
       -d.x * sincos.x + d.y * sincos.y
    );
}

float sdEllipse(float2 p, float2 center, float2 r)
{
    p -= center;
    float k1 = length(p / r);
    float k2 = length(p / (r * r));
    return k1 * (k1 - 1.0) / k2;
}

float4 shaderEllipse(PS_INPUT input)
{
    float2 p      = input.pixPos;
    float2 center = input.params14.xy;
    float2 r      = input.params14.zw;
    float  th     = input.params58.x;

    float4 fillColor  = input.color1;
    float4 frameColor = input.color2;

    float2 pRot = fastMap(p, center, input.sincos);

    float d  = sdEllipse(pRot, center, r);
    float aa = fwidth(d) * 0.7;

    float outerMask = smoothstep(aa, -aa, d);
    float innerMask = smoothstep(aa, -aa, d + th);

    float4 finalColor = lerp(frameColor, fillColor, innerMask);

    finalColor.a *= outerMask;
    finalColor.rgb *= finalColor.a;

    return finalColor;
}

float sdRect(float2 p, float2 a, float2 b, float4 r)
{
    float2 halfSize = (b - a) * 0.5;
    float2 center = a + halfSize;
    p -= center;

    float2 s = step(0.0, p);
    float rad = lerp(lerp(r.x, r.w, s.x), lerp(r.y, r.z, s.x), s.y);

    float2 d = abs(p) - halfSize + rad;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - rad;
}

float4 shaderRect(PS_INPUT input)
{
    float2 p = input.pixPos;
    float2 a = input.params14.xy;
    float2 b = input.params14.zw;
    float4 r = input.params58;
    float th = input.param9;

    float4 fillColor  = input.color1;
    float4 frameColor = input.color2;

    float2 halfSize = abs(b - a) * 0.5;
    float2 center   = a + halfSize;
    float2 pRot     = fastMap(p, center, input.sincos);

    float d  = sdRect(pRot, a, b, r);
    float aa = fwidth(d) * 0.7;

    float maskOuter = smoothstep(aa, -aa, d);
    float maskInner = smoothstep(aa, -aa, d + th);

    float4 finalColor = lerp(frameColor, fillColor, maskInner);

    finalColor.a *= maskOuter;
    finalColor.rgb *= finalColor.a;

    return finalColor;
}

float sdTriangle(float2 p, float2 a, float2 b, float2 c)
{
    float2 e0 = b-a, e1 = c-b, e2 = a-c;
    float2 v0 = p -a, v1 = p -b, v2 = p -c;
    float2 pq0 = v0 - e0*clamp( dot(v0,e0)/dot(e0,e0), 0.0, 1.0 );
    float2 pq1 = v1 - e1*clamp( dot(v1,e1)/dot(e1,e1), 0.0, 1.0 );
    float2 pq2 = v2 - e2*clamp( dot(v2,e2)/dot(e2,e2), 0.0, 1.0 );
    float s = sign( e0.x*e2.y - e0.y*e2.x );
    float2 d = min(min(float2(dot(pq0,pq0), s*(v0.x*e0.y-v0.y*e0.x)),
                     float2(dot(pq1,pq1), s*(v1.x*e1.y-v1.y*e1.x))),
                     float2(dot(pq2,pq2), s*(v2.x*e2.y-v2.y*e2.x)));
    return -sqrt(d.x)*sign(d.y);
}

float4 shaderTriangle(PS_INPUT input)
{
    float2 p = input.pixPos;

    float2 a = input.params14.xy;
    float2 b = input.params14.zw;
    float2 c = input.params58.xy;
    float th = input.params58.z;

    float4 fillColor  = input.color1;
    float4 frameColor = input.color2;

    float2 center = (a + b + c) / 3.0;
    float2 pRot   = fastMap(p, center, input.sincos);

    float d  = sdTriangle(pRot, a, b, c);
    float aa = fwidth(d) * 0.7;

    float outerMask = smoothstep(aa, -aa, d);
    float innerMask = smoothstep(aa, -aa, d + th);

    float4 finalColor = lerp(frameColor, fillColor, innerMask);
    
    finalColor.a *= outerMask;
    finalColor.rgb *= finalColor.a;

    return finalColor;
}

float sdSegment(float2 p, float2 a, float2 b)
{
    float2 pa = p - a;
    float2 ba = b - a;
    float h = saturate(dot(pa, ba) / dot(ba, ba));
    return length(pa - ba * h);
}

float4 shaderLine(PS_INPUT input)
{
    float2 p = input.pixPos;

    float2 a  = input.params14.xy;
    float2 b  = input.params14.zw;
    float  th = input.params58.x;

    float4 fillColor = input.color1;

    float2 center = (a + b) * 0.5;
    float2 pRot   = fastMap(p, center, input.sincos);

    float d = sdSegment(pRot, a, b) - th * 0.5;

    float aa = fwidth(d) * 0.5;

    float coverage = smoothstep(aa, -aa, d);

    float4 finalColor = fillColor;
    finalColor.a *= coverage;
    finalColor.rgb *= finalColor.a;

    return finalColor;
}

sampler sampler0 : register(s0);
Texture2D texture0 : register(t0);
Texture2D fontTexture : register(t1);
float4 shaderImage(PS_INPUT input)
{
    float2 p = input.pixPos;
    float2 a = input.params14.xy;
    float2 b = input.params14.zw;
    float4 r = float4(input.param9, input.param9, input.param9, input.param9);

    float4 src = texture0.Sample(sampler0, input.params58.xy);
    
    float2 halfSize = abs(b - a) * 0.5;
    float2 center = a + halfSize;
    float2 pRot = fastMap(p, center, input.sincos);
    
    float d = sdRect(pRot, a, b, r);
    float aa = fwidth(d) * 0.7;

    float mask = smoothstep(aa, -aa, d);

    float4 finalColor = src * input.color1;
    finalColor.a *= mask;
    finalColor.rgb *= finalColor.a;

    return finalColor;
}

float4 shaderGlyph(PS_INPUT input)
{
    float2 uv = input.params58.xy;
    float pxRange = input.params14.z;
    float2 texSize = input.params14.xy;
    
    float3 msd = fontTexture.Sample(sampler0, uv).rgb; 
    
    float2 dest = 1.0 / fwidth(uv); 
    float px_size = max(0.5 * dot((pxRange / texSize), dest), 1.0);
    
    float d = max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));
    
    float a = clamp((d - 0.5) * px_size + 0.5, 0.0, 1.0);

    float4 finalColor = input.color1;
    finalColor.a *= a;
    finalColor.rgb *= finalColor.a;

    return finalColor;
}

float4 main(PS_INPUT input) : SV_Target
{    
    input.sincos.x = -input.sincos.x;
    switch (input.type)
    {
        case 0: return shaderEllipse(input);
        case 1: return shaderRect(input);
        case 2: return shaderTriangle(input);
        case 3: return shaderLine(input);
        case 4: return shaderImage(input);
        case 5: return shaderGlyph(input);
    }
    return float4(0,0,0,0);
}
)";
}