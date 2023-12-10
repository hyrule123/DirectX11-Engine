#include "PCH_Engine.h"
#include "guiMaterialEditor.h"

#include "Material.h"
#include "PathManager.h"
#include "ResourceManager.h"
#include "GraphicsShader.h"
#include "Application.h"
#include "define_Util.h"


namespace editor
{
	STRKEY eRenderingMode_String[(int)ehw::eRenderingMode::END] =
	{
		//Deffered
		"DefferdOpaque",
		"DefferdMask",

		"Light",

		//Forward
		"Opaque",
		"CutOut",
		"Transparent",
		"PostProcess",
	};



	guiMaterialEditor::guiMaterialEditor()
		: guiWindow("Material Editor")
		, mTargetMaterial()
		, mShaderCombo{}
		, mbNewMaterial()
	{
		mTargetMaterial = std::make_shared<ehw::Material>();
	}
	guiMaterialEditor::~guiMaterialEditor()
	{
	}

	void guiMaterialEditor::Init()
	{
		//쉐이더 파일 목록 업데이트
		mShaderCombo.SetStrKey("Shader Lists");
		RefreshShaderSettingFiles();

		//렌더링 모드 UI 생성
		mRenderingModeCombo.SetStrKey("Rendering Mode");
		{
			for (int i = 0; i < (int)ehw::eRenderingMode::END; ++i)
			{
				mRenderingModeCombo.AddItem(eRenderingMode_String[i]);
			}
		}
	}

	void guiMaterialEditor::Update()
	{
		if (mShaderCombo.IsSelectionChanged())
		{
			std::string shaderName = mShaderCombo.GetCurrentSelected().strName;
			if (false == shaderName.empty())
			{
				std::shared_ptr<ehw::GraphicsShader> gs = ehw::ResourceManager::Load<ehw::GraphicsShader>(shaderName);
			}
		}
	}

