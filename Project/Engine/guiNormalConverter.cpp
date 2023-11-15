#include "PCH_Engine.h"
#include "guiNormalConverter.h"

#include "PathMgr.h"
#include "ResourceMgr.h"
#include "define_Util.h"

#include "NormalConvertShader.h"
#include "DefaultShader/SH_NormalConvertMH.hlsli"

namespace gui
{
	guiNormalConverter::guiNormalConverter()
		: guiWindow("Normal Converter")
		, mTextureSrc{}
		, mTextureDestDir{}
	{
	}
	guiNormalConverter::~guiNormalConverter()
	{
	}

	void guiNormalConverter::Init()
	{
	}

	void guiNormalConverter::UpdateUI()
	{
		SrcTextureUpdate();

		ImGui::Dummy(ImVec2(0.f, 15.f));
		ImGui::Separator();

		DestTextureUpdate();

		ImGui::Dummy(ImVec2(0.f, 15.f));
		ImGui::Separator();
		
		CopyTextureUpdate();

		ImGui::Dummy(ImVec2(0.f, 50.f));
		ImGui::Separator();
		if (ImGui::Button("Reset All", ImVec2(0.f, 35.f)))
		{
			Reset();
		}
	}

	void guiNormalConverter::SrcTextureUpdate()
	{
		HilightText("Source Texture");

		{
			std::string curText = "* Current: ";
			if (mTextureSrc)
			{
				curText += mTextureSrc->GetKey();
			}
			else
			{
				curText += "None";
			}
			
			ImGui::Text(curText.c_str());
		}

if (ImGui::Button("Load##Source Texture" , ImVec2(0.f, 25.f)))
{
	std::vector<std::fs::path> vecExt{};
	for (size_t i = 0; i < ehw::define::strKey::Ext_Tex_Size; ++i)
	{
		vecExt.push_back(ehw::define::strKey::Ext_Tex[i]);
	}

	std::fs::path texPath = ehw::WinAPI::FileDialog(ehw::PathMgr::GetContentPathAbsolute(ehw::eResourceType::Texture), vecExt);

	texPath = ehw::PathMgr::MakePathStrKey(texPath);

	mTextureSrc = ehw::ResourceMgr::Load<ehw::Texture>(texPath);
}

ImGui::SameLine();

if (ImGui::Button("Clear##Source Texture", ImVec2(0.f, 25.f)))
{
	mTextureSrc = nullptr;
}

	}

	void guiNormalConverter::DestTextureUpdate()
	{
		HilightText("Dest Texture Save Directory");
		{
			std::string curText = "* Current: ";

			if (mTextureDestDir.empty())
			{
				curText += "None";
			}
			else
			{
				curText += mTextureDestDir.string();
			}


			ImGui::Text(curText.c_str());
		}


		if (ImGui::Button("Set Directory##Dest Texture", ImVec2(0.f, 25.f)))
		{
			std::vector<std::fs::path> vecExt{};
			for (size_t i = 0; i < ehw::define::strKey::Ext_Tex_Size; ++i)
			{
				vecExt.push_back(ehw::define::strKey::Ext_Tex[i]);
			}

			std::fs::path texFile = ehw::PathMgr::GetContentPathAbsolute(ehw::eResourceType::Texture);

			if (mTextureSrc)
			{
				texFile /= mTextureSrc->GetKey();
			}

			mTextureDestDir = ehw::WinAPI::FileDialog(texFile, vecExt);
			mTextureDestDir = mTextureDestDir.parent_path();
		}

		ImGui::SameLine();

		if (ImGui::Button("Clear##Dest Texture", ImVec2(0.f, 25.f)))
		{
			mTextureDestDir.clear();
		}
	}

	void guiNormalConverter::CopyTextureUpdate()
	{
		if (ImGui::Button("Convert Normal Map Texture", ImVec2(0.f, 35.f)))
		{
			if (nullptr == mTextureSrc)
			{
				NOTIFICATION_W(L"원본 텍스처가 등록되지 않았습니다.");
				return;
			}
			else if (mTextureDestDir.empty())
			{
				NOTIFICATION_W(L"텍스처파일 출력 경로가 등록되지 않았습니다.");
				return;
			}

			std::shared_ptr<ehw::NormalConvertShader> converter = ehw::ResourceMgr::Load<ehw::NormalConvertShader>(ehw::strKey::Default::shader::compute::NormalConvert);

			std::shared_ptr<ehw::Texture> convertedTex = converter->Convert(mTextureSrc);

			if (convertedTex)
			{
				std::fs::path savePath = mTextureDestDir;
				std::fs::path texKey = convertedTex->GetKey();
				savePath /= texKey.filename();

				if (std::fs::exists(savePath))
				{
					if (IDNO == MessageBoxW(nullptr, L"이미 파일이 있습니다. 덮어 쓰시겠습니까?", L"알림", MB_YESNO))
					{
						NOTIFICATION_W(L"저장을 취소했습니다.");
						return;
					}
				}

				if (eResultFail(convertedTex->Save(savePath)))
				{
					NOTIFICATION_W(L"변환에 실패했습니다.");
				}
				else
				{
					NOTIFICATION_W(L"변환 성공.");
				}
			}
			else
			{
				NOTIFICATION_W(L"변환 실패");
			}
		}
	}

	void guiNormalConverter::Reset()
	{
		mTextureSrc = nullptr;
		mTextureDestDir.clear();
	}
}
