#include "Entity.h"

#include <Framework/Common.h>

#include "Components.h"

bool Entity::IsValid() const
{
    return m_enttHandle != entt::null && m_enttRegistry != nullptr;
}

std::string_view Entity::GetName()
{
    return GetComponent<TagComponent>().tag;
}

bool Entity::HasParent()
{
    if (!HasComponent<EntityRelationsComponent>())
    {
        return false;
    }

    return GetComponent<EntityRelationsComponent>().parent.IsValid();
}

Entity Entity::GetParent()
{
    return GetComponent<EntityRelationsComponent>().parent;
}

bool Entity::HasChildren()
{
    if (!HasComponent<EntityRelationsComponent>())
    {
        return false;
    }

    return !GetComponent<EntityRelationsComponent>().children.empty();
}

const std::vector<Entity>& Entity::GetChildren()
{
    if (HasComponent<EntityRelationsComponent>())
    {
        return GetComponent<EntityRelationsComponent>().children;
    }

    static std::vector<Entity> noChildren;
    return noChildren;
}

bool Entity::operator==(const Entity& other) const
{
    return m_enttHandle == other.m_enttHandle && m_enttRegistry == other.m_enttRegistry;
}

bool Entity::operator!=(const Entity& other) const
{
    return !(*this == other);
}

size_t Entity::GetId()
{
    return static_cast<size_t>(m_enttHandle);
}

Entity::Entity(entt::registry* enttRegistry, std::string_view name)
    : m_enttRegistry(enttRegistry)
{
    m_enttHandle = m_enttRegistry->create();

    Assert(m_enttHandle != entt::null, "Failed to create an entity");

    AddComponent<TagComponent>().tag = name;
    AddComponent<TransformComponent>();
}

void EntityRelationsComponent::AddChild(Entity child)
{
    Assert(std::find(children.begin(), children.end(), child) == children.end(), "Entity is already a child!");
    children.push_back(child);
}

void EntityRelationsComponent::RemoveChild(Entity child)
{
    Assert(std::find(children.begin(), children.end(), child) != children.end(), "Entity does not have this child!");
    std::erase(children, child);
}