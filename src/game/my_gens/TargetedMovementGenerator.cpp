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

#include "ByteBuffer.h"
#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "World.h"

#define SMALL_ALPHA 0.05f

#include <cmath>
#include "Movement/UnitMovement.h"

extern void GeneratePath(const WorldObject*, G3D::Vector3, const G3D::Vector3&, std::vector<G3D::Vector3>&, bool isFlight = false);
using G3D::Vector3;

template<class V>
inline void polar_offset(V& vec, float angle, float dist)
{
    vec.x += cos(angle)*dist;
    vec.y += sin(angle)*dist;
}

template<class V>
inline void polar_offset(V& vec, V& direction, float dist)
{
    vec.x += v.x * dist;
    vec.y += v.y * dist;
}

static float pos_recalc_time = 0.8f;
static float assumption_time_additive_follow = 1.f;
static float assumption_time_additive_chase = 0.f;

inline int intervalComparison (float x, float lowerBound, float upperBound)
{
    if (x < lowerBound) return -1;
    if (x > upperBound) return +1;
    return 0;
}


// 5 yards -- bounding1 + bounding2
// L yards -- X
/*
template<class T>
bool ChaseMovementGenerator<T>::compute_dest(const Unit& in_owner, const Unit& in_target, float i_angle, float i_offset, Vector3& out_dest)
{
    using namespace Movement;
    UnitMovement& me = *in_owner.movement;
    UnitMovement& target = *in_target.movement;

    Vector3 offset = target.GetGlobalPosition() - me.GetGlobalPosition();
    float distance = offset.length();

    float allowed_dist = in_target.GetFloatValue(UNIT_FIELD_COMBATREACH)+in_owner.GetFloatValue(UNIT_FIELD_COMBATREACH)+CONTACT_DISTANCE;
    if (distance <= allowed_dist)
        return false;

    Vector3 unitOffset = offset / distance;

    Vector3 me_dir(me.direction()), target_dir(target.direction());
    // how parallel are the paths of "this" and the quarry
    // (1 means parallel, 0 is pependicular, -1 is anti-parallel)
    float parallelness = me_dir.dot(target_dir);
    // how "forward" is the direction to the quarry
    // (1 means dead ahead, 0 is directly to the side, -1 is straight back)
    float forwardness = me_dir.dot (unitOffset);
    int f = intervalComparison (forwardness,  -0.707f, 0.707f);
    int p = intervalComparison (parallelness, -0.707f, 0.707f);
    const float time_factors[3][3] = 
    {
        {2, 2, 0.5f},        // -1
        {4, 0.8f, 1},        //  0
        {4, 1.8f, 0.85f},     // +1
    };  // -1, 0, 1
    float timeFactor = time_factors[f+1][p+1];

    float directTravelTime = distance / me.GetCurrentSpeed();

    float time = std::min(directTravelTime * timeFactor, 10.f);
    Vector3 future_pos = target.GetGlobalPosition() + (time * target.GetCurrentSpeed()) * target_dir;

    //polar_offset(new_dest, atan2(new_dest), allowed_dist);
    future_pos = future_pos + (me.GetGlobalPosition()-future_pos).fastDirection() * allowed_dist;

    if (!MaNGOS::IsValidMapCoord(future_pos.x,future_pos.y))
        return false;

    out_dest = future_pos;
    return true;
}*/

