// CodeGenerator.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "PCH_CodeGenerator.h"
#include "DirTree.h"

#include "../Engine/define_Res.h"
#include "../Engine/define_GPU.h"

#include <iostream>

void ManualInput(std::vector<std::string>& _args);
void CreateTextureKey();
void CreateShaderKey();
void CreateComponentKey();
void CreateScriptKey();
void CreateSceneKey();


//시작 지점 = $(SolutionDir) : 상대 경로로 작업해주면 된다.
int main(int argc, char* argv[])
{
    std::vector<std::string>args{};
    for (int i = 1; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }

    //argument가 들어오지 않았을 경우 직접 입력 받는다
    if (args.empty())
    {
        ManualInput(args);
    }


    for (size_t i = 0; i < args.size(); ++i)
    {
        StringConv::UpperCase(args[i]);

        if ("TEXTURE" == args[i])
        {
            //CreateTextureKey();
        }
        else if ("SHADER" == args[i])
        {
            CreateShaderKey();
        }
        else if ("COMPONENT" == args[i])
        {
            CreateComponentKey();
        }
        else if ("SCRIPT" == args[i])
        {
            CreateScriptKey();
        }
        else if ("SCENE" == args[i])
        {
            CreateSceneKey();
        }
    }
    
    return 0;
}

void CreateTextureKey()
{
    //Create variable name restraints regex
    define_Preset::Regex::g_VarForbiddenChars::CreateVarForbiddenRegex();

    constexpr const char* TexExt[] = { ".bmp", ".png", ".jpg", ".tga", ".dds" };
    constexpr const size_t TexExtSize = sizeof(TexExt) / sizeof(TexExt[0]);

    //Generate Texture Key

    std::string regbase;
    regbase += R"((.+)\.()";

    for (size_t i = 0; i < TexExtSize; ++i)
    {
        std::string temp = TexExt[i];
        size_t pos = temp.find('.');
        if (std::string::npos != pos)
        {
            temp.erase(0, pos + 1);
        }
        regbase += temp;

        if (i == TexExtSize - 1ui64)
            regbase += ")$";
        else
            regbase += "|";
    }

    std::regex reg(regbase, std::regex::icase);

    DirTree DirTree;
    {
        stdfs::path DirPath = define_Preset::Path::Content::A;
        DirPath /= ehw::define::strKey::GetResName(ehw::define::eResourceType::Texture);
        DirTree.SearchRecursive(DirPath, reg);
    }

    stdfs::path outPath = define_Preset::Path::ContentsProj::A;
    outPath /= "strKey_Texture.h";
    DirTree.CreateStrKeyHeader(outPath, "Texture", false);

}

void ManualInput(std::vector<std::string>& _args)
{
    std::cout << "[[Manual Mode]]" << std::endl;

    std::cout << "Possible Modes: ";
    std::cout << "ComputeShader, Component, Script, Scene" << std::endl;
    std::cout << "enter 'q' to end input" << std::endl;

    bool bGetInputs = true;
    while (bGetInputs)
    {
        std::cout << ">>";
        std::string input{};
        std::cin >> input;
        if ("q" == input || input.empty())
        {
            bGetInputs = false;
            continue;
        }

        _args.push_back(input);
    }
}

void CreateShaderKey()
{
    //Generate Compute Shader Key    
    std::regex reg(define_Preset::Regex::AllShader::A);

    DirTree DirTree;
    DirTree.SearchRecursive(define_Preset::Path::Shader_Proj::A, reg);

    stdfs::path strKeyPath = define_Preset::Path::ContentsProj::A;
    strKeyPath /= "strKey_Shader.h";
    DirTree.CreateShaderStrKey(strKeyPath);
    //DirTree.CreateStrKeyHeader(DirPath / DIRECTORY_NAME, "Shader", true);

    //일단 미사용
    //DirTree.CreateCShaderCode(DirPath / define_Preset::Path::UserClassInit_CS::A);
    
}

