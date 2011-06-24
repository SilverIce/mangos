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

#include "HomeMovementGenerator.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "Movement/UnitMovement.h"

extern void GeneratePath(const WorldObject*, G3D::Vector3, const G3D::Vector3&, std::vector<G3D::Vector3>&, bool);

void
HomeMovementGenerator<Creature>::Initialize(Creature & owner)
{
    _setTargetLocation(owner);
}

void
HomeMovementGenerator<Creature>::Reset(Creature &)
{
}

void
HomeMovementGenerator<Creature>::_setTargetLocation(Creature & owner)
{
    if (owner.hasUnitState(UNIT_STAT_NOT_MOVE))
        return;

    float x, y, z, o;
    
    // at apply we can select more nice return points base at current movegen
    bool isRespawnMove = (owner.GetMotionMaster()->empty() || !owner.GetMotionMaster()->top()->GetResetPosition(owner,x,y,z));
    if (isRespawnMove)
        owner.GetRespawnCoord(x, y, z, &o);
    
    {
        arrived = false;

        using namespace Movement;
        UnitMovement& state = *owner.movement;
        MoveCommonInit init(state);
        if (isRespawnMove)
            init.SetFacing(o);
        GeneratePath(&owner,state.GetPosition3(),Vector3(x,y,z), init.Path(), state.IsFlying());
        init.Launch();
        mySpline = state.MoveSplineId();
    }

    owner.clearUnitState(UNIT_STAT_ALL_STATE);
}

bool HomeMovementGenerator<Creature>::Update(Creature &owner, const uint32& time_diff)
{
    if (arrived)
        ((Creature&)owner).AI()->JustReachedHome();
    return !arrived;
}

void HomeMovementGenerator<Creature>::OnEvent(Unit& unit, const Movement::OnEventArgs& args)
{
    if (mySpline != args.splineId)
    {
        return;
    }

    arrived = args.isArrived();
}

