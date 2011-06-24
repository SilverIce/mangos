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
#include "Map.h"
#include "Util.h"
#include "Movement/UnitMovement.h"

extern void GeneratePath(const WorldObject*, G3D::Vector3, const G3D::Vector3&, std::vector<G3D::Vector3>&, bool isFlight = false);

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
    uint32 points_count = urand(RANDOM_POINTS_MIN, RANDOM_POINTS_MAX);
  
    {
        using namespace Movement;
        UnitMovement& mov = *creature.movement;
        MoveCommonInit init(mov);
        init.Path().push_back(Vector3());
        for (int i = 0; i < points_count; ++i)
        {
            Vector3 v;
            if (GenerateCoord(creature, v))
                init.Path().push_back(v);
        }
        init.Launch();
        mySpline = mov.MoveSplineId();
    }

    creature.addUnitState(UNIT_STAT_ROAMING_MOVE);
}

template<class T>
void RandomMovementGenerator<T>::OnEvent(Unit& unit, const Movement::OnEventArgs& args)
{
    if (mySpline != args.splineId)
    {
        return;
    }
    if (args.isArrived())
        i_nextMoveTime.Reset(urand(500,10000));
}

template<>
void RandomMovementGenerator<Creature>::Initialize(Creature &creature)
{
    if (!creature.isAlive())
        return;

    creature.movement->ApplyWalkMode(true);
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
    Finalize(creature);
}

template<>
void RandomMovementGenerator<Creature>::Finalize(Creature &creature)
{
    creature.movement->ApplyWalkMode(false);
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
