/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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

#include "MotionMaster.h"
#include "CreatureAISelector.h"
#include "Creature.h"
#include "ConfusedMovementGenerator.h"
#include "FleeingMovementGenerator.h"
#include "HomeMovementGenerator.h"
#include "IdleMovementGenerator.h"
#include "PointMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "RandomMovementGenerator.h"
#include "movement/MoveSpline.h"
#include "movement/MoveSplineInit.h"
#include "CreatureLinkingMgr.h"

#include <cassert>

inline bool isStatic(MovementGenerator *mv)
{
    return (mv == &si_idleMovement);
}

void MotionMaster::Initialize()
{
    // stop current move
    if (!m_owner->IsStopped())
        m_owner->StopMoving();

    impl->DropAllStates();
    // clear ALL movement generators (including default)
    //Clear(false,true);

    // set new default movement generator
    if (m_owner->GetTypeId() == TYPEID_UNIT /*&& !m_owner->hasUnitState(UNIT_STAT_CONTROLLED)*/)
    {
        MovementGenerator* movement = FactorySelector::selectMovementGenerator((Creature*)m_owner);
        if (movement)
            Mutate(movement, UnitAction_DoWaypoints);
    }
}

MotionMaster::~MotionMaster()
{
}

void MotionMaster::UpdateMotion(uint32 diff)
{
    impl->Update(diff);

/*
    if (m_owner->hasUnitState(UNIT_STAT_CAN_NOT_MOVE))
        return;

    MANGOS_ASSERT( !empty() );
    m_cleanFlag |= MMCF_UPDATE;

    if (!top()->Update(*m_owner, diff))
    {
        m_cleanFlag &= ~MMCF_UPDATE;
        MovementExpired();
    }
    else
        m_cleanFlag &= ~MMCF_UPDATE;

    if (m_expList)
    {
        for (size_t i = 0; i < m_expList->size(); ++i)
        {
            MovementGenerator* mg = (*m_expList)[i];
            if (!isStatic(mg))
                delete mg;
        }

        delete m_expList;
        m_expList = NULL;

        if (empty())
            Initialize();

        if (m_cleanFlag & MMCF_RESET)
        {
            top()->Reset(*m_owner);
            m_cleanFlag &= ~MMCF_RESET;
        }
    }*/
}

void MotionMaster::DirectClean(bool reset, bool all)
{
/*
    while( all ? !empty() : size() > 1 )
    {
        MovementGenerator *curr = top();
        pop();
        curr->Finalize(*m_owner);

        if (!isStatic(curr))
            delete curr;
    }

    if (!all && reset)
    {
        MANGOS_ASSERT( !empty() );
        top()->Reset(*m_owner);
    }
*/
}

void MotionMaster::DelayedClean(bool reset, bool all)
{
/*
    if (reset)
        m_cleanFlag |= MMCF_RESET;
    else
        m_cleanFlag &= ~MMCF_RESET;

    if (empty() || (!all && size() == 1))
        return;

    if (!m_expList)
        m_expList = new ExpireList();

    while( all ? !empty() : size() > 1 )
    {
        MovementGenerator *curr = top();
        pop();
        curr->Finalize(*m_owner);

        if (!isStatic(curr))
            m_expList->push_back(curr);
    }*/
}

void MotionMaster::DirectExpire(bool reset)
{
/*
    if (empty() || size() == 1)
        return;

    MovementGenerator *curr = top();
    pop();

    // also drop stored under top() targeted motions
    while (!empty() && (top()->GetMovementGeneratorType() == CHASE_MOTION_TYPE || top()->GetMovementGeneratorType() == FOLLOW_MOTION_TYPE))
    {
        MovementGenerator *temp = top();
        pop();
        temp->Finalize(*m_owner);
        delete temp;
    }

    // Store current top MMGen, as Finalize might push a new MMGen
    MovementGenerator* nowTop = empty() ? NULL : top();
    // it can add another motions instead
    curr->Finalize(*m_owner);

    if (!isStatic(curr))
        delete curr;

    if (empty())
        Initialize();

    // Prevent reseting possible new pushed MMGen
    if (reset && top() == nowTop)
        top()->Reset(*m_owner);
*/
}

void MotionMaster::DelayedExpire(bool reset)
{
/*
    if (reset)
        m_cleanFlag |= MMCF_RESET;
    else
        m_cleanFlag &= ~MMCF_RESET;

    if (empty() || size() == 1)
        return;

    MovementGenerator *curr = top();
    pop();

    if (!m_expList)
        m_expList = new ExpireList();

    // also drop stored under top() targeted motions
    while (!empty() && (top()->GetMovementGeneratorType() == CHASE_MOTION_TYPE || top()->GetMovementGeneratorType() == FOLLOW_MOTION_TYPE))
    {
        MovementGenerator *temp = top();
        pop();
        temp ->Finalize(*m_owner);
        m_expList->push_back(temp );
    }

    curr->Finalize(*m_owner);

    if (!isStatic(curr))
        m_expList->push_back(curr);*/
}

void MotionMaster::MoveIdle()
{
    impl->EnterAction(UnitAction_Idle);
}