	void guiMaterialEditor::UpdateUI()
	{
		UpdateShader();

		ImGui::Separator();

		UpdateRenderingMode();

		ImGui::Separator();

		UpdateTextureList();

		ImGui::Separator();

		UpdateMtrlConstBuffer();

		ImGui::Separator();

		if(ImGui::Button("New Material", ImVec2(0.f, 35.f)))
		{
			NewMaterial();
		}

		ImGui::SameLine();

		UpdateSaveLoad();
	}
	void guiMaterialEditor::RefreshShaderSettingFiles()
	{
		const std::fs::path& shaderPath = 
			ehw::PathManager::GetContentPathRelative(ehw::eResourceType::GraphicsShader);

		if (false == std::fs::exists(shaderPath))
		{
			std::fs::create_directories(shaderPath);
			return;
		}

		mShaderCombo.Reset();

		std::fs::directory_iterator dirIter(shaderPath);
		for (auto& entry : dirIter)
		{
			std::fs::path relPath = entry.path().lexically_relative(shaderPath);
			mShaderCombo.AddItem(relPath.string());
		}
	}
	void guiMaterialEditor::UpdateShader()
	{
		std::string strCurShader = "Current Shader: ";
		{
			std::shared_ptr<ehw::GraphicsShader> curShader = mTargetMaterial->GetShader();
			if (curShader)
			{
				strCurShader += curShader->GetStrKey();
			}
			else
			{
				strCurShader += "None";
			}
		}
		ImGui::Text(strCurShader.c_str());


		mShaderCombo.InternalUpdate();
		if (ImGui::Button("Refresh Shaders"))
		{
			RefreshShaderSettingFiles();
		}

		if (mShaderCombo.IsSelectionChanged())
		{
			const std::string& shaderKey = mShaderCombo.GetCurrentSelected().strName;
			if (false == shaderKey.empty())
			{
				std::shared_ptr<ehw::GraphicsShader> GS = ehw::ResourceManager::Load<ehw::GraphicsShader>(shaderKey);
				mTargetMaterial->SetShader(GS);
			}
		}

		std::string shaderKey = "Shader: ";
		std::shared_ptr<ehw::GraphicsShader> shader = mTargetMaterial->GetShader();
		if (shader)
		{
			shaderKey += shader->GetStrKey();
		}
		else
		{
			shaderKey += "None";
		}
		ImGui::Text(shaderKey.c_str());
	}
	void guiMaterialEditor::UpdateTextureList()
	{
		for (int i = 0; i < (int)ehw::eTextureSlot::END; ++i)
		{
			constexpr const char* texText = "Texture_";
			std::string text = texText;
			text += std::to_string(i);
			text += ": ";
			ImGui::Text(text.c_str());

			ImGui::SameLine();

			auto Tex = mTargetMaterial->GetTexture((ehw::eTextureSlot)i);
			if (Tex)
			{
				ImGui::Text(Tex->GetStrKey().c_str());
			}
			else
			{
				ImGui::Text("None");
			}

			ImGui::SameLine();

			std::string ButtonName = "Load##";
			ButtonName += std::to_string(i);
			if (ImGui::Button(ButtonName.c_str()))
			{
				const std::fs::path& texPath = ehw::PathManager::GetContentPathAbsolute(ehw::eResourceType::Texture);
				
				std::vector<std::fs::path> vecExt{};
				for (size_t i = 0; i < ehw::strKey::Ext_Tex_Size; ++i)
				{
					vecExt.push_back(ehw::strKey::Ext_Tex[i]);
				}
				std::fs::path receivedPath = ehw::WinAPI::FileDialog(texPath, vecExt);
				if (false == receivedPath.empty())
				{
					std::fs::path PathstrKey =  ehw::PathManager::MakePathStrKey(receivedPath);

					std::shared_ptr<ehw::Texture> tex = ehw::ResourceManager::Load<ehw::Texture>(PathstrKey);
					if (tex)
					{
						mTargetMaterial->SetTexture((ehw::eTextureSlot)i, tex);
					}
					else
					{
						ERROR_MESSAGE_W(L"텍스처 찾기 실패");
					}
				}

			}
			ImGui::SameLine();

			ButtonName = "Remove##";
			ButtonName += std::to_string(i);
			if (ImGui::Button(ButtonName.c_str()))
			{
				mTargetMaterial->SetTexture((ehw::eTextureSlot)i, nullptr);
			}
		}
	}
	void guiMaterialEditor::UpdateRenderingMode()
	{
		ehw::eRenderingMode mode = mTargetMaterial->GetRenderingMode();
		mRenderingModeCombo.SetCurrentIndex((int)mode);
		mRenderingModeCombo.InternalUpdate();
		if (mRenderingModeCombo.IsSelectionChanged())
		{
			int idx = mRenderingModeCombo.GetCurrentIndex();
			if (0 <= idx)
			{
				mode = (ehw::eRenderingMode)idx;
				mTargetMaterial->SetRenderingMode(mode);
			}
		}
	}
	void guiMaterialEditor::UpdateMtrlConstBuffer()
	{
		ImGui::DragFloat4("Diffuse Color", 
			reinterpret_cast<float*>(&(mTargetMaterial->mCB.Diff)));

		ImGui::DragFloat4("Specular Color",
			reinterpret_cast<float*>(&(mTargetMaterial->mCB.Spec)));

		ImGui::DragFloat4("Ambient Color",
			reinterpret_cast<float*>(&(mTargetMaterial->mCB.Amb)));

		ImGui::DragFloat4("Emissive Color",
			reinterpret_cast<float*>(&(mTargetMaterial->mCB.Emv)));
	}
	void guiMaterialEditor::UpdateSaveLoad()
	{
		if (ImGui::Button("Save to File", ImVec2(0.f, 35.f)))
		{
			if (CheckSavePossible())
			{
				SaveToFile();
			}
			
		}
		
		
		ImGui::SameLine();

		if (ImGui::Button("Load from File", ImVec2(0.f, 35.f)))
		{
			mCurContext = eContext::LoadFromFile;
			mSaveLoadFileName.clear();

			//ResMgr로부터 로드되어있는 재질 목록 싹 수집
			mCurrentLoadedMtrl.Reset();
			const auto& materials = ehw::ResourceManager::GetResources<ehw::Material>();
			for (auto& mtrl : materials)
			{
				mCurrentLoadedMtrl.AddItem(mtrl.second->GetStrKey());
			}
		}
		LoadFromFile();
	}
	bool guiMaterialEditor::CheckSavePossible()
	{
		bool bPossible = true;
		//저장 조건 확인
		if (nullptr == mTargetMaterial->GetShader() || mTargetMaterial->GetShader()->GetStrKey().empty())
		{
			MessageBoxW(nullptr, L"쉐이더를 설정하지 않았습니다.\n쉐이더는 반드시 설정해야 합니다.", nullptr, MB_OK);
			bPossible = false;
		}
		else if (nullptr == mTargetMaterial->mTextures[0])
		{
			if (IDNO == MessageBoxW(nullptr, L"첫 번째 텍스처(Tex0)가 없습니다.\n(에러는 아님 나중에 코드로 할거면 괜찮음)\n저장할까요?", L"Notification", MB_YESNO))
			{
				bPossible = false;
			}
		}

		return bPossible;
	}
	void guiMaterialEditor::SaveToFile()
	{
		std::fs::path outputPath = ehw::PathManager::GetContentPathAbsolute(ehw::eResourceType::Material);
		outputPath /= mSaveLoadFileName;
		outputPath = ehw::WinAPI::FileDialog(outputPath, ehw::strKey::Ext_Material);

		if (outputPath.empty())
		{
			NOTIFICATION_W(L"경로를 설정하지 않았습니다.");
		}

		//저장할 때는 Key값을 바꿔야 하기 때문에 Clone 해서 저장해야 한다.
		//기존 리소스를 그대로 Save하게 되면 Key 값이 변경되어 에러가 발생할 수 있음.
		mTargetMaterial = std::shared_ptr<ehw::Material>(mTargetMaterial->Clone());

		std::string strKey = outputPath.filename().string();
		mTargetMaterial->SetStrKey(strKey);
		mTargetMaterial->Save(strKey);

	}
	void guiMaterialEditor::LoadFromFile()
	{
		if (eContext::LoadFromFile == mCurContext)
		{
			//모달 창 팝업
			ImGui::SetNextWindowSize(ImVec2{ 400.f, 500.f });
			ImGui::OpenPopup("Material Load");
			if (ImGui::BeginPopupModal("Material Load"))
			{
				HilightText("Res/Material");
				mCurrentLoadedMtrl.InternalUpdate();

				if (ImGui::Button("Load##Material", ImVec2(100.f, 35.f)))
				{
					const std::string& mtrlKey = mCurrentLoadedMtrl.GetCurrentSelected().strName;
					if (mtrlKey.empty())
					{
						NOTIFICATION_W(L"파일을 선택하세요.");
					}
					else
					{
						std::shared_ptr<ehw::Material> mtrl = ehw::ResourceManager::Find<ehw::Material>(mtrlKey);

						//엔진 기본 Material일 경우에는 Clone
						if (mtrl->IsEngineDefaultRes())
						{
							mtrl = std::shared_ptr<ehw::Material>(mtrl->Clone());
							mtrl->SetEngineDefaultRes(false);
						}

						mTargetMaterial = mtrl;
						if (nullptr == mTargetMaterial)
						{
							NOTIFICATION_W(L"제대로 로드되지 않았습니다.");
							mTargetMaterial = std::make_shared<ehw::Material>();
						}


						mCurContext = eContext::None;
					}
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel##Material", ImVec2(100.f, 35.f)))
				{
					mCurContext = eContext::None;
				}

				ImGui::Separator();
				
				//파일 선택해서 로드
				if (ImGui::Button("Load From Other Directory", ImVec2(0.f, 35.f)))
				{
					//Res/Material 폴더
					const std::fs::path& mtrlDir = ehw::PathManager::GetContentPathAbsolute(ehw::eResourceType::Material);

					//Res 폴더
					const std::fs::path& resDir = ehw::PathManager::GetResPathAbsolute();

					std::fs::path filePath = ehw::WinAPI::FileDialog(mtrlDir, ".json");
					if (false == std::fs::exists(filePath))
					{
						NOTIFICATION_W(L"파일을 찾지 못했습니다.");
					}
					else
					{
						mTargetMaterial = std::make_shared<ehw::Material>();
						if (ehw::eResultFail(mTargetMaterial->Load(filePath.filename())))
						{
							std::wstring errMsg = filePath.wstring();
							errMsg += L"\n불러오기에 실패했습니다.";
							NOTIFICATION_W(errMsg.c_str());
							mTargetMaterial = std::make_shared<ehw::Material>();
						}

						//불러왔던 불러오지 못했던 모달 빠져나가줌
						mCurContext = eContext::None;
					}
				}

				ImGui::EndPopup();
			}
		}
	}
	void guiMaterialEditor::NewMaterial()
	{
		mTargetMaterial = std::make_shared<ehw::Material>();
	}
}
