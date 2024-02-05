#pragma once

#include "Entity.h"

class Level;
using LevelPtr = std::unique_ptr<Level>;

class Level
{
public:
    Level() = default;

    Entity CreateEntity(std::string_view name, Entity parent = {});
    void DestroyEntity(Entity entity);

    const std::vector<Entity>& GetRootEntities() const;
    const std::vector<Entity>& GetAllEntities() const;

    static LevelPtr Create();

private:
    entt::registry m_enttRegistry;
    std::vector<Entity> m_rootEntities;
    std::vector<Entity> m_allEntities;
};