void MotionMaster::MoveRandom()
{
    if (m_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog.outError("%s attempt to move random.", m_owner->GetGuidStr().c_str());
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s move random.", m_owner->GetGuidStr().c_str());
        //Mutate(new RandomMovementGenerator<Creature>(*m_owner), OutOfCombat);
    }
}

void MotionMaster::MoveTargetedHome()
{
   // if (m_owner->hasUnitState(UNIT_STAT_LOST_CONTROL))
       // return;

    // home movement is command that returns AI to his default state
    impl->DropAllStates();

    if (m_owner->GetTypeId() == TYPEID_UNIT && !((Creature*)m_owner)->GetCharmerOrOwnerGuid())
    {
        // Manual exception for linked mobs
        if (m_owner->IsLinkingEventTrigger() && m_owner->GetMap()->GetCreatureLinkingHolder()->TryFollowMaster((Creature*)m_owner))
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s refollowed linked master", m_owner->GetGuidStr().c_str());
        else
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s targeted home", m_owner->GetGuidStr().c_str());
            Mutate(new HomeMovementGenerator<Creature>(), UnitAction_Effect);
        }
    }
    else if (m_owner->GetTypeId() == TYPEID_UNIT && ((Creature*)m_owner)->GetCharmerOrOwnerGuid())
    {
        if (Unit *target = ((Creature*)m_owner)->GetCharmerOrOwner())
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s follow to %s", m_owner->GetGuidStr().c_str(), target->GetGuidStr().c_str());
            Mutate(new FollowMovementGenerator<Creature>(*target,PET_FOLLOW_DIST,PET_FOLLOW_ANGLE), UnitAction_DoWaypoints);
        }
        else
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s attempt but fail to follow owner", m_owner->GetGuidStr().c_str());
        }
    }
    else
        sLog.outError("%s attempt targeted home", m_owner->GetGuidStr().c_str());
}

void MotionMaster::MoveConfused()
{
    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s move confused", m_owner->GetGuidStr().c_str());

    if (m_owner->GetTypeId() == TYPEID_PLAYER)
        Mutate(new ConfusedMovementGenerator<Player>(), UnitAction_Confused);
    else
        Mutate(new ConfusedMovementGenerator<Creature>(), UnitAction_Confused);
}

void MotionMaster::MoveChase(Unit* target, float dist, float angle)
{
    // ignore movement request if target not exist
    if (!target)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s chase to %s", m_owner->GetGuidStr().c_str(), target->GetGuidStr().c_str());

    if (m_owner->GetTypeId() == TYPEID_PLAYER)
        Mutate(new ChaseMovementGenerator<Player>(*target,dist,angle), UnitAction_Chase);
    else
        Mutate(new ChaseMovementGenerator<Creature>(*target,dist,angle), UnitAction_Chase);
}

void MotionMaster::MoveFollow(Unit* target, float dist, float angle)
{
    // ignore movement request if target not exist
    if (!target)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s follow to %s", m_owner->GetGuidStr().c_str(), target->GetGuidStr().c_str());

    if (m_owner->GetTypeId() == TYPEID_PLAYER)
        Mutate(new FollowMovementGenerator<Player>(*target,dist,angle), UnitAction_Chase);
    else
        Mutate(new FollowMovementGenerator<Creature>(*target,dist,angle), UnitAction_Chase);
}

void MotionMaster::MovePoint(uint32 id, float x, float y, float z)
{
    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s targeted point (Id: %u X: %f Y: %f Z: %f)", m_owner->GetGuidStr().c_str(), id, x, y, z );

    if (m_owner->GetTypeId() == TYPEID_PLAYER)
        Mutate(new PointMovementGenerator<Player>(id,x,y,z), UnitAction_Effect);
    else
        Mutate(new PointMovementGenerator<Creature>(id,x,y,z), UnitAction_Effect);
}

void MotionMaster::MoveSeekAssistance(float x, float y, float z)
{
    if (m_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog.outError("%s attempt to seek assistance", m_owner->GetGuidStr().c_str());
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s seek assistance (X: %f Y: %f Z: %f)",
            m_owner->GetGuidStr().c_str(), x, y, z );
        Mutate(new AssistanceMovementGenerator(x,y,z), UnitAction_Effect);
    }
}

void MotionMaster::MoveSeekAssistanceDistract(uint32 time)
{
    if (m_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog.outError("%s attempt to call distract after assistance", m_owner->GetGuidStr().c_str());
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s is distracted after assistance call (Time: %u)",
            m_owner->GetGuidStr().c_str(), time );
        Mutate(new AssistanceDistractMovementGenerator(time), UnitAction_Effect);
    }
}

void MotionMaster::MoveFleeing(Unit* enemy, uint32 time)
{
    if (!enemy)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s flee from %s", m_owner->GetGuidStr().c_str(), enemy->GetGuidStr().c_str());

    if (m_owner->GetTypeId() == TYPEID_PLAYER)
        Mutate(new FleeingMovementGenerator<Player>(enemy->GetObjectGuid()), UnitAction_Feared);
    else
    {
        if (time)
            Mutate(new TimedFleeingMovementGenerator(enemy->GetObjectGuid(), time), UnitAction_Feared);
        else
            Mutate(new FleeingMovementGenerator<Creature>(enemy->GetObjectGuid()), UnitAction_Feared);
    }
}

