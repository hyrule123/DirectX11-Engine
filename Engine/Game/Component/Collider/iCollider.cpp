#include "Engine/Game/Component/Collider/iCollider.h"

#include "Engine/Game/GameObject.h"

namespace ehw
{
	iCollider::iCollider(eColliderType _type)
		: m_colliderType(_type)
	{
	}

	iCollider::~iCollider()
	{

	}
	void iCollider::Contacted(const std::shared_ptr<iCollider>& _col, const Vector2 _contactPoint)
	{
		//자신의 충돌만을 처리
		//저쪽은 CollisionManager 쪽에서 알아서 처리해줄것임.
		const auto& iter = m_prevContacts.find(_col);

		m_curContacts.insert(_col);

		//첫 충돌
		if (m_prevContacts.end() == iter)
		{
			OnCollisionEnter(_col, _contactPoint);
			
			//TODO : 중복 충돌함수 호출 방지(저쪽에서 이쪽 충돌함수를 호출하는 일이 없도록)
			//중복 충돌은 CollisionManager 단에서 처리할것
		}

		//충돌 중
		else
		{
			//지난 충돌 map에서 제거(여기 남아있으면 충돌 해제된걸로 판정함)
			m_prevContacts.erase(iter);
			OnCollisionStay(_col, _contactPoint);
		}
	}
	void iCollider::CollisionUpdate()
	{
		//여기서 prevContacts에 남아있는 애들은 이번에 충돌 안한거임 -> 충돌 해제
		for (const auto& iter : m_prevContacts)
		{
			OnCollisionExit(iter);
		}

		//충돌 해체 처리 뒤 이번 프레임 충돌했던 충돌체들을 swap
		m_prevContacts.clear();
		m_prevContacts.swap(m_curContacts);
	}


}

