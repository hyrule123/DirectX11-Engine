#pragma once
#include "Editor/Base/EditorChild.h"

#include "Resource/iResource.h"

namespace editor
{
	template <class ResourceType>
	class EditorResource : public EditorChild
	{
	public:
		EditorResource(const std::string_view _resTypeName);
		virtual ~EditorResource();

		void UpdateUI() override;
		
		void SetTarget(const std::weak_ptr<ResourceType>& _target) { mTarget = _target; }
		const std::weak_ptr<ResourceType>& GetTarget() const { return mTarget; }

	private:
		std::weak_ptr<ResourceType> mTarget;
	};

	template<class ResourceType>
	inline EditorResource<ResourceType>::EditorResource(const std::string_view _resTypeName)
		: EditorChild(_resTypeName)
	{
	}

	template<class ResourceType>
	inline EditorResource<ResourceType>::~EditorResource()
	{
	}

	template<class ResourceType>
	inline void EditorResource<ResourceType>::UpdateUI()
	{
		if (mTarget.expired())
			return;

		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));

		//ImGui::Button(ehw::strKey::ArrResName[(UINT)mTarget->GetResType()]);
		ImGui::PopStyleColor(3);
		ImGui::PopID();
	}
}
