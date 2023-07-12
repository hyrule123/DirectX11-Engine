#include "guiYamYamEditor.h"


namespace gui
{
	YamYamEditor::YamYamEditor()
        : mbPadding(false)
        , mbFullscreen(true)
        , mDockspace_flags(ImGuiDockNodeFlags_None)
	{
		SetName("YamYamEditor");
        SetSize(ImVec2(100.0f, 100.0f));

	}

	YamYamEditor::~YamYamEditor()
	{
	}

	void YamYamEditor::FixedUpdate()
	{
        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        mWindow_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (mbFullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            mWindow_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            mWindow_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            mDockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (mDockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            mWindow_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        if (!mbPadding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	}

	void YamYamEditor::Update()
	{

        
        
        //ImGui::Begin("DockSpace Demo", p_open, window_flags);


        if (!mbPadding)
            ImGui::PopStyleVar();

        if (mbFullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("YamYamEndgineDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), mDockspace_flags);
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Options"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                ImGui::MenuItem("Fullscreen", NULL, &mbFullscreen);
                ImGui::MenuItem("Padding", NULL, &mbPadding);
                ImGui::Separator();

                if (ImGui::MenuItem("Flag: NoSplit", "", (mDockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0)) { mDockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
                if (ImGui::MenuItem("Flag: NoResize", "", (mDockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) { mDockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
                if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (mDockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0)) { mDockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
                if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (mDockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) { mDockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
                if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (mDockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, mbFullscreen)) { mDockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
                ImGui::Separator();

                bool p_open = (bool)GetState();
                if (ImGui::MenuItem("Close", NULL, false, p_open != NULL))
                    SetState(eState::Paused);

                ImGui::EndMenu();
            }
           
            ImGui::EndMenuBar();
        }

	}

	void YamYamEditor::LateUpdate()
	{
	}

}
