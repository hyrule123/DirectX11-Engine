

//=========================================================
//This Code is Automatically generated by CodeGenerator.exe
//=========================================================




#include "ComponentInitializer.h"
#include "../../Manager/ComponentManager.h"
#include "strKey_Component.h"


#include "Com_DummyTransform.h"
#include "Com_Animator3D.h"
#include "Com_AudioListener.h"
#include "Com_Renderer_Sprite.h"
#include "Com_DummyAnimator.h"
#include "Com_Renderer_UIBase.h"
#include "Com_AudioSource.h"
#include "Com_BehaviorTree.h"
#include "Com_Renderer_Mesh.h"
#include "Com_Light3D.h"
#include "Com_Camera.h"
#include "Com_Renderer_3DAnimMesh.h"
#include "Com_Renderer_ParticleSystem.h"
#include "Com_Transform.h"
#include "TestDir\Com_Animator2D.h"

#define CONSTRUCTOR_T(T) ComponentManager::AddComponentConstructor<T>(strKey::component::##T)

namespace ehw
{

	void ComponentInitializer::Init()
	{
		CONSTRUCTOR_T(Com_DummyTransform);
		CONSTRUCTOR_T(Com_Animator3D);
		CONSTRUCTOR_T(Com_AudioListener);
		CONSTRUCTOR_T(Com_Renderer_Sprite);
		CONSTRUCTOR_T(Com_DummyAnimator);
		CONSTRUCTOR_T(Com_Renderer_UIBase);
		CONSTRUCTOR_T(Com_AudioSource);
		CONSTRUCTOR_T(Com_BehaviorTree);
		CONSTRUCTOR_T(Com_Renderer_Mesh);
		CONSTRUCTOR_T(Com_Light3D);
		CONSTRUCTOR_T(Com_Camera);
		CONSTRUCTOR_T(Com_Renderer_3DAnimMesh);
		CONSTRUCTOR_T(Com_Renderer_ParticleSystem);
		CONSTRUCTOR_T(Com_Transform);
		CONSTRUCTOR_T(Com_Animator2D);
	}
}