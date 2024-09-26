#include "Engine/Manager/RenderManager.h"

#include "Engine/Manager/ResourceManager.h"
#include "Engine/Manager/TimeManager.h"


#include "Engine/GameEngine.h"

#include "Engine/Resource/Mesh/Mesh.h"
#include "Engine/Resource/Material/Material.h"
#include "Engine/Resource/Texture.h"
#include "Engine/Resource/Shader/ComputeShaders/GPUInitSetting.h"

#include "Engine/Util/AtExit.h"

#include "Engine/GPU/ConstBuffer.h"
#include "Engine/GPU/StructBuffer.h"
#include "Engine/GPU/MultiRenderTarget.h"


namespace ehw
{
	RenderManager::RenderManager()
		: m_resolution_x{}
		, m_resolution_y{}
		, m_refreshRate{}
		, m_device{}
		, m_context{}
		, m_swapChain{}
		, m_renderTarget_texture{}
		, m_depth_stencil_buffer_texture{}
		, m_viewport{}
		, m_multi_render_targets{}
		, m_constBuffers{}
		, m_samplerStates{}
		, m_rasterizerStates{}
		, m_depthStencilStates{}
		, m_blendStates{}
		, m_postProcessTexture{}
		, m_sceneRenderAgent{}
	{
	}
	RenderManager::~RenderManager()
	{
	}

	bool RenderManager::init()
	{
		AtExit::AddFunc(std::bind(&RenderManager::release, this));

		// Device, Device Context
		uint DeviceFlag{};
#ifdef _DEBUG
		DeviceFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

		D3D_FEATURE_LEVEL MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_11_1;
		D3D_FEATURE_LEVEL FeatureLevels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};

		if (FAILED(D3D11CreateDevice(
			nullptr, 
			D3D_DRIVER_TYPE_HARDWARE, 
			nullptr, 
			DeviceFlag, 
			FeatureLevels,
			sizeof(FeatureLevels) / sizeof(D3D_FEATURE_LEVEL),
			D3D11_SDK_VERSION, 
			m_device.ReleaseAndGetAddressOf(), 
			&MaxSupportedFeatureLevel,
			m_context.ReleaseAndGetAddressOf())))
		{
			m_device.Reset();
			m_context.Reset();
			ERROR_MESSAGE("Graphics Device 생성에 실패했습니다.");
			return false;
		}