void CreateComponentKey()
{
    //Generate Componets
    std::regex regexCom(R"(^Com_\w+\.h)");

    DirTree DirTree;
    stdfs::path DirPath = define_Preset::Path::EngineProj::A;

    DirTree.SearchRecursive(DirPath, regexCom);

    DirTree.CreateStrKeyHeader(DirPath / "strKey_Component.h", "com", true);

    tAddBaseClassDesc Desc = {};
    Desc.BaseType = "IComponent";
    Desc.IncludePCH = R"(#include "PCH_Engine.h")";
    Desc.ClassName = "ComponentInitializer";
    Desc.IncludeStrKeyHeaderName = R"(#include "strKey_Component.h")";
    Desc.IncludeManagerHeader = R"(#include "ComMgr.h")";
    Desc.MasterNamespace = "namespace ehw";
    Desc.UsingNamespace = "using namespace ehw::define;";
    Desc.Constructor_T_MacroDefine = R"(ComMgr::AddComConstructor<T>(strKey::com::##T))";
    Desc.UserClassMgr_InitFuncName = "Init()";
    Desc.FilePath = DirPath / "ComponentInitializer.cpp";

    DirTree.CreateComMgrInitCode(Desc);
    
}

void CreateScriptKey()
{
    //Generate Scipts
    std::regex regexCom(R"(Script_\w+\.h)");

    DirTree DirTree;
    stdfs::path DirPath = define_Preset::Path::ContentsProj::A;

    DirTree.SearchRecursive(DirPath, regexCom);

    DirTree.CreateStrKeyHeader(DirPath / "strKey_Script.h", "Script", true);

    tAddBaseClassDesc Desc = {};
    Desc.BaseType = "IScript";
    Desc.IncludePCH = R"(#include "PCH_UserContents.h")";
    Desc.ClassName = "ContentsClassInitializer";
    Desc.IncludeStrKeyHeaderName = R"(#include "strKey_Script.h")";
    Desc.IncludeManagerHeader = "#include <EngineBase/Engine/ComMgr.h>";
    Desc.MasterNamespace = "namespace ehw";
    Desc.UsingNamespace = "using namespace ehw::define;";
    Desc.Constructor_T_MacroDefine = R"(ComMgr::AddComConstructor<T>(strKey::Script::##T))";
    Desc.UserClassMgr_InitFuncName = "InitScript()";
    Desc.FilePath = DirPath / "ContentsClassInitializer_Script.cpp";

    DirTree.CreateComMgrInitCode(Desc);
    
}

void CreateSceneKey()

//Generate Scene
{
    std::regex regexCom(R"(Scene_\w+\.h)");

    DirTree DirTree;
    stdfs::path DirPath = define_Preset::Path::ContentsProj::A;

    DirTree.SearchRecursive(DirPath, regexCom);

    DirTree.CreateStrKeyHeader(DirPath / "strKey_Scene.h", "Scene", true);

    tAddBaseClassDesc Desc = {};
    Desc.BaseType = "IScene";
    Desc.IncludePCH = R"(#include "PCH_UserContents.h")";
    Desc.ClassName = "ContentsClassInitializer";
    Desc.IncludeStrKeyHeaderName = R"(#include "strKey_Scene.h")";
    Desc.IncludeManagerHeader = "#include <EngineBase/Engine/SceneMgr.h>";
    Desc.MasterNamespace = "namespace ehw";
    Desc.UsingNamespace = "using namespace ehw::define;";
    Desc.Constructor_T_MacroDefine = R"(SceneMgr::AddSceneConstructor<T>(strKey::Scene::##T))";
    Desc.UserClassMgr_InitFuncName = "InitScene()";
    Desc.FilePath = DirPath / "ContentsClassInitializer_Scene.cpp";

    DirTree.CreateComMgrInitCode(Desc);
}
