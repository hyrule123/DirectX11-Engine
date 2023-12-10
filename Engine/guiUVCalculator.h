#pragma once
#include "EditorWindow.h"
#include "SimpleMath.h"

namespace ehw
{
    class Texture;
}

namespace editor
{
    class guiUVCalculator :
        public EditorWindow
    {
    public:
        guiUVCalculator();
        virtual ~guiUVCalculator();

        virtual void Init() override;
        virtual void UpdateUI() override;

    private:
        void UpdateLoadButton();
        void UpdateTexture();
        void UpdateUVIndicator();

    private:
        std::shared_ptr<ehw::Texture> mTexture;
        ImVec2                      mTexRenderSize;

        ImVec2 mCursorPos;
        ImVec2 mDragStartPos;
        ImVec2 mDragEndPos;
    };
}


