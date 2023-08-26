#pragma once
#include "tspch.h"
#include "Entity.h"

namespace TS_ENGINE
{
	typedef std::vector<Ref<Entity>> EntityCollection;
	typedef EntityCollection::size_type EntityCollectionIndex;

	class EntityManager
	{
	private:
		EntityCollection mEntities;
		std::unordered_map<EntityID, EntityCollectionIndex> mEntityLookUp;
		
		static Ref<EntityManager> mInstance;
	public:
		static Ref<EntityManager> GetInstance();

		Ref<Entity> Initialize(const std::string& name);// , EntityType entityType);
		Ref<Entity> Get(EntityID id);

		void Remove(EntityID id);
	};
}

