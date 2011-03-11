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

#include "Creature.h"
#include "MapManager.h"
#include "RandomMovementGenerator.h"
#include "DestinationHolderImp.h"
#include "Map.h"
#include "Util.h"
#include "Movement/UnitMovement.h"

extern void GeneratePath(const Map*, G3D::Vector3, const G3D::Vector3&, std::vector<G3D::Vector3>&);

using G3D::Vector3;

bool GenerateCoord(const Creature &creature, Vector3& v)
{
    float respX, respY, respZ, respO, currZ, destX, destY, destZ, wander_distance, travelDistZ;

    creature.GetRespawnCoord(respX, respY, respZ, &respO, &wander_distance);

    currZ = creature.GetPositionZ();
    TerrainInfo const* map = creature.GetTerrain();

    // For 2D/3D system selection
    //bool is_land_ok  = creature.CanWalk();                // not used?
    //bool is_water_ok = creature.CanSwim();                // not used?
    bool is_air_ok = creature.CanFly();

    const float angle = rand_norm_f() * (M_PI_F*2.0f);
    const float range = rand_norm_f()*wander_distance*4.f;
    const float distanceX = range * cos(angle);
    const float distanceY = range * sin(angle);

    destX = respX + distanceX;
    destY = respY + distanceY;

    // prevent invalid coordinates generation
    MaNGOS::NormalizeMapCoord(destX);
    MaNGOS::NormalizeMapCoord(destY);

    travelDistZ = distanceX*distanceX + distanceY*distanceY;

    if (is_air_ok)                                          // 3D system above ground and above water (flying mode)
    {
        // Limit height change
        const float distanceZ = rand_norm_f() * sqrtf(travelDistZ)/2.0f;
        destZ = respZ + distanceZ;
        float levelZ = map->GetWaterOrGroundLevel(destX, destY, destZ-2.0f);

        // Problem here, we must fly above the ground and water, not under. Let's try on next tick
        if (levelZ >= destZ)
            return false;
    }
    //else if (is_water_ok)                                 // 3D system under water and above ground (swimming mode)
    else                                                    // 2D only
    {
        // 10.0 is the max that vmap high can check (MAX_CAN_FALL_DISTANCE)
        travelDistZ = travelDistZ >= 100.0f ? 10.0f : sqrtf(travelDistZ);

        // The fastest way to get an accurate result 90% of the time.
        // Better result can be obtained like 99% accuracy with a ray light, but the cost is too high and the code is too long.
        destZ = map->GetHeight(destX, destY, respZ+travelDistZ-2.0f, false);

        if (fabs(destZ - respZ) > travelDistZ)              // Map check
        {
            // Vmap Horizontal or above
            destZ = map->GetHeight(destX, destY, respZ - 2.0f, true);

            if (fabs(destZ - respZ) > travelDistZ)
            {
                // Vmap Higher
                destZ = map->GetHeight(destX, destY, respZ+travelDistZ-2.0f, true);

                // let's forget this bad coords where a z cannot be find and retry at next tick
                if (fabs(destZ - respZ) > travelDistZ)
                    return false;
            }
        }
    }
    v = Vector3(destX,destY,destZ);
    return true;
}

enum{
    RANDOM_POINTS_MIN = 1,
    RANDOM_POINTS_MAX = 4,
};

template<>
void
RandomMovementGenerator<Creature>::_setRandomLocation(Creature &creature)
{
    Vector3 v;
    Movement::PointsArray path;
    uint32 points_count = urand(RANDOM_POINTS_MIN, RANDOM_POINTS_MAX);
    for (uint32 i = 0; i < points_count; ++i)
    {
        if (!GenerateCoord(creature, v))
            return;
        path.push_back(v);
    }
    
    {
        using namespace Movement;

        UnitMovement& state = *creature.movement;

        MoveSplineInit init(state);
        if (state.HasMode(MoveModeLevitation) || state.HasMode(MoveModeFly))
        {
            init.SetFly();
            init.MovebyPath(path);
            // walk mode - 90% chance, hovever not offlike
            state.ApplyWalkMode(urand(0,3));
        }
        else
        {
            //PointsArray path;
            //GeneratePath(creature.GetMap(),state.GetPosition3(),Vector3(destX, destY, destZ), path);
            // walk mode - 90% chance, hovever not offlike
            state.ApplyWalkMode(urand(0,9));
            init.MovebyPath(path);
        }
        init.Launch();
    }

    creature.addUnitState(UNIT_STAT_ROAMING_MOVE);
}

template<class T>
void RandomMovementGenerator<T>::OnSplineDone( Unit& unit )
{
    using namespace Movement;
    if (unit.movement->HasMode(MoveModeLevitation)||unit.movement->HasMode(MoveModeFly))
    {
        i_nextMoveTime.Reset(10000);
    }
    //else if (is_water_ok)                                 // Swimming mode to be done with more than this check
    else
    {
        i_nextMoveTime.Reset(urand(500,10000));
    }
}

template<>
void RandomMovementGenerator<Creature>::Initialize(Creature &creature)
{
    if (!creature.isAlive())
        return;

    creature.addUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
    _setRandomLocation(creature);
}

template<>
void RandomMovementGenerator<Creature>::Reset(Creature &creature)
{
    Initialize(creature);
}

template<>
void RandomMovementGenerator<Creature>::Interrupt(Creature &creature)
{
    creature.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

template<>
void RandomMovementGenerator<Creature>::Finalize(Creature &creature)
{
    creature.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

template<>
bool RandomMovementGenerator<Creature>::Update(Creature &creature, const uint32 &diff)
{
    if (creature.hasUnitState(UNIT_STAT_NOT_MOVE))
    {
        i_nextMoveTime.Update(i_nextMoveTime.GetExpiry());  // Expire the timer
        creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    if (!IsActive(creature))                        // force stop processing (movement can move out active zone with cleanup movegens list)
        return true;                                // not expire now, but already lost

    if (!i_nextMoveTime.Passed())
    {
        i_nextMoveTime.Update(diff);

        if (i_nextMoveTime.Passed())
            _setRandomLocation(creature);
    }

    return true;
}

template<>
bool RandomMovementGenerator<Creature>::GetResetPosition(Creature& c, float& x, float& y, float& z)
{
    float radius;
    c.GetRespawnCoord(x, y, z, NULL, &radius);

    // use current if in range
    if (c.IsWithinDist2d(x,y,radius))
        c.GetPosition(x,y,z);

    return true;
}