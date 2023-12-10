#pragma once
#include "EditorWindow.h"

namespace editor
{
	class guiChild :
		public EditorWindow
	{
	public:
		guiChild(const std::string_view _strName);
		virtual ~guiChild();

		virtual bool BeginUI() override;
		virtual void UpdateUI() override {};
		virtual void EndUI() override;

		void SetSize(ImVec2 _v2Size) { mSize = _v2Size; }

	private:
		ImVec2 mSize;
		bool mbBorder;
	};
}


