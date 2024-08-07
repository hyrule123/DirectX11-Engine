#include "Engine/Manager/SceneRenderAgent.h"

#include "Engine/Manager/RenderManager.h"

#include "Engine/GPU/StructBuffer.h"
#include "Engine/GPU/ConstBuffer.h"

#include "Engine/Scene/Component/Light/Com_Light3D.h"
#include "Engine/Scene/Component/Camera/Com_Camera.h"

namespace ehw {
	SceneRenderAgent::SceneRenderAgent()
		: m_cameras{}
		, m_mainCamIndex(0u)
		, m_renderers{}
		, m_lights_3D{}
		, m_lightAttributes{}
		, m_lights_SBuffer{}
	{
	}
	SceneRenderAgent::~SceneRenderAgent()
	{
	}

	void SceneRenderAgent::Init()
	{
		StructBuffer::Desc SDesc{};
		SDesc.eSBufferType = eStructBufferType::READ_ONLY;
		m_lights_SBuffer = std::make_unique<StructBuffer>(SDesc);
		m_lights_SBuffer->Create<tLightAttribute>(128u, nullptr, 0);
	}

	void SceneRenderAgent::Release()
	{
		m_cameras.clear();
		m_mainCamIndex = 0u;

		m_renderers.clear();

		m_lightAttributes.clear();
		m_lights_SBuffer.reset();
	}

	void SceneRenderAgent::FrameEnd()
	{
		m_renderers.clear();
		m_lightAttributes.clear();
		m_lights_3D.clear();
	}

	void SceneRenderAgent::SetResolution(UINT _resX, UINT _resY)
	{
		for (const auto& iter : m_cameras)
		{
			if (iter)
			{
				iter->CreateProjectionMatrix(_resX, _resY);
			}
		}
	}

	void SceneRenderAgent::EraseIfDestroyed_Camera(bool _callRenderFunction = false)
	{
		std::erase_if(m_cameras,
			[_callRenderFunction](Com_Camera* _cam)->bool
			{
				if (_cam->IsDestroyed())
				{
					return true;
				}
				else if (_callRenderFunction && _cam->IsEnabled())
				{
					_cam->RenderCamera();
				}

				return false;
			}
		);
	}

	void SceneRenderAgent::SetMainCamera(Com_Camera* const _pCam)
	{
		for (size_t i = 0; i < m_cameras.size(); ++i)
		{
			if (_pCam == m_cameras[i])
			{
				m_mainCamIndex = i;
				return;
			}
		}
	}

	void SceneRenderAgent::RegisterCamera(Com_Camera* const _pCam)
	{
		ASSERT(_pCam, "nullptr"); m_cameras.push_back(_pCam);
	}

	void SceneRenderAgent::RemoveCamera(Com_Camera* const _pCam)
	{
		for (auto iter = m_cameras.begin(); iter != m_cameras.end(); ++iter)
		{
			if (_pCam == (*iter))
			{
				m_cameras.erase(iter);
				return;
			}
		}
	}


	void SceneRenderAgent::RemoveLight(Com_Light3D* const _pComLight)
	{
		for (auto iter = m_lights_3D.begin(); iter != m_lights_3D.end(); ++iter)
		{
			if (_pComLight == (*iter))
			{
				m_lights_3D.erase(iter);
				break;
			}
		}
	}

	void SceneRenderAgent::BindLights()
	{
		m_lights_SBuffer->SetData(m_lightAttributes.data(), m_lightAttributes.size());

		eShaderStageFlag_ Flag = eShaderStageFlag::Vertex | eShaderStageFlag::Pixel;

		m_lights_SBuffer->BindDataSRV(GPU::Register::t::lightAttributes, Flag);

		tCB_NumberOfLight trCb = {};
		trCb.numberOfLight = (uint)m_lightAttributes.size();


		//Destroy된 light 포인터 제거
		std::erase_if(m_lights_3D,
			[](Com_Light3D* const iter)->bool
			{
				return iter->IsDestroyed();
			}
		);

		//light index 지정
		for (size_t i = 0; i < m_lights_3D.size(); i++)
		{
			if (m_lights_3D[i]->IsEnabled())
			{
				m_lights_3D[i]->SetIndex((uint)i);
			}
		}

		ConstBuffer* cb = RenderManager::GetInst().GetConstBuffer(eCBType::numberOfLight);
		cb->SetData(&trCb);
		cb->BindData(Flag);
	}
}
