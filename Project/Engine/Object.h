#pragma once
#include "Scene.h"
#include "Layer.h"
#include "GameObject.h"
#include "SceneManager.h"
#include "Com_Transform.h"

namespace mh::object
{
	template <typename T>
	static T* Instantiate(define::eLayerType _type)
	{
		T* gameObject = new T();
		Scene* scene = SceneManager::GetActiveScene();
		Layer& layer = scene->GetLayer(_type);
		layer.AddGameObject(gameObject);
		gameObject->Init();


		return gameObject;
	}

	template <typename T>
	static T* Instantiate(define::eLayerType _type, Scene* _scene)
	{
		T* gameObject = new T();
		Layer& layer = _scene->GetLayer(_type);
		layer.AddGameObject(gameObject);

		return gameObject;
	}

	template <typename T>
	static T* Instantiate(define::eLayerType _type, Com_Transform* _parent)
	{
		T* gameObject = new T();
		Scene* scene = SceneManager::GetActiveScene();
		Layer& layer = scene->GetLayer(_type);
		layer.AddGameObject(gameObject);

		Com_Transform& tr = gameObject->GameObject::GetTransform();
		tr.SetOwner(_parent);

		return gameObject;
	}

	template <typename T>
	static T* Instantiate(define::eLayerType _type, float3 _position, float3 _rotation)
	{
		T* gameObject = new T();
		Scene* scene = SceneManager::GetActiveScene();
		Layer& layer = scene->GetLayer(_type);
		layer.AddGameObject(gameObject);

		Com_Transform& tr = gameObject->GameObject::GetTransform();
		tr.SetPosition(_position);
		tr.SetRotation(_rotation);

		return gameObject;
	}

	template <typename T>
	static T* Instantiate(define::eLayerType _type, float3 _position, float3 _rotation, Com_Transform* _parent)
	{
		T* gameObject = new T();
		Scene* scene = SceneManager::GetActiveScene();
		Layer& layer = scene->GetLayer(_type);
		layer.AddGameObject(gameObject);

		Com_Transform& tr = gameObject->GameObject::GetTransform();
		tr.SetPosition(_position);
		tr.SetRotation(_rotation);
		tr.SetOwner(_parent);

		return gameObject;
	}

	static void Destroy(GameObject* _gameObject)
	{
		_gameObject->Death();
	}

	static void DontDestroyOnLoad(GameObject* _gameObject)   //씬 이동시 이 오브젝트는 삭제하지 않는다
	{
		if (_gameObject == nullptr)
			return;

		_gameObject->DontDestroy(true);
	}
}
