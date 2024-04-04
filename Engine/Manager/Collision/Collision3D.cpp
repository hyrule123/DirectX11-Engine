#include "Collision3D.h"

#if defined (_WIN64) && (_DEBUG)
#pragma comment(lib, "Physx/Library/Debug/PhysX_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXCharacterKinematic_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXCommon_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXCooking_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXExtensions_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXFoundation_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXTask_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXVehicle_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PhysXVehicle2_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/PVDRuntime_64.lib")
#pragma comment(lib, "Physx/Library/Debug/SceneQuery_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/SimulationController_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/LowLevel_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/LowLevelAABB_static_64.lib")
#pragma comment(lib, "Physx/Library/Debug/LowLevelDynamics_static_64.lib")

#elif defined (_WIN64) && !(_DEBUG)

#pragma comment(lib, "Physx/Library/Release/PhysX_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXCharacterKinematic_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXCommon_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXCooking_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXExtensions_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXFoundation_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXTask_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXVehicle_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/PhysXVehicle2_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/PVDRuntime_64.lib")
#pragma comment(lib, "Physx/Library/Release/SceneQuery_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/SimulationController_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/LowLevel_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/LowLevelAABB_static_64.lib")
#pragma comment(lib, "Physx/Library/Release/LowLevelDynamics_static_64.lib")

#endif

#include "Engine/Game/iScene.h"
#include "Engine/Game/GameObject.h"

#include "Engine/Game/Component/Transform/Com_Transform.h"
#include "Engine/Game/Component/Collider/Com_Collider3D.h"

#include "Engine/Manager/CollisionManager.h"
#include "Engine/Manager/TimeManager.h"

#include "Engine/Manager/Collision/MathUtil.h"

namespace ehw
{
	using namespace ::physx;
	Collision3D::Collision3D()
		: m_allocator{ }
		, m_errorCallback{ }
		, m_foundation{ nullptr }
		, m_physics{ nullptr }
		, m_dispatcher{ nullptr }
		, m_currentScene{ nullptr }
		, m_material{ nullptr }
		, m_curUpdateInterval{ UpdateInterval::Frame_60 }
	{

	}

	Collision3D::~Collision3D()
	{
		for (auto& scene : m_physxScenes)
		{
			const PxU32 actorCount = scene.second->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
			if (actorCount > 0)
			{
				std::vector<PxRigidActor*> actors(actorCount);
				scene.second->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(actors.data()), actorCount);
				for (const auto& actor : actors)
				{
					GameObject* gameObject = static_cast<GameObject*>(actor->userData);
					if (gameObject)
					{
						gameObject->GetComponent<Com_Collider3D>()->DestroyShape();
					}
						
				}
			}
			PX_RELEASE(scene.second);
		}
		PX_RELEASE(m_dispatcher);
		PX_RELEASE(m_physics);
#ifdef _DEBUG
		PxCloseExtensions();
		if (m_PVD)
		{
			PxPvdTransport* transport = m_PVD->getTransport();
			PX_RELEASE(m_PVD);
			PX_RELEASE(transport);
		}
#endif
		PX_RELEASE(m_foundation);
	}

	void Collision3D::Init()
	{
		m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);

		bool enable = false;
#ifdef _DEBUG
		enable = true;
		m_PVD = PxCreatePvd(*m_foundation);
		PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		m_PVD->connect(*transport, PxPvdInstrumentationFlag::eALL);
#endif
		m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale{}, enable, m_PVD);

#ifdef _DEBUG
		PxInitExtensions(*m_physics, m_PVD);