template<class T>
bool ChaseMovementGenerator<T>::compute_dest(const Unit& in_owner, const Unit& in_target, float i_angle, float i_offset, Vector3& out_dest)
{
    using namespace Movement;
    UnitMovement& me = *in_owner.movement;
    UnitMovement& target = *in_target.movement;

    Vector3 target_velocity = target.direction() * target.GetCurrentSpeed();

    float move_time = me.MoveSplineTimeElapsed()*0.001f + assumption_time_additive_chase;
    Vector3 target_future = target.GetGlobalPosition() + move_time * target_velocity;

    float distance = (target_future - me.MoveSplineDest()).length();
    float allowed_dist = in_target.GetFloatValue(UNIT_FIELD_COMBATREACH)+in_owner.GetFloatValue(UNIT_FIELD_COMBATREACH)+CONTACT_DISTANCE;

    if (distance <= allowed_dist )
        return false;

    move_time = (me.GetGlobalPosition()-target.GetGlobalPosition()).length() / me.GetCurrentSpeed() + assumption_time_additive_chase;
    target_future = target.GetGlobalPosition() + move_time * target_velocity;
    target_future += (me.GetGlobalPosition()-target_future).fastDirection() * allowed_dist;

    if (!MaNGOS::IsValidMapCoord(target_future.x,target_future.y))
        return false;
    out_dest = target_future;
    return true;
}
// Follow movement specialization:
template<class T>
bool FollowMovementGenerator<T>::compute_dest(const Unit& in_owner, const Unit& in_target, float i_angle, float i_offset, Vector3& out_dest)
{
    using namespace Movement;
    UnitMovement& me = *in_owner.movement;
    UnitMovement& target = *in_target.movement;

    float move_time = me.MoveSplineTimeElapsed()*0.001f + assumption_time_additive_follow;
    Vector3 target_velocity = target.direction() * target.GetCurrentSpeed();
    Vector3 target_future = target.GetGlobalPosition() + move_time * target_velocity;

    float distance = (target_future - me.MoveSplineDest()).length();
    float allowed_dist = in_target.GetFloatValue(UNIT_FIELD_COMBATREACH)+in_owner.GetFloatValue(UNIT_FIELD_COMBATREACH)+CONTACT_DISTANCE;

    if (distance <= allowed_dist )
        return false;

    move_time = (me.GetGlobalPosition()-target.GetGlobalPosition()).length() / me.GetCurrentSpeed() + assumption_time_additive_follow;
    target_future = target.GetGlobalPosition() + move_time * target_velocity;
    target_future += (me.GetGlobalPosition()-target_future).fastDirection() * allowed_dist * 0.8f;

    if (!MaNGOS::IsValidMapCoord(target_future.x,target_future.y))
        return false;
    out_dest = target_future;
    return true;
}

//-----------------------------------------------//
template<class T, typename D>
void TargetedMovementGeneratorMedium<T,D>::_moveToTarget(T &owner)
{
    if (!i_target.isValid() || !i_target->IsInWorld() || owner.hasUnitState(UNIT_STAT_NOT_MOVE))
        return;

    Vector3 new_dest;
    if (!D::compute_dest(owner, *i_target.getTarget(), i_angle, i_offset, new_dest))
        return;

    arrived = false;
    {
        using namespace Movement;
        UnitMovement& mov = *owner.movement;
        MoveCommonInit init(mov);
        GeneratePath(&owner,mov.GetPosition3(),new_dest, init.Path(), mov.IsFlying());
        //init.SetWalk(D::use_walk_mode(i_target.getTarget()));
        init.Launch();
    }

    D::_addUnitStateMove(owner);
}

template<>
void TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // nothing to do for Player
}

template<>
void TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // nothing to do for Player
}

template<>
void TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<>
void TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T,D>::Update(T &owner, const uint32 & time_diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return false;

    if (!owner.isAlive())
        return true;

    if (owner.hasUnitState(UNIT_STAT_NOT_MOVE))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    // prevent movement while casting spells with cast time or channel time
    if (owner.IsNonMeleeSpellCasted(false, false,  true))
    {
        if (!owner.IsStopped())
            owner.StopMoving();
        return true;
    }

    // prevent crash after creature killed pet
    if (static_cast<D*>(this)->_lostTarget(owner))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    if (owner.IsStopped())
    {
        D::_addUnitStateMove(owner);

        _moveToTarget(owner);
        return true;
    }

    i_distance_check.Update(time_diff);
    if (i_distance_check.Passed())
    {
        i_distance_check.Reset(pos_recalc_time);
        _moveToTarget(owner);
    }
        
    
/*
    if ((owner.IsStopped() && !i_destinationHolder.HasArrived()) || i_recalculateTravel)
    {
        i_recalculateTravel = false;

        owner.StopMoving();
        static_cast<D*>(this)->_reachTarget(owner);
    }
*/
    return true;
}

