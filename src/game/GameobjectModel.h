#pragma once

#include "ModelInstance.h"

using G3D::Vector3;

class ModelInstance_Overriden : public VMAP::ModelInstance
{
    ModelInstance_Overriden() {}
    bool initialize(const class GameObject & go, const struct GameObjectDisplayInfoEntry& info);

public:

    ~ModelInstance_Overriden();

    const Vector3& getPosition() const { return iPos;}

    static ModelInstance_Overriden* construct(const class GameObject & go, const struct GameObjectDisplayInfoEntry& info);
};