		return true;
	}

	bool RenderManager::Settings(const tGPUManagerDesc& _Desc)
	{
		if (nullptr == m_device || nullptr == m_context) {
			ASSERT(false, "Device Context가 생성되지 않았습니다. init()를 먼저 호출하세요.");
			return false;
		}

		AtExit::AddFunc(std::bind(&RenderManager::ReleaseResources, this));

		/// 1. Device 와 SwapChain 생성한다.
		/// 2. 백버퍼에 실제로 렌더링할 렌더타겟 뷰를 생성해야한다.
		/// 3. 화면을 클리어 해줘야한다. 뷰포트를 생성해줘야 한다.
		/// 4. 매프레임마다 위에서 생성한 렌더타겟뷰에 렌더링해주어야한다.
		/// 5. Swapchain을 이용하여 최종 디바이스(디스플레이)에 화면을 그려줘야한다.
		
		CreateSamplerStates();
		CreateRasterizerStates();
		CreateDepthStencilStates();
		CreateBlendStates();
		create_const_buffers();

		//Resource
		load_default_resources();

		std::shared_ptr<GPUInitSetting> initSetting = ResourceManager<ComputeShader>::get_inst().load<GPUInitSetting>(name::defaultRes::shader::compute::GPUInitSetting);
		initSetting->on_execute();

		m_sceneRenderAgent.init();

		m_resolution_x = _Desc.ResolutionX;
		m_resolution_y = _Desc.ResolutionY;
		m_refreshRate = _Desc.RefreshRate;

		if (false == SetResolution(m_resolution_x, m_resolution_y))
		{
			ASSERT(false, "GPU 초기화 작업 실패");
			return false;
		}

		CreateMainViewPort();

		return true;
	}

	void RenderManager::load_default_resources()
	{
		LoadDefaultMesh();
		load_default_shaders();
		LoadDefaultTexture();
		LoadDefaultMaterial();
	}

	void RenderManager::ReleaseResources()
	{
		m_sceneRenderAgent.release();

		for (int i = 0; i < (int)eCBType::END; ++i)
		{
			m_constBuffers[i].reset();
		}
		for (int i = 0; i < (int)eSamplerType::END; ++i)
		{
			m_samplerStates[i] = nullptr;
		}
		for (int i = 0; i < (int)eRasterizerState::END; ++i)
		{
			m_rasterizerStates[i] = nullptr;
		}
		for (int i = 0; i < (int)eDepthStencilState::END; ++i)
		{
			m_depthStencilStates[i] = nullptr;
		}
		for (int i = 0; i < (int)eBlendState::END; ++i)
		{
			m_blendStates[i] = nullptr;
		}

		m_postProcessTexture = nullptr;

		for (int i = 0; i < (int)eMRTType::END; ++i)
		{
			m_multi_render_targets[i].reset();
		}

		m_viewport = D3D11_VIEWPORT{};
		m_depth_stencil_buffer_texture = nullptr;
		m_renderTarget_texture = nullptr;
		m_swapChain = nullptr;
	}

	void RenderManager::release()
	{
		m_context = nullptr;
		m_device = nullptr;
	}

	bool RenderManager::SetResolution(UINT _resX, UINT _resY)
	{
		bool bResult = false;
		//1. 스왑체인 및 최종 렌더타겟 생성
		std::shared_ptr<Texture> RenderTex = CreateSwapChain(_resX, _resY, m_refreshRate);
		if (nullptr == RenderTex)
		{
			ERROR_MESSAGE("렌더타겟 생성 실패");
			return false;
		}
		m_renderTarget_texture = RenderTex;

		//3. DS 텍스처 다시 생성
		std::shared_ptr<Texture> DSTex = CreateDepthStencil(_resX, _resY);
		if (nullptr == DSTex)
		{
			ERROR_MESSAGE("Depth-Stencil 텍스처 생성 실패");
			return false;
		}
		m_depth_stencil_buffer_texture = DSTex;

		if (false == CreateMultiRenderTargets(_resX, _resY))
		{
			ERROR_MESSAGE("해상도 변경 실패");
			return false;
		}

		//4. RenderMgr에서 해상도에 영향을 받는 요소들 값 변경
		//RenderManager가 초기화 된 이후(나중에 해상도 변경 시)에만 여기서 실행
		m_sceneRenderAgent.SetResolution(_resX, _resY);

		gGlobal.uResolution = uint2(_resX, _resY);
		gGlobal.fResolution = float2(_resX, _resY);

		return true;
	}

	std::shared_ptr<Texture> RenderManager::CreateSwapChain(UINT _resX, UINT _resY, UINT _RefreshRate)
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

		swapChainDesc.OutputWindow = GameEngine::get_inst().GetHwnd();
		swapChainDesc.Windowed = true;
		swapChainDesc.BufferCount = 2;
		//DXGI WARNING: IDXGIFactory::CreateSwapChain: Blt-model swap effects (DXGI_SWAP_EFFECT_DISCARD and DXGI_SWAP_EFFECT_SEQUENTIAL) are legacy swap effects that are predominantly superceded by their flip-model counterparts (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD)
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferDesc.Width = _resX;
		swapChainDesc.BufferDesc.Height = _resY;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = _RefreshRate;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1u;

		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;

		Microsoft::WRL::ComPtr<IDXGIDevice> pDXGIDevice = nullptr;
		Microsoft::WRL::ComPtr<IDXGIAdapter> pDXGIAdapter = nullptr;
		Microsoft::WRL::ComPtr<IDXGIFactory> pDXGIFactory = nullptr;

		if (FAILED(m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)pDXGIDevice.GetAddressOf())))
		{
			return nullptr;
		}

		if (FAILED(pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)pDXGIAdapter.GetAddressOf())))
		{
			return nullptr;
		}

		if (FAILED(pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)pDXGIFactory.GetAddressOf())))
		{
			return nullptr;
		}

		if (FAILED(pDXGIFactory->CreateSwapChain(m_device.Get(), &swapChainDesc, m_swapChain.GetAddressOf())))
		{
			return nullptr;
		}

		
		//최종 렌더타겟 생성
		std::shared_ptr<Texture> ReturnTex = std::make_shared<Texture>();
		Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTarget = nullptr;

		// Get rendertarget for swapchain
		if (FAILED(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)renderTarget.GetAddressOf())))
		{
			ERROR_MESSAGE("스왑체인으로부터 렌더타겟을 받아오는 데 실패했습니다.");
			return nullptr;
		}

		//스왑체인으로 받아온 텍스처 버퍼를 가지고 렌더타겟 텍스처 뷰(RTV) 생성
		if (false == ReturnTex->create(renderTarget))
		{
			ERROR_MESSAGE("렌더타겟 텍스처 생성에 실패했습니다.");
			return nullptr;
		}

		m_resolution_x = _resX;
		m_resolution_y = _resY;
		return ReturnTex;
	}


	std::shared_ptr<Texture> RenderManager::CreateDepthStencil(UINT _Width, UINT _Height)
	{
		std::shared_ptr<Texture> DSTex = std::make_shared<ehw::Texture>();
		
		if (false == DSTex->create(_Width, _Height, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL))
		{
			ERROR_MESSAGE("Depth Stencil 버퍼 생성에 실패했습니다.");
			return nullptr;
		}

		return DSTex;
	}

	void RenderManager::CreateMainViewPort()
	{
		//창 속에서의 상대적인 좌표이므로 0부터 시작하면 됨
		m_viewport.TopLeftX = (FLOAT)0.f;
		m_viewport.TopLeftY = (FLOAT)0.f;
		m_viewport.MinDepth = (FLOAT)0.f;
		m_viewport.MaxDepth = (FLOAT)1.f;

		int2 winSize = GameEngine::get_inst().GetWindowSize();

		m_viewport.Width = (FLOAT)winSize.x;
		m_viewport.Height = (FLOAT)winSize.y;

		m_context->RSSetViewports(1u, &m_viewport);
	}



	void RenderManager::ClearRenderTarget()
	{
		constexpr FLOAT backgroundColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
		m_context->ClearRenderTargetView(m_renderTarget_texture->GetRTV().Get(), backgroundColor);
		m_context->ClearDepthStencilView(m_depth_stencil_buffer_texture->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	}

	void RenderManager::Present(bool _bVSync)
	{
		m_swapChain->Present(_bVSync ? 1 : 0, 0u);
	}


	void RenderManager::render()
	{
		ClearMultiRenderTargets();

		UpdateGlobalCBuffer();

		BindNoiseTexture();

		m_sceneRenderAgent.render();
	}

	void RenderManager::render_debug()
	{
		m_sceneRenderAgent.render_debug();
	}

	void RenderManager::FrameEnd()
	{
		m_sceneRenderAgent.frame_end();
	}

	void RenderManager::BindNoiseTexture()
	{
		std::shared_ptr<Texture> noise = ResourceManager<Texture>::get_inst().find(name::defaultRes::texture::noise_03);
		noise->bind_buffer_as_SRV(GPU::Register::t::NoiseTexture, eShaderStageFlag::ALL);

		tCB_Noise info = {};
		info.NoiseSize.x = (float)noise->GetWidth();
		info.NoiseSize.y = (float)noise->GetHeight();

		static float noiseTime = 10.f;
		noiseTime -= TimeManager::get_inst().DeltaTime();
		info.NoiseTime = noiseTime;

		ConstBuffer* cb = m_constBuffers[(uint)eCBType::Noise].get();
		cb->set_data(&info);
		cb->bind_buffer_to_GPU_register(eShaderStageFlag::ALL);
	}
	void RenderManager::CopyRenderTarget()
	{
		std::shared_ptr<Texture> renderTarget = ResourceManager<Texture>::get_inst().find(name::defaultRes::texture::RenderTarget);

		//renderTarget->unbind_buffer();

		//ID3D11ShaderResourceView* srv = nullptr;
		//RenderManager::get_inst().Context()->PSSetShaderResources()(eShaderStage::Pixel, 60, &srv);

		ID3D11Texture2D* dest = m_postProcessTexture->get_texture().Get();
		ID3D11Texture2D* source = renderTarget->get_texture().Get();

		RenderManager::get_inst().Context()->CopyResource(dest, source);

		m_postProcessTexture->bind_buffer_as_SRV(GPU::Register::t::postProcessTexture, eShaderStageFlag::Pixel);
	}

	void RenderManager::ClearMultiRenderTargets()
	{
		for (int i = 0; i < (int)eMRTType::END; ++i)
		{
			if (m_multi_render_targets[i])
			{
				if ((int)eMRTType::Swapchain == i)
				{
					m_multi_render_targets[i]->Clear(float4(0.2f));
				}
				else
				{
					m_multi_render_targets[i]->Clear(float4(0.f));
				}
			}

		}
	}


	void RenderManager::UpdateGlobalCBuffer()
	{
		gGlobal.DeltaTime = TimeManager::get_inst().DeltaTime();
		gGlobal.AccFramePrev = gGlobal.AccFrame;
		gGlobal.AccFrame += 1u;

		ConstBuffer* global = GetConstBuffer(eCBType::Global);
		global->set_data(&gGlobal);
		global->bind_buffer_to_GPU_register();
	}


	bool RenderManager::CreateMultiRenderTargets(UINT _resX, UINT _resY)
	{
		{
			//Swapchain MRT
			std::shared_ptr<Texture> arrRTTex[8] = {};
			std::shared_ptr<Texture> dsTex = nullptr;

			arrRTTex[0] = RenderManager::get_inst().GetRenderTargetTex();
			dsTex = RenderManager::get_inst().GetDepthStencilBufferTex();

			m_multi_render_targets[(UINT)eMRTType::Swapchain] = std::make_unique<MultiRenderTarget>();

			if (false == m_multi_render_targets[(UINT)eMRTType::Swapchain]->create(arrRTTex, dsTex))
			{
				ERROR_MESSAGE("Multi Render Target 생성 실패.");
				return false;
			}
		}

		// Deffered MRT
		{
			std::shared_ptr<Texture> arrRTTex[MRT_MAX] = {};
			std::shared_ptr<Texture> dsTex = nullptr;

			for (int i = 0; i < (int)eMRT_Deffered::END; ++i)
			{
				std::shared_ptr<Texture> defferedTex = std::make_shared<Texture>();
				arrRTTex[i] = defferedTex;
				arrRTTex[i]->create(_resX, _resY, DXGI_FORMAT_R32G32B32A32_FLOAT
					, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
				arrRTTex[i]->set_resource_name(name::eMRT_Deffered_String[i]);
			}


			dsTex = RenderManager::get_inst().GetDepthStencilBufferTex();

			m_multi_render_targets[(int)eMRTType::Deffered] = std::make_unique<MultiRenderTarget>();
			if (false == m_multi_render_targets[(int)eMRTType::Deffered]->create(arrRTTex, dsTex))
			{
				ERROR_MESSAGE("Multi Render Target 생성 실패.");
				return false;
			}
		}


		// Light MRT
		{
			std::shared_ptr<Texture> arrRTTex[MRT_MAX] = { };

			for (int i = 0; i < (int)eMRT_Light::END; ++i)
			{
				std::shared_ptr<Texture> defferedTex = std::make_shared<Texture>();
				arrRTTex[i] = defferedTex;
				arrRTTex[i]->create(_resX, _resY, DXGI_FORMAT_R32G32B32A32_FLOAT
					, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
				arrRTTex[i]->set_resource_name(name::eMRT_Light_String[i]);
			}

			m_multi_render_targets[(int)eMRTType::Light] = std::make_unique<MultiRenderTarget>();
			m_multi_render_targets[(int)eMRTType::Light]->create(arrRTTex, nullptr);
		}

		SetTexturesToDefferedMaterials();

		return true;
	}

	void RenderManager::SetTexturesToDefferedMaterials()
	{
		//Light
		{
			std::shared_ptr<Material> lightDirMtrl = ResourceManager<Material>::get_inst().find(name::defaultRes::material::LightDirMaterial);

			std::shared_ptr<Material> lightPointMtrl = ResourceManager<Material>::get_inst().find(name::defaultRes::material::LightPointMaterial);

			MultiRenderTarget* DefferedMRT = m_multi_render_targets[(uint)eMRTType::Deffered].get();

			{
				//Position Target
				std::shared_ptr<Texture> positionTarget =
					DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::PositionTarget);
				lightDirMtrl->set_texture(eTextureSlot::PositionTarget, positionTarget);
				lightPointMtrl->set_texture(eTextureSlot::PositionTarget, positionTarget);
			}

			{
				//Normal Target
				std::shared_ptr<Texture> normalTarget =
					DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::NormalTarget);

				lightDirMtrl->set_texture(eTextureSlot::NormalTarget, normalTarget);
				lightPointMtrl->set_texture(eTextureSlot::NormalTarget, normalTarget);
			}

			{
				//Specular Target
				std::shared_ptr<Texture> specularTarget =
					DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::SpecularTarget);

				lightDirMtrl->set_texture(eTextureSlot::NormalTarget, specularTarget);
				lightPointMtrl->set_texture(eTextureSlot::NormalTarget, specularTarget);
			}
		}

		//Merge
		{
			std::shared_ptr<Material> mergeMaterial = ResourceManager<Material>::get_inst().find(name::defaultRes::material::MergeMaterial);

			MultiRenderTarget* DefferedMRT = m_multi_render_targets[(uint)eMRTType::Deffered].get();
			{
				std::shared_ptr<Texture> AlbedoRT = DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::AlbedoTarget);
				mergeMaterial->set_texture(eTextureSlot::AlbedoTarget, AlbedoRT);
			}

			{
				std::shared_ptr<Texture> NormalRT = DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::NormalTarget);
				mergeMaterial->set_texture(eTextureSlot::NormalTarget, NormalRT);
			}

			{
				std::shared_ptr<Texture> SpecularRT = DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::SpecularTarget);
				mergeMaterial->set_texture(eTextureSlot::SpecularTarget, SpecularRT);
			}

			{
				std::shared_ptr<Texture> EmissiveRT = DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::EmissiveTarget);
				mergeMaterial->set_texture(eTextureSlot::EmissiveTarget, EmissiveRT);
			}

			{
				std::shared_ptr<Texture> RoughnessMetailcRT = DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::RoughnessAndMetalicTarget);
				mergeMaterial->set_texture(eTextureSlot::RoughnessAndMetalicTarget, RoughnessMetailcRT);
			}


			{
				std::shared_ptr<Texture> PosRenderTarget = DefferedMRT->GetRenderTarget((uint)eMRT_Deffered::PositionTarget);
				mergeMaterial->set_texture(eTextureSlot::PositionTarget, PosRenderTarget);
			}


			MultiRenderTarget* LightMRT = m_multi_render_targets[(uint)eMRTType::Light].get();
			{
				std::shared_ptr<Texture> DiffuseLightTarget = LightMRT->GetRenderTarget((uint)eMRT_Light::DiffuseLightTarget);
				mergeMaterial->set_texture(eTextureSlot::DiffuseLightTarget, DiffuseLightTarget);
			}
			{
				std::shared_ptr<Texture> SpecularLightTarget = LightMRT->GetRenderTarget((uint)eMRT_Light::SpecularLightTarget);
				mergeMaterial->set_texture(eTextureSlot::SpecularLightTarget, SpecularLightTarget);
			}
		}


	}

}


