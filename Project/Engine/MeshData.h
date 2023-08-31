#pragma once
#include "IRes.h"
#include "define_Struct.h"
#include "define_GPU.h"

namespace gui
{
	class guiFBXConverter;
}

namespace mh
{
	class Mesh;
	class Material;
	class GameObject;
	class Com_Renderer_Mesh;
	class Skeleton;
	struct tFBXMaterial;

	struct tMeshContainer
	{
		std::shared_ptr<Mesh>					pMesh;
		std::vector<std::shared_ptr<Material>>	pMaterials;
	};

    class MeshData :
        public IRes
    {
		friend class gui::guiFBXConverter;
		
	public:
		MeshData();
		virtual ~MeshData();

		virtual eResult Save(const std::fs::path& _filePath, const std::fs::path& _basePath = "") override;
		virtual eResult Load(const std::fs::path& _filePath, const std::fs::path& _basePath = "") override;

		/*static eResult ConvertFBX(const std::fs::path& _fbxFullPath, bool _bStatic, const std::fs::path& _dirAndFileName);*/
		

		virtual eResult SaveJson(Json::Value* _pJson) override;
		virtual eResult LoadJson(const Json::Value* _pJson) override;
		GameObject* Instantiate();

	private:
		eResult LoadAndConvertFBX(
			const std::fs::path& _fbxPath, bool _bStatic,
			const std::fs::path& _dirAndFileName
			);

		//_defaultMtrlStrKey: FBX 변환된 재질이 이름이 없을수도 있음 -> 만약 없을 경우 사용할 기본 키 이름
		std::shared_ptr<Material> ConvertMaterial(const tFBXMaterial* _material, const std::fs::path& _texDestDir);
		bool SetRenderer(Com_Renderer_Mesh* _renderer, UINT _idx);

	private:
		std::vector<tMeshContainer> mMeshContainers;
		std::shared_ptr<Skeleton> mSkeleton;
    };

}


