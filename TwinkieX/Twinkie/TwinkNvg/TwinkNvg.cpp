#include "pch.h"
#include <Twinkie/TwinkNvg/TwinkNvg.h>
#include <Twinkie/TwinkUi/TwinkUi.h>

#ifdef MANIAPLANET
#include <nanovg/src/nanovg_d3d11.h>
#else
#include <nanovg/src/nanovg_d3d9.h>
#endif

namespace TwinkNvg
{
	NVGcontext* Context = nullptr;
	uint32_t Generation = 0;

	static bool InFrame = false;
	static float FrameWidth = 0.0f;
	static float FrameHeight = 0.0f;

#ifdef MANIAPLANET
	static ID3D11Texture2D* DepthTexture = nullptr;
	static ID3D11DepthStencilView* DepthView = nullptr;
	static D3D11_TEXTURE2D_DESC DepthFor = {};

	static void ReleaseDepth()
	{
		if (DepthView) DepthView->Release();
		if (DepthTexture) DepthTexture->Release();
		DepthView = nullptr;
		DepthTexture = nullptr;
		DepthFor = {};
	}

	static bool EnsureDepth(ID3D11RenderTargetView* Target)
	{
		ID3D11Resource* Resource = nullptr;
		Target->GetResource(&Resource);
		if (!Resource) return false;

		ID3D11Texture2D* Texture = nullptr;
		Resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&Texture);
		Resource->Release();
		if (!Texture) return false;

		D3D11_TEXTURE2D_DESC TargetDesc;
		Texture->GetDesc(&TargetDesc);
		Texture->Release();

		if (DepthView and DepthFor.Width == TargetDesc.Width and DepthFor.Height == TargetDesc.Height
			and DepthFor.SampleDesc.Count == TargetDesc.SampleDesc.Count and DepthFor.SampleDesc.Quality == TargetDesc.SampleDesc.Quality)
		{
			return true;
		}

		ReleaseDepth();

		D3D11_TEXTURE2D_DESC Desc = {};
		Desc.Width = TargetDesc.Width;
		Desc.Height = TargetDesc.Height;
		Desc.MipLevels = 1;
		Desc.ArraySize = 1;
		Desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		Desc.SampleDesc = TargetDesc.SampleDesc;
		Desc.Usage = D3D11_USAGE_DEFAULT;
		Desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		if (FAILED(TwinkUiState::Device->CreateTexture2D(&Desc, nullptr, &DepthTexture))) return false;

		D3D11_DEPTH_STENCIL_VIEW_DESC ViewDesc = {};
		ViewDesc.Format = Desc.Format;
		ViewDesc.ViewDimension = Desc.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		if (FAILED(TwinkUiState::Device->CreateDepthStencilView(DepthTexture, &ViewDesc, &DepthView)))
		{
			ReleaseDepth();
			return false;
		}

