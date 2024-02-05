#pragma once

#include <entt.hpp>

#include "Framework/Common.h"

#include "Components.h"

struct EntityRelationsComponent;
struct TagComponent;

class Entity
{
    friend class Level;

public:
    Entity() = default;

    bool IsValid() const;

    std::string_view GetName();

    bool HasParent();
    Entity GetParent();
    bool HasChildren();
    const std::vector<Entity>& GetChildren();

    template<typename T>
    bool HasComponent() const
    {
        Assert(IsValid());
        return nullptr != m_enttRegistry->try_get<T>(m_enttHandle);
    }

    template<typename T>
    T& GetComponent()
    {
        Assert(IsValid() && HasComponent<T>());
        return m_enttRegistry->get<T>(m_enttHandle);
    }

    template<typename T, typename... Args>
    T& GetOrAddComponent(Args... args)
    {
        return m_enttRegistry->get_or_emplace<T>(m_enttHandle, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        Assert(IsValid() && !HasComponent<T>());
        return m_enttRegistry->emplace<T>(m_enttHandle, std::forward<Args>(args)...);
    }

    template<typename T>
    void RemoveComponent()
    {
        Assert(entt::type_hash<T>::value != entt::type_hash<TagComponent>::value, "You cannot remove TagComponent");
        Assert(entt::type_hash<T>::value != entt::type_hash<EntityRelationsComponent>::value, "You cannot remove EntityRelationsComponent");
        Assert(IsValid() && HasComponent<T>());
        m_enttRegistry->remove<T>(m_enttHandle);
    }

    bool operator==(const Entity& other) const;
    bool operator!=(const Entity& other) const;

    size_t GetId();

private:
    Entity(entt::registry* enttRegistry, std::string_view name);

private:
    entt::entity m_enttHandle{};
    entt::registry* m_enttRegistry = nullptr;
};

struct EntityRelationsComponent
{
    Entity parent;
    std::vector<Entity> children;

    void AddChild(Entity child);
    void RemoveChild(Entity child);
};