void MotionMaster::MoveWaypoint()
{
    if (m_owner->GetTypeId() == TYPEID_UNIT)
    {
        if (GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
        {
            sLog.outError("Creature %s (Entry %u) attempt to MoveWaypoint() but creature is already using waypoint", m_owner->GetGuidStr().c_str(), m_owner->GetEntry());
            return;
        }

        Creature* creature = (Creature*)m_owner;

        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature %s (Entry %u) start MoveWaypoint()", m_owner->GetGuidStr().c_str(), m_owner->GetEntry());
        Mutate(new WaypointMovementGenerator<Creature>(*creature), UnitAction_DoWaypoints);
    }
    else
    {
        sLog.outError("Non-creature %s attempt to MoveWaypoint()", m_owner->GetGuidStr().c_str());
    }
}

void MotionMaster::MoveTaxiFlight(uint32 path, uint32 pathnode)
{
    if (m_owner->GetTypeId() == TYPEID_PLAYER)
    {
        if (path < sTaxiPathNodesByPath.size())
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s taxi to (Path %u node %u)", m_owner->GetGuidStr().c_str(), path, pathnode);
            FlightPathMovementGenerator* mgen = new FlightPathMovementGenerator(sTaxiPathNodesByPath[path],pathnode);
            Mutate(mgen, UnitAction_Effect);
        }
        else
        {
            sLog.outError("%s attempt taxi to (nonexistent Path %u node %u)",
                m_owner->GetGuidStr().c_str(), path, pathnode);
        }
    }
    else
    {
        sLog.outError("%s attempt taxi to (Path %u node %u)",
            m_owner->GetGuidStr().c_str(), path, pathnode);
    }
}

void MotionMaster::MoveDistract(uint32 timer)
{
    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s distracted (timer: %u)", m_owner->GetGuidStr().c_str(), timer);
    DistractMovementGenerator* mgen = new DistractMovementGenerator(timer);
    Mutate(mgen, UnitAction_Effect);
}

void MotionMaster::Mutate(MovementGenerator *m, int stateId)
{
    impl->PushAction(stateId, m);
}

void MotionMaster::propagateSpeedChange()
{
/*
    Impl::container_type::iterator it = Impl::c.begin();
    for ( ;it != end(); ++it)
    {
        (*it)->unitSpeedChanged();
    }
*/
}

MovementGeneratorType MotionMaster::GetCurrentMovementGeneratorType() const
{
    return const_cast<MotionMaster*>(this)->top()->GetMovementGeneratorType();
}

bool MotionMaster::GetDestination(float &x, float &y, float &z)
{
    if (m_owner->movespline->Finalized())
        return false;

    const G3D::Vector3& dest = m_owner->movespline->FinalDestination();
    x = dest.x;
    y = dest.y;
    z = dest.z;
    return true;
}

void MotionMaster::UpdateFinalDistanceToTarget(float fDistance)
{
}

MotionMaster::MotionMaster(Unit *unit) : m_owner(unit), m_expList(NULL), m_cleanFlag(MMCF_NONE)
{
    impl = &unit->stateMgr();
}

/**	Does nothing */
void MotionMaster::Clear(bool reset /*= true*/, bool all /*= false*/)
{
    impl->InitDefaults();
}

MovementGenerator * MotionMaster::top()
{
    MovementGenerator * mm = dynamic_cast<MovementGenerator*>(impl->CurrentAction());
    MANGOS_ASSERT(mm);
    return mm;
}

void MotionMaster::MoveJump(float x, float y, float z, float horizontalSpeed, float max_height, uint32 id)
{
    Movement::MoveSplineInit init(*m_owner);
    init.MoveTo(x,y,z);
    init.SetParabolic(max_height,0);
    init.SetVelocity(horizontalSpeed);
    init.Launch();
    Mutate(new EffectMovementGenerator(id), UnitAction_Effect);
}

void MotionMaster::MoveFall()
{
    // use larger distance for vmap height search than in most other cases
    float tz = m_owner->GetTerrain()->GetHeight(m_owner->GetPositionX(), m_owner->GetPositionY(), m_owner->GetPositionZ(), true, MAX_FALL_DISTANCE);
    if (tz <= INVALID_HEIGHT)
    {
        DEBUG_LOG("MotionMaster::MoveFall: unable retrive a proper height at map %u (x: %f, y: %f, z: %f).",
            m_owner->GetMap()->GetId(), m_owner->GetPositionX(), m_owner->GetPositionX(), m_owner->GetPositionZ());
        return;
    }

    // Abort too if the ground is very near
    if (fabs(m_owner->GetPositionZ() - tz) < 0.1f)
        return;

    Movement::MoveSplineInit init(*m_owner);
    init.MoveTo(m_owner->GetPositionX(),m_owner->GetPositionY(),tz);
    init.SetFall();
    init.Launch();
    Mutate(new EffectMovementGenerator(0), UnitAction_Effect);
}