		DepthFor = TargetDesc;
		return true;
	}

	struct StateBackup
	{
		ID3D11RenderTargetView* RenderTarget;
		ID3D11DepthStencilView* DepthStencil;
		UINT ScissorRectsCount, ViewportsCount;
		D3D11_RECT ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		ID3D11RasterizerState* RS;
		ID3D11BlendState* BlendState;
		FLOAT BlendFactor[4];
		UINT SampleMask;
		UINT StencilRef;
		ID3D11DepthStencilState* DepthStencilState;
		ID3D11ShaderResourceView* PSShaderResource;
		ID3D11SamplerState* PSSampler;
		ID3D11PixelShader* PS;
		ID3D11VertexShader* VS;
		ID3D11GeometryShader* GS;
		ID3D11HullShader* HS;
		ID3D11DomainShader* DS;
		UINT PSInstancesCount, VSInstancesCount, GSInstancesCount, HSInstancesCount, DSInstancesCount;
		ID3D11ClassInstance* PSInstances[256], * VSInstances[256], * GSInstances[256], * HSInstances[256], * DSInstances[256];
		D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
		ID3D11Buffer* IndexBuffer, * VertexBuffer, * VSConstantBuffer, * PSConstantBuffer;
		UINT IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
		DXGI_FORMAT IndexBufferFormat;
		ID3D11InputLayout* InputLayout;

		void Capture(ID3D11DeviceContext* Ctx)
		{
			Ctx->OMGetRenderTargets(1, &RenderTarget, &DepthStencil);
			ScissorRectsCount = ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
			Ctx->RSGetScissorRects(&ScissorRectsCount, ScissorRects);
			Ctx->RSGetViewports(&ViewportsCount, Viewports);
			Ctx->RSGetState(&RS);
			Ctx->OMGetBlendState(&BlendState, BlendFactor, &SampleMask);
			Ctx->OMGetDepthStencilState(&DepthStencilState, &StencilRef);
			Ctx->PSGetShaderResources(0, 1, &PSShaderResource);
			Ctx->PSGetSamplers(0, 1, &PSSampler);
			PSInstancesCount = VSInstancesCount = GSInstancesCount = HSInstancesCount = DSInstancesCount = 256;
			Ctx->PSGetShader(&PS, PSInstances, &PSInstancesCount);
			Ctx->VSGetShader(&VS, VSInstances, &VSInstancesCount);
			Ctx->GSGetShader(&GS, GSInstances, &GSInstancesCount);
			Ctx->HSGetShader(&HS, HSInstances, &HSInstancesCount);
			Ctx->DSGetShader(&DS, DSInstances, &DSInstancesCount);
			Ctx->VSGetConstantBuffers(0, 1, &VSConstantBuffer);
			Ctx->PSGetConstantBuffers(0, 1, &PSConstantBuffer);
			Ctx->IAGetPrimitiveTopology(&PrimitiveTopology);
			Ctx->IAGetIndexBuffer(&IndexBuffer, &IndexBufferFormat, &IndexBufferOffset);
			Ctx->IAGetVertexBuffers(0, 1, &VertexBuffer, &VertexBufferStride, &VertexBufferOffset);
			Ctx->IAGetInputLayout(&InputLayout);
		}

		template<typename T>
		static void Release(T*& Object)
		{
			if (Object) Object->Release();
			Object = nullptr;
		}

		static void ReleaseInstances(ID3D11ClassInstance** Instances, UINT Count)
		{
			for (UINT Idx = 0; Idx < Count; Idx++)
			{
				if (Instances[Idx]) Instances[Idx]->Release();
			}
		}

		void Restore(ID3D11DeviceContext* Ctx)
		{
			Ctx->OMSetRenderTargets(1, &RenderTarget, DepthStencil);
			Ctx->RSSetScissorRects(ScissorRectsCount, ScissorRects);
			Ctx->RSSetViewports(ViewportsCount, Viewports);
			Ctx->RSSetState(RS);
			Ctx->OMSetBlendState(BlendState, BlendFactor, SampleMask);
			Ctx->OMSetDepthStencilState(DepthStencilState, StencilRef);
			Ctx->PSSetShaderResources(0, 1, &PSShaderResource);
			Ctx->PSSetSamplers(0, 1, &PSSampler);
			Ctx->PSSetShader(PS, PSInstances, PSInstancesCount);
			Ctx->VSSetShader(VS, VSInstances, VSInstancesCount);
			Ctx->GSSetShader(GS, GSInstances, GSInstancesCount);
			Ctx->HSSetShader(HS, HSInstances, HSInstancesCount);
			Ctx->DSSetShader(DS, DSInstances, DSInstancesCount);
			Ctx->VSSetConstantBuffers(0, 1, &VSConstantBuffer);
			Ctx->PSSetConstantBuffers(0, 1, &PSConstantBuffer);
			Ctx->IASetPrimitiveTopology(PrimitiveTopology);
			Ctx->IASetIndexBuffer(IndexBuffer, IndexBufferFormat, IndexBufferOffset);
			Ctx->IASetVertexBuffers(0, 1, &VertexBuffer, &VertexBufferStride, &VertexBufferOffset);
			Ctx->IASetInputLayout(InputLayout);

			Release(RenderTarget);
			Release(DepthStencil);
			Release(RS);
			Release(BlendState);
			Release(DepthStencilState);
			Release(PSShaderResource);
			Release(PSSampler);
			Release(PS);
			Release(VS);
			Release(GS);
			Release(HS);
			Release(DS);
			ReleaseInstances(PSInstances, PSInstancesCount);
			ReleaseInstances(VSInstances, VSInstancesCount);
			ReleaseInstances(GSInstances, GSInstancesCount);
			ReleaseInstances(HSInstances, HSInstancesCount);
			ReleaseInstances(DSInstances, DSInstancesCount);
			Release(VSConstantBuffer);
			Release(PSConstantBuffer);
			Release(IndexBuffer);
			Release(VertexBuffer);
			Release(InputLayout);
		}
	};
