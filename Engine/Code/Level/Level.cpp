#include "Level.h"

#include <Framework/Common.h>

Entity Level::CreateEntity(std::string_view name, Entity parent)
{
    Entity newEntity(&m_enttRegistry, name);

    if (parent.IsValid())
    {
        EntityRelationsComponent& relations = newEntity.AddComponent<EntityRelationsComponent>();
        relations.parent = parent;
        parent.GetOrAddComponent<EntityRelationsComponent>().children.push_back(newEntity);
    }
    else
    {
        m_rootEntities.push_back(newEntity);
    }

    m_allEntities.push_back(newEntity);

    return newEntity;
}

void Level::DestroyEntity(Entity entity)
{
    Assert(m_enttRegistry.valid(entity.m_enttHandle), "Provided entity is not valid");

    Entity parent = entity.GetParent();
    if (parent.IsValid())
    {
        parent.GetComponent<EntityRelationsComponent>().RemoveChild(entity);
    }
    else
    {
        std::erase(m_rootEntities, entity);
    }

    std::erase(m_allEntities, entity);

    m_enttRegistry.destroy(entity.m_enttHandle);
}

const std::vector<Entity>& Level::GetRootEntities() const
{
    return m_rootEntities;
}

const std::vector<Entity>& Level::GetAllEntities() const
{
    return m_allEntities;
}

LevelPtr Level::Create()
{
    return std::make_unique<Level>();
}