//-----------------------------------------------//
template<class T>
void ChaseMovementGenerator<T>::_reachTarget(T &owner)
{
    if(owner.CanReachWithMeleeAttack(this->i_target.getTarget()))
        owner.Attack(this->i_target.getTarget(),true);
}

template<>
void ChaseMovementGenerator<Player>::Initialize(Player &owner)
{
    owner.addUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
    _moveToTarget(owner);
}

template<>
void ChaseMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.addUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
    _moveToTarget(owner);
}

template<class T>
void ChaseMovementGenerator<T>::Finalize(T &owner)
{
    owner.clearUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::Interrupt(T &owner)
{
    owner.clearUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

//-----------------------------------------------//
template<class T>
bool FollowMovementGenerator<T>::use_walk_mode(const Unit* target)
{
    if (target)
        return target->movement->IsWalking();
    else
        return false;
}

template<>
void FollowMovementGenerator<Creature>::_updateWalkMode(Creature &u)
{
    if (i_target.isValid() && u.IsPet())
        u.UpdateWalkMode(i_target.getTarget());
}

template<>
void FollowMovementGenerator<Player>::_updateWalkMode(Player &)
{
}

template<>
void FollowMovementGenerator<Player>::_updateSpeed(Player &/*u*/)
{
    // nothing to do for Player
}

template<>
void FollowMovementGenerator<Creature>::_updateSpeed(Creature &u)
{
    // pet only sync speed with owner
    if (!((Creature&)u).IsPet() || !i_target.isValid() || i_target->GetObjectGuid() != u.GetOwnerGuid())
        return;

    u.UpdateSpeed(MOVE_RUN,true);
    u.UpdateSpeed(MOVE_WALK,true);
    u.UpdateSpeed(MOVE_SWIM,true);
}

template<>
void FollowMovementGenerator<Player>::Initialize(Player &owner)
{
    owner.addUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    //_updateWalkMode(owner);
    _updateSpeed(owner);
    _moveToTarget(owner);
}

template<>
void FollowMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.addUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    //_updateWalkMode(owner);
    _updateSpeed(owner);

    _moveToTarget(owner);
}

template<class T>
void FollowMovementGenerator<T>::Finalize(T &owner)
{
    owner.clearUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateWalkMode(owner);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::Interrupt(T &owner)
{
    owner.clearUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateWalkMode(owner);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

//-----------------------------------------------//
template void TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::_moveToTarget(Player &);
template void TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::_moveToTarget(Player &);
template void TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::_moveToTarget(Creature &);
template void TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::_moveToTarget(Creature &);
template bool TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::Update(Player &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::Update(Player &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::Update(Creature &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::Update(Creature &, const uint32 &);

template void ChaseMovementGenerator<Player>::_reachTarget(Player &);
template void ChaseMovementGenerator<Creature>::_reachTarget(Creature &);
template void ChaseMovementGenerator<Player>::Finalize(Player &);
template void ChaseMovementGenerator<Creature>::Finalize(Creature &);
template void ChaseMovementGenerator<Player>::Interrupt(Player &);
template void ChaseMovementGenerator<Creature>::Interrupt(Creature &);
template void ChaseMovementGenerator<Player>::Reset(Player &);
template void ChaseMovementGenerator<Creature>::Reset(Creature &);

template void FollowMovementGenerator<Player>::Finalize(Player &);
template void FollowMovementGenerator<Creature>::Finalize(Creature &);
template void FollowMovementGenerator<Player>::Interrupt(Player &);
template void FollowMovementGenerator<Creature>::Interrupt(Creature &);
template void FollowMovementGenerator<Player>::Reset(Player &);
template void FollowMovementGenerator<Creature>::Reset(Creature &);
