/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

#include <G3D/Matrix3.h>
#include <G3D/Vector3.h>
#include <G3D/AABox.h>
#include <G3D/Ray.h>

#include "Platform/Define.h"

namespace VMAP
{
    class WorldModel;
}

class GameObject;
struct GameObjectDisplayInfoEntry;

class ModelInstance_Overriden /*, public Intersectable*/
{
    uint32 phasemask;
    G3D::AABox iBound;
    G3D::Matrix3 iInvRot;
    G3D::Vector3 iPos;
    //G3D::Vector3 iRot;
    float iInvScale;
    float iScale;
    VMAP::WorldModel *iModel;

    ModelInstance_Overriden() : phasemask(0), iModel(NULL) {}
    bool initialize(const GameObject & go, const GameObjectDisplayInfoEntry& info);

public:
    std::string name;

    const G3D::AABox& getBounds() const { return iBound; }

    ~ModelInstance_Overriden();

    const G3D::Vector3& getPosition() const { return iPos;}

    /**	Enables\disables collision. */
    void disable() { phasemask = 0;}
    void enable(uint32 ph_mask) { phasemask = ph_mask;}

    bool intersectRay(const G3D::Ray& Ray, float& MaxDist, bool StopAtFirstHit, uint32 ph_mask) const;

    static ModelInstance_Overriden* construct(const GameObject & go);
};