#else
	static IDirect3DSurface9* DepthSurface = nullptr;
	static D3DSURFACE_DESC DepthFor = {};

	static void ReleaseDepth()
	{
		if (DepthSurface) DepthSurface->Release();
		DepthSurface = nullptr;
		DepthFor = {};
	}

	static bool EnsureDepth(IDirect3DSurface9* Target)
	{
		D3DSURFACE_DESC TargetDesc;
		if (FAILED(Target->GetDesc(&TargetDesc))) return false;

		if (DepthSurface and DepthFor.Width == TargetDesc.Width and DepthFor.Height == TargetDesc.Height
			and DepthFor.MultiSampleType == TargetDesc.MultiSampleType and DepthFor.MultiSampleQuality == TargetDesc.MultiSampleQuality)
		{
			return true;
		}

		ReleaseDepth();

		if (FAILED(TwinkUiState::Device->CreateDepthStencilSurface(TargetDesc.Width, TargetDesc.Height, D3DFMT_D24S8,
			TargetDesc.MultiSampleType, TargetDesc.MultiSampleQuality, TRUE, &DepthSurface, nullptr)))
		{
			DepthSurface = nullptr;
			return false;
		}

		DepthFor = TargetDesc;
		return true;
	}
#endif

	bool Init()
	{
		if (Context) return true;
		if (!TwinkUiState::Device) return false;

#ifdef MANIAPLANET
		Context = nvgCreateD3D11(TwinkUiState::Device, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#else
		Context = nvgCreateD3D9(TwinkUiState::Device, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif
		if (Context) Generation++;
		return Context != nullptr;
	}

	bool IsInFrame()
	{
		return Context and InFrame;
	}

	void Invalidate()
	{
		if (InFrame and Context) nvgCancelFrame(Context);
		InFrame = false;

		if (Context)
		{
#ifdef MANIAPLANET
			nvgDeleteD3D11(Context);
#else
			nvgDeleteD3D9(Context);
#endif
			Context = nullptr;
		}

		ReleaseDepth();
	}

	void BeginFrame(float Width, float Height)
	{
		if (!Context or InFrame or Width <= 0.0f or Height <= 0.0f) return;

		FrameWidth = Width;
		FrameHeight = Height;
		nvgBeginFrame(Context, Width, Height, 1.0f);
		InFrame = true;
	}

	void EndFrame()
	{
		if (!Context or !InFrame) return;
		InFrame = false;

#ifdef MANIAPLANET
		ID3D11DeviceContext* Ctx = TwinkUiState::Context;
		ID3D11RenderTargetView* Target = TwinkUiState::MainRenderTargetView;
		if (!Target or !EnsureDepth(Target))
		{
			nvgCancelFrame(Context);
			return;
		}

		StateBackup Backup = {};
		Backup.Capture(Ctx);

		Ctx->OMSetRenderTargets(1, &Target, DepthView);
		Ctx->ClearDepthStencilView(DepthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		D3D11_VIEWPORT Viewport = { 0.0f, 0.0f, FrameWidth, FrameHeight, 0.0f, 1.0f };
		Ctx->RSSetViewports(1, &Viewport);
		Ctx->GSSetShader(nullptr, nullptr, 0);
		Ctx->HSSetShader(nullptr, nullptr, 0);
		Ctx->DSSetShader(nullptr, nullptr, 0);

		nvgEndFrame(Context);

		Backup.Restore(Ctx);
#else
		IDirect3DDevice9* Device = TwinkUiState::Device;

		IDirect3DSurface9* Target = nullptr;
		if (FAILED(Device->GetRenderTarget(0, &Target)) or !Target)
		{
			nvgCancelFrame(Context);
			return;
		}

		bool HasDepth = EnsureDepth(Target);
		Target->Release();
		if (!HasDepth)
		{
			nvgCancelFrame(Context);
			return;
		}

		IDirect3DSurface9* OldDepth = nullptr;
		Device->GetDepthStencilSurface(&OldDepth);

		D3DVIEWPORT9 Viewport = { 0, 0, (DWORD)FrameWidth, (DWORD)FrameHeight, 0.0f, 1.0f };
		Device->SetViewport(&Viewport);

		Device->SetDepthStencilSurface(DepthSurface);
		Device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 1.0f, 0);

		nvgEndFrame(Context);

		Device->SetDepthStencilSurface(OldDepth);
		if (OldDepth) OldDepth->Release();
#endif
	}
}