#endif
		constexpr uint32 kThreadCount = 2;
		m_dispatcher = PxDefaultCpuDispatcherCreate(kThreadCount);
		m_material = m_physics->createMaterial(0.5f, 0.5f, 0.6f);

	}

	void Collision3D::CollisionUpdate()
	{
		if (m_currentScene == nullptr)
			return;

		//m_currentScene->simulate(_intervals[static_cast<uint8>(m_curUpdateInterval)]);
		m_currentScene->simulate(TimeManager::DeltaTime());
		m_currentScene->fetchResults(true);

		//SyncGameScene();
	}


	void Collision3D::SyncGameScene() const
	{
		const PxU32 actorCount = m_currentScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);

		if (actorCount == 0)
		{
			return;
		}

		std::vector<PxRigidActor*> actors(actorCount);
		m_currentScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(actors.data()), actorCount);

		for (const auto& actor : actors)
		{
			const PxTransform worldTransform = actor->getGlobalPose();

			GameObject* gameObject = static_cast<GameObject*>(actor->userData);

			if (gameObject == nullptr || gameObject->IsDestroyed())
			{
				continue;
			}
				
			// 충돌체에 대해서 일어난 변화이므로, 이를 Com_Transform에도 반영해야 한다.
			const Com_Collider3D* collider = gameObject->GetComponent<Com_Collider3D>();
			if (collider->IsTrigger())
			{
				continue;
			}
				

			//원래 위치 정보에서 크기정보 제거
			const MATRIX from = MATRIX::CreateScale(collider->GetWorldScale()).Invert() * collider->GetWorldMatrix();

			//피직스에서 받아온 트랜스폼 정보를 가져온다
			const MATRIX to = MathUtil::ToMATRIX(PxMat44{ worldTransform });

			//차이를 구해준다
			const MATRIX diff = from.Invert() * to;

			// 이동은 회전에 영향을 받으므로
			const Quaternion rotation = Quaternion::CreateFromRotationMatrix(diff);
			
			const float3	 position = to.Translation() - float3::Transform(from.Translation(), rotation);

			Com_Transform* transform = gameObject->GetComponent<Com_Transform>();
			transform->SetWorldPosition(float3::Transform(transform->GetWorldPosition(), rotation) + position);
			transform->SetWorldRotation(transform->GetWorldRotation() * rotation);
		}
	}

	void Collision3D::onTrigger(PxTriggerPair* pairs, PxU32 count)
	{
		for (PxU32 i = 0; i < count; ++i)
		{
			const PxTriggerPair& contactpair = pairs[i];

			if (contactpair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
				continue;

			// if these actors do not exist in the scene anymore due to deallocation, do not process
			if (contactpair.triggerActor->userData == nullptr || contactpair.otherActor->userData == nullptr)
			{
				continue;
			}

			GameObject* triggerObject = static_cast<GameObject*>(contactpair.triggerActor->userData);
			assert(triggerObject);
			GameObject* otherObject = static_cast<GameObject*>(contactpair.otherActor->userData);
			assert(otherObject);
			if (triggerObject->IsDestroyed() || otherObject->IsDestroyed())
			{
				continue;
			}
				

			Com_Collider3D* triggerCollider = triggerObject->GetComponent<Com_Collider3D>();
			Com_Collider3D* otherCollider = otherObject->GetComponent<Com_Collider3D>();

			// process events
			if (contactpair.status & (PxPairFlag::eNOTIFY_TOUCH_FOUND))
			{
				triggerCollider->OnTriggerEnter(otherCollider);
				otherCollider->OnTriggerEnter(triggerCollider);
			}
			else if (contactpair.status & (PxPairFlag::eNOTIFY_TOUCH_LOST))
			{
				triggerCollider->OnTriggerExit(otherCollider);
				otherCollider->OnTriggerExit(triggerCollider);
			}
		}
	}

	void Collision3D::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{
		for (PxU32 i = 0; i < nbPairs; ++i)
		{
			const PxContactPair& contactpair = pairs[i];

			// if these actors do not exist in the scene anymore due to deallocation, do not process
			if (pairHeader.actors[0]->userData == nullptr || pairHeader.actors[1]->userData == nullptr)
				continue;

			GameObject* leftGameObject = static_cast<GameObject*>(pairHeader.actors[0]->userData);
			GameObject* rightGameObject = static_cast<GameObject*>(pairHeader.actors[1]->userData);
			if (leftGameObject->IsDestroyed() || rightGameObject->IsDestroyed())
				continue;
			else if (leftGameObject == rightGameObject)
				continue;

			Com_Collider3D* leftCollider = leftGameObject->GetComponent<Com_Collider3D>();
			Com_Collider3D* rightCollider = rightGameObject->GetComponent<Com_Collider3D>();

			PxContactPairPoint collisionPoint{};
			const PxU32		   count = contactpair.extractContacts(&collisionPoint, 1);
			const float3	   collisionPosition = MathUtil::Tofloat3(collisionPoint.position);

			// invoke events
			if (contactpair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
			{
				leftCollider->OnCollisionEnter(rightCollider, collisionPosition);
				rightCollider->OnCollisionEnter(leftCollider, collisionPosition);
			}
			else if (contactpair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
			{
				leftCollider->OnCollisionStay(rightCollider, collisionPosition);
				rightCollider->OnCollisionStay(leftCollider, collisionPosition);
			}
			else if (contactpair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
			{
				leftCollider->OnCollisionExit(rightCollider);
				rightCollider->OnCollisionExit(leftCollider);
			}
		}
	}

	void Collision3D::EnableGravity(bool enable, iScene* scene, const float3& gravity) const
	{
		//assert(m_currentScene);

		auto iter = m_physxScenes.find(scene);
		if (iter != m_physxScenes.end())
		{
			iter->second->setGravity(MathUtil::ToPxVec3(gravity));
		}
	}

	void Collision3D::changeGeometry(Com_Collider3D* collider, physx::PxShape* shape, eCollider3D_Shape type)
	{
		const float3 scale = collider->GetWorldScale();
		switch (type)
		{
			
		case eCollider3D_Shape::Box:
			shape->setGeometry(PxSphereGeometry{ scale.x });
			break;
		case eCollider3D_Shape::Sphere:
			shape->setGeometry(PxBoxGeometry{ scale.x, scale.y, scale.z });
			break;
		}
	}

	bool Collision3D::Raycast(uint32 srcLayerIndex, const float3& origin, const float3& direction, float maxDistance, RaycastHit* outHit) const
	{

		std::bitset<32> layer{};
		layer[srcLayerIndex] = true;

		const PxHitFlags  hitFlag = PxHitFlag::eDEFAULT;
		PxQueryFilterData filter{};

		filter.data.word0 = CollisionManager::GetRaycastMask()[srcLayerIndex].to_ulong();

		PxRaycastBuffer hit{};
		const bool		result = m_currentScene->raycast(MathUtil::ToPxVec3(origin), MathUtil::ToPxVec3(direction), maxDistance, hit, hitFlag, filter);

		outHit->gameObject = (hit.hasBlock) ? static_cast<GameObject*>(hit.block.actor->userData) : nullptr;
		outHit->hasBlocking = hit.hasBlock;
		outHit->hitDistance = hit.block.distance;
		outHit->hitPosition = MathUtil::Tofloat3(hit.block.position);
		outHit->hitNormal = MathUtil::Tofloat3(hit.block.normal);

		return result;
	}

	void Collision3D::createScene(iScene* scene)
	{
		assert(scene);
		const auto iter = m_physxScenes.find(scene);
			
			//std::find_if(m_physxScenes.begin(), m_physxScenes.end(), [scene](const PxScene* pxScene) { return pxScene->getName() == scene->GetNameChar(); });

		//이미 생성된 씬이 있을 경우 return
		if (iter != m_physxScenes.end())
		{
			return;
		}

		//assert(iter == m_physxScenes.end());

		PxSceneDesc sceneDescription = PxSceneDesc{ m_physics->getTolerancesScale() };
		sceneDescription.gravity = PxVec3{ 0.f, -0.5f, 0.f };
		sceneDescription.cpuDispatcher = m_dispatcher;
		sceneDescription.filterShader = &Collision3D::FilterShader;
		sceneDescription.simulationEventCallback = this;

		PxScene* newScene = m_physics->createScene(sceneDescription);

		
		newScene->setName(scene->GetStrKey().c_str());

		m_physxScenes.insert(std::make_pair(scene, newScene));
		//m_physxScenes.push_back(newScene);
	}

	void Collision3D::changeScene(iScene* scene)
	{
		const auto iter = m_physxScenes.find(scene);
			//std::find_if(m_physxScenes.begin(), m_physxScenes.end(), [scene](const PxScene* pxScene) {
			//return pxScene->getName() == scene->GetNameChar();
			//});
		//assert(iter != m_physxScenes.end());

		physx::PxScene* pxScene = nullptr;

		if (iter != m_physxScenes.end())
		{
			pxScene = iter->second;
		}
		
		m_currentScene = pxScene;
	}


	void Collision3D::createActorSphere(GameObject* gameObject, float radius, PxShape** outShape, bool isStatic)
	{
		Com_Transform* component = gameObject->GetComponent<Com_Transform>();

		auto iter = m_physxScenes.find(gameObject->GetScene());
		assert(m_physxScenes.end() != iter);

		PxTransform t{};
		t.p = MathUtil::ToPxVec3(component->GetWorldPosition());
		t.q = MathUtil::ToPxQuat(component->GetWorldRotation());

		if (isStatic)
		{
			PxRigidStatic* staticObject = PxCreateStatic(*m_physics, t, PxSphereGeometry{ radius }, *m_material);
			//m_currentScene->addActor(*staticObject);
			iter->second->addActor(*staticObject);

			PxShape* shapes{};
			const PxU32 count = staticObject->getShapes(&shapes, 1);
			assert(count);

			staticObject->userData = component->GetOwner();
			*outShape = &shapes[0];
			
			setupFiltering(*outShape, gameObject->GetLayer());
		}
		else
		{
			PxRigidDynamic* dynamic = PxCreateDynamic(*m_physics, t, PxSphereGeometry{ radius }, *m_material, s_defaultDensity);

			iter->second->addActor(*dynamic);
			//currentScene->addActor(*dynamic);
			dynamic->setAngularDamping(100.f);
			PxShape* shapes{};
			const PxU32 count = dynamic->getShapes(&shapes, 1);
			assert(count);
			dynamic->userData = component->GetOwner();
			*outShape = &shapes[0];
			setupFiltering(*outShape, static_cast<uint32>(gameObject->GetLayer()));
		}
	}

	void Collision3D::createActorCapsule(GameObject* gameObject, float radius, float height, physx::PxShape** outShape, bool isStatic)
	{
		Com_Transform* component =gameObject->GetComponent<Com_Transform>();

		auto iter = m_physxScenes.find(gameObject->GetScene());
		assert(m_physxScenes.end() != iter);

		PxTransform t{};
		t.p = *outShape ? MathUtil::ToPxVec3(component->GetWorldPosition()) : MathUtil::ToPxVec3(component->GetLocalPosition());
		t.q = *outShape ? MathUtil::ToPxQuat(component->GetWorldRotation()) : MathUtil::ToPxQuat(component->GetLocalRotation());

		if (isStatic)
		{
			PxRigidStatic* staticObject = PxCreateStatic(*m_physics, t, PxCapsuleGeometry{ radius, height }, *m_material);
			//m_currentScene->addActor(*staticObject);
			iter->second->addActor(*staticObject);

			PxShape* shapes{};
			const PxU32 count = staticObject->getShapes(&shapes, 1);
			assert(count);
			staticObject->userData = component->GetOwner();
			*outShape = &shapes[0];
			setupFiltering(*outShape, static_cast<uint32>(gameObject->GetLayer()));
		}
		else
		{
			PxRigidDynamic* dynamic = PxCreateDynamic(*m_physics, t, PxCapsuleGeometry{ radius, height }, *m_material, s_defaultDensity);
			dynamic->setAngularDamping(100.f);

			iter->second->addActor(*dynamic);
			//m_currentScene->addActor(*dynamic);

			PxShape* shapes{};
			const PxU32 count = dynamic->getShapes(&shapes, 1);
			assert(count);

			dynamic->userData = component->GetOwner();
			*outShape = &shapes[0];
			setupFiltering(*outShape, static_cast<uint32>(gameObject->GetLayer()));
		}
	}

	void Collision3D::createActorCube(GameObject* gameObject, const float3& halfExtents, PxShape** outShape, bool isStatic)
	{
		Com_Transform* component = gameObject->GetComponent<Com_Transform>();

		auto iter = m_physxScenes.find(gameObject->GetScene());
		assert(m_physxScenes.end() != iter);



		PxTransform t{};
		t.p = *outShape ? 
			MathUtil::ToPxVec3(component->GetWorldPosition()) : MathUtil::ToPxVec3(component->GetLocalPosition());
		t.q = *outShape ? MathUtil::ToPxQuat(component->GetWorldRotation()) : MathUtil::ToPxQuat(component->GetLocalRotation());

		if (isStatic)
		{
			PxRigidStatic* staticObject = PxCreateStatic(*m_physics, t, PxBoxGeometry{ halfExtents.x, halfExtents.y, halfExtents.z }, *m_material);
			//targetScene->addActor(*staticObject);
			iter->second->addActor(*staticObject);

			PxShape* shapes{};
			const PxU32 count = staticObject->getShapes(&shapes, 1);
			assert(count);
			staticObject->userData = component->GetOwner();
			*outShape = &shapes[0];
			setupFiltering(*outShape, static_cast<uint32>(gameObject->GetLayer()));
		}
		else
		{
			PxRigidDynamic* dynamic = PxCreateDynamic(*m_physics, t, PxBoxGeometry{ halfExtents.x, halfExtents.y, halfExtents.z }, *m_material, s_defaultDensity);
			//m_currentScene->addActor(*dynamic);
			iter->second->addActor(*dynamic);

			PxShape* shapes{};
			const PxU32 count = dynamic->getShapes(&shapes, 1);
			assert(count);

			dynamic->userData = component->GetOwner();
			*outShape = &shapes[0];
			setupFiltering(*outShape, gameObject->GetLayer());
		}
	}

	PxFilterFlags Collision3D::FilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags,
		const void* constantBlock, physx::PxU32 constantBlockSize)
	{
		// let triggers through
		if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
		{
			if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
				pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
			else
				return PxFilterFlag::eKILL;

			return PxFilterFlag::eDEFAULT;
		}

		// generate contacts for all that were not filtered above
		// trigger the contact callback for pairs (A,B) where
		// the filtermask of A contains the ID of B or vice versa.
		if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
		{
			pairFlags = PxPairFlag::eCONTACT_DEFAULT;
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eNOTIFY_CONTACT_POINTS;
			return PxFilterFlag::eDEFAULT;
		}

		return PxFilterFlag::eKILL;
	}

	void Collision3D::setupFiltering(PxShape* shape, uint32 layerIndex) const
	{
		std::bitset<32> layer{};

		layer[layerIndex] = true;

		PxFilterData filterData{};

		// word0 = own ID
		filterData.word0 = layer.to_ulong();

		// word1 = ID mask to filter pairs that trigger a contact callback
		filterData.word1 = CollisionManager::GetCollisionMask()[layerIndex].to_ulong(); 
		shape->setSimulationFilterData(filterData);

		PxFilterData queryFilterData{};

		// word0 = own ID
		queryFilterData.word0 = layer.to_ulong();					   

		// word1 = ID mask to filter pairs that trigger a contact callback
		queryFilterData.word1 = CollisionManager::GetRaycastMask()[layerIndex].to_ulong(); 
		shape->setQueryFilterData(queryFilterData);
	}

#pragma region NOT IMPLEMENTED
	void Collision3D::onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)
	{
	}

	void Collision3D::onWake(PxActor** actors, PxU32 count)
	{
	}

	void Collision3D::onSleep(PxActor** actors, PxU32 count)
	{
	}

	void Collision3D::onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count)
	{
	}
#pragma endregion
} // namespace pa 
