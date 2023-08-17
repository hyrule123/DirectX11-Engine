#pragma once
#include "Entity.h"

#include "define_Component.h"
#include "SimpleMath.h"

namespace mh
{
	class GameObject;
	class IComponent : public Entity
	{
	public:
		IComponent(define::eComponentType _type);

		IComponent(const IComponent& _other);
		CLONE_DISABLE(IComponent);

		virtual ~IComponent();

		virtual define::eResult SaveJson(Json::Value* _pJson) override;
		virtual define::eResult LoadJson(const Json::Value* _pJson) override;

		virtual void Init() {};
		virtual void Update() {};
		virtual void FixedUpdate() = 0;

		define::eComponentType GetComType() const { return mType; };

		GameObject* GetOwner() const { return mOwner; }
		void SetOwner(GameObject* _owner) { mOwner = _owner; }

	private:
		const define::eComponentType mType;
		GameObject* mOwner;
	};
}
