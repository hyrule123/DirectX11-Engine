#pragma once
#include "guiChild.h"
#include "GameObject.h"
#include "guiComponent.h"
#include "guiResource.h"

namespace editor
{
	class guiInspector : public guiWindow
	{
	public:
		guiInspector();
		virtual ~guiInspector();

		virtual void Init() override;
		virtual void Update() override;
		virtual void UpdateUI() override;

		void SetTargetGameObject(ehw::GameObject* _targetObj) { mTargetGameObject = _targetObj; }
		ehw::GameObject* GetTargetGameObject() const { return mTargetGameObject; }

		void SetTargetResource(ehw::iResource* _targetRes) { mTargetResource = _targetRes; }
		ehw::iResource* GetTargetResource() const { return mTargetResource; }


	private:
		void IndicatorButton(const char* _strButtonName);

	private:
		ehw::GameObject* mTargetGameObject;
		ehw::iResource* mTargetResource;

		std::vector<editor::guiComponent*> mGuiComponents;
		std::vector<editor::guiResource*> mGuiResources;
	};
}
