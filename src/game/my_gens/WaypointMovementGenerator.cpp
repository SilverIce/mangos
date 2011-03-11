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

/*
creature_movement Table

alter table creature_movement add `textid1` int(11) NOT NULL default '0';
alter table creature_movement add `textid2` int(11) NOT NULL default '0';
alter table creature_movement add `textid3` int(11) NOT NULL default '0';
alter table creature_movement add `textid4` int(11) NOT NULL default '0';
alter table creature_movement add `textid5` int(11) NOT NULL default '0';
alter table creature_movement add `emote` int(10) unsigned default '0';
alter table creature_movement add `spell` int(5) unsigned default '0';
alter table creature_movement add `wpguid` int(11) default '0';

*/

#include <ctime>

#include "WaypointMovementGenerator.h"
#include "ObjectMgr.h"
#include "Creature.h"
#include "DestinationHolderImp.h"
#include "CreatureAI.h"
#include "WaypointManager.h"
#include "WorldPacket.h"
#include "ScriptMgr.h"

#include <cassert>

#include "Movement/UnitMovement.h"
#include "Movement/MoveSpline.h"

using G3D::Vector3;

template<typename Path, typename InPath>
void FillPath(const Path& p, InPath & v, uint32 start_idx = 0, uint32 last_idx = 0)
{
    //v.reserve(last_idx - start_idx + 1);

    for (uint32 i = start_idx; i <= last_idx; ++i)
    {
        const Path::value_type& node = p[i];
        v.push_back(Vector3(node.x, node.y, node.z));
    }
}

template<typename T, typename P, typename InPath>
void FillPath(const Path<T,P>& p, InPath & v, uint32 start_idx = 0, uint32 last_idx = 0)
{
    //v.reserve(last_idx - start_idx + 1);

    for (uint32 i = start_idx; i <= last_idx; ++i)
    {
        const P& node = p[i];
        v.push_back(Vector3(node.x, node.y, node.z));
    }
}

uint32 MAX_LINEAR_POINTS  = 3;
float MAX_LINEAR_DISTANCE  = 200.f;

//-----------------------------------------------//
bool WaypointMovementGenerator<Creature>::LoadPath(Creature &c)
{
    DETAIL_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "LoadPath: loading waypoint path for creature %u, %u", c.GetGUIDLow(), c.GetDBTableGUIDLow());

    i_path = sWaypointMgr.GetPath(c.GetDBTableGUIDLow());

    // We may LoadPath() for several occasions:

    // 1: When creature.MovementType=2
    //    1a) Path is selected by creature.guid == creature_movement.id
    //    1b) Path for 1a) does not exist and then use path from creature.GetEntry() == creature_movement_template.entry

    // 2: When creature_template.MovementType=2
    //    2a) Creature is summoned and has creature_template.MovementType=2
    //        Creators need to be sure that creature_movement_template is always valid for summons.
    //        Mob that can be summoned anywhere should not have creature_movement_template for example.

    // No movement found for guid
    if (!i_path)
    {
        i_path = sWaypointMgr.GetPathTemplate(c.GetEntry());

        // No movement found for entry
        if (!i_path)
        {
            sLog.outErrorDb("WaypointMovementGenerator::LoadPath: creature %s (Entry: %u GUID: %u) doesn't have waypoint path",
                c.GetName(), c.GetEntry(), c.GetDBTableGUIDLow());
        return false;
        }
    }

    // ignore some nonsense paths
	// TODO: it should be handled in waypoint manager, not here
    if (i_path->size() < 2)
    {
        return false;
    }

    // analyzes path: splits path into nodes, detects cyclic path case
    {
        // TODO: info about path type (linear\catmullrom) should be stored in db,
        // it shouldn't be determined here
        bool isLinear = false;//!c.movement->HasMode(Movement::MoveModeLevitation) && !c.movement->HasMode(Movement::MoveModeFly);

        uint32 last = i_path->size() - 1 ;

        node_indexes.push_back(Node(0, last));
        current_node = 0;

        float length = 0.f;
        for (uint32 i = 1, j = 0; i <= last; ++i, ++j)
        {            
            const WaypointNode& node = i_path->at(i);
            //if (isLinear)
            //{
            //    const WaypointNode& prev_node = i_path->at(i-1);
            //    length += Vector3(node.x-prev_node.x,node.y-prev_node.y,node.z-prev_node.z).length();
            //}

            if (node.delay || /*|| node.script_id || node.behavior || */(isLinear && j > 5))
            {
                j = 0;
                length = 0.f;

                if (!node_indexes.empty())
                    node_indexes.back().lastIdx = i;

                if (i != last)
                    node_indexes.push_back(Node(i, last));
            }
        }

        is_cyclic = (node_indexes.size() == 1);
    }
    return true;
}

void WaypointMovementGenerator<Creature>::continueMove( Unit &u, uint32 /*node_index*/ )
{
    if (!i_path || i_path->size() < 2)
        return;

    u.addUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);

    uint32 last = node_indexes[current_node].lastIdx;
    uint32 first = std::min(node_indexes[current_node].firstIdx /*+ node_index*/, last);

    using namespace Movement;
    PointsArray path;
    FillPath(*i_path, path, first, last);

    UnitMovement& state = *u.movement;

    MoveSplineInit init(state);
    if (state.HasMode(MoveModeLevitation) || state.HasMode(MoveModeFly))
    {
        init.SetVelocity(10.f).SetFly();
        state.ApplyWalkMode(false);
    }
    else
    {
        //state.ApplyWalkMode(true);
        state.ApplyWalkMode(false);
    }

    if (is_cyclic)
        init.SetCyclic();

    init.MovebyPath(path, first, true).Launch();
}

void WaypointMovementGenerator<Creature>::Initialize( Creature &u )
{
    //u.StopMoving();
    if (LoadPath(u))
    {
        continueMove(u);
    }
}

void WaypointMovementGenerator<Creature>::Finalize(Creature &creature)
{
    creature.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

void WaypointMovementGenerator<Creature>::Interrupt(Creature &creature)
{
    // currently Interrupt called twice - as a result, reset position became wrong at second call
    // Temporary solution:
    if (creature.hasUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE))
    {
        reset_position = (Pos&)creature.movement->GetPosition3();
        creature.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
    }
}

void WaypointMovementGenerator<Creature>::Reset(Creature &u)
{
    b_Stopped = false;
    continueMove(u);
}

bool WaypointMovementGenerator<Creature>::Update(Creature &creature, const uint32 &diff)
{
    if (!i_path || i_path->size() < 2)
        return false;

    // Waypoint movement can be switched on/off
    // This is quite handy for escort quests and other stuff
    //if (creature.hasUnitState(UNIT_STAT_NOT_MOVE))
    //{
    //    creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);
    //    return true;
    //}

    while (!OnArrived.empty())
    {
        int point = OnArrived.front();
        OnArrived.pop_front();
        processNodeScripts(creature, point);
    }

    if (IsPaused())
    {
        i_stopTimer.Update(diff);
        if (i_stopTimer.Passed())
        {
            b_Stopped = false;
            continueMove(creature);
        }
    }
    else if (creature.IsStopped())
    {
        PauseMovement(15000); // 15 seconds
    }

    return true;
}

void WaypointMovementGenerator<Creature>::OnSplineDone(Unit& u)
{
}

void WaypointMovementGenerator<Creature>::OnEvent(Unit& u, int eventId, int data)
{
    if (eventId == 1)
    {
        OnArrived.push_back(data);
        current_path_index = data;
    }
    else if (eventId == 0)
    {
        if ((++current_node) >= node_indexes.size())
            current_node = 0;
        OnArrived.push_back(-10);
    }
}

void WaypointMovementGenerator<Creature>::processNodeScripts(Creature& creature, int32 pointId)
{
    if (!i_path || i_path->size() < 2)
        return;
   
    if (pointId == -10)// current dirty way of OnArrived even processing
    {
        if (!IsPaused())
            continueMove(creature);
        return;
    }
    return;

    const WaypointNode& node = i_path->at(pointId);

    if (node.delay)
    {
        PauseMovement(node.delay);
    }

    if (node.script_id)
    {
        //DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature movement start script %u at point %u for creature %u (entry %u).", node.script_id, node_indexes[current_node].lastIdx, creature.GetDBTableGUIDLow(), creature.GetEntry());
        creature.GetMap()->ScriptsStart(sCreatureMovementScripts, node.script_id, &creature, &creature);
    }

    if (WaypointBehavior *behavior = node.behavior)
    {
        if (behavior->emote != 0)
            creature.HandleEmote(behavior->emote);

        if (behavior->spell != 0)
            creature.CastSpell(&creature, behavior->spell, false);

        if (behavior->model1 != 0)
            creature.SetDisplayId(behavior->model1);

        if (behavior->textid[0])
        {
            // Not only one text is set
            if (behavior->textid[1])
            {
                // Select one from max 5 texts (0 and 1 already checked)
                int i = 2;
                for(; i < MAX_WAYPOINT_TEXT; ++i)
                {
                    if (!behavior->textid[i])
                        break;
                }

                creature.MonsterSay(behavior->textid[rand() % i], 0, 0);
            }
            else
                creature.MonsterSay(behavior->textid[0], 0, 0);
        }
    }

    //m_isArrivalDone = true;

    // Inform script
    MovementInform(creature, pointId);
}

void WaypointMovementGenerator<Creature>::MovementInform(Creature &creature, uint32 pointId)
{
    if (creature.AI())
        creature.AI()->MovementInform(WAYPOINT_MOTION_TYPE, pointId);
}

bool WaypointMovementGenerator<Creature>::GetResetPosition(Creature&, float& x, float& y, float& z)
{
    x = reset_position.x;
    y = reset_position.y;
    z = reset_position.z;
    return true;
}

//----------------------------------------------------//
uint32 FlightPathMovementGenerator::GetPathAtMapEnd() const
{
    if (current_node >= i_path->size())
        return i_path->size();

    uint32 curMapId = (*i_path)[current_node].mapid;

    for(uint32 i = current_node; i < i_path->size(); ++i)
    {
        if ((*i_path)[i].mapid != curMapId)
            return i;
    }

    return i_path->size();
}

void FlightPathMovementGenerator::Initialize(Player &player)
{
    Reset(player);
}

void FlightPathMovementGenerator::Finalize(Player & player)
{
    // remove flag to prevent send object build movement packets for flight state and crash (movement generator already not at top of stack)
    player.clearUnitState(UNIT_STAT_TAXI_FLIGHT);

    //float x, y, z;
    //i_destinationHolder.GetLocationNow(player.GetBaseMap(), x, y, z);
    //player.SetPosition(x, y, z, player.GetOrientation());

    player.Unmount();
    player.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);

    if(player.m_taxi.empty())
    {
        player.getHostileRefManager().setOnlineOfflineState(true);
        if(player.pvpInfo.inHostileArea)
            player.CastSpell(&player, 2479, true);

        // update z position to ground and orientation for landing point
        // this prevent cheating with landing  point at lags
        // when client side flight end early in comparison server side
        player.StopMoving();
    }
}

void FlightPathMovementGenerator::Interrupt(Player & player)
{
    player.clearUnitState(UNIT_STAT_TAXI_FLIGHT);
}

void FlightPathMovementGenerator::Reset(Player & player)
{
    player.getHostileRefManager().setOnlineOfflineState(false);
    player.addUnitState(UNIT_STAT_TAXI_FLIGHT);
    player.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);
    //Traveller<Player> traveller(player);
    // do not send movement, it was sent already
    //i_destinationHolder.SetDestination(traveller, (*i_path)[i_currentNode].x, (*i_path)[i_currentNode].y, (*i_path)[i_currentNode].z, false);
    //player.SendMonsterMoveByPath(GetPath(),GetCurrentNode(),GetPathAtMapEnd(), SplineFlags(SPLINEFLAG_WALKMODE|SPLINEFLAG_FLYING));

    using namespace Movement;
    PointsArray path;
    FillPath(GetPath(), path, GetCurrentNode(), GetPathAtMapEnd()-1);

    MoveSplineInit init(*player.movement);
    init.MovebyPath(path,GetCurrentNode());
    init.SetVelocity(PLAYER_FLIGHT_SPEED);
    init.SetFly();
    init.Launch();
}

bool FlightPathMovementGenerator::Update(Player &player, const uint32 &diff)
{
    while (!OnArrived.empty())
    {
        int point = OnArrived.front();
        OnArrived.pop_front();
        DoEventIfAny(player, point, false);
        DoEventIfAny(player, point, true);
    }

    // we have arrived to the end of the path
    return !player.movement->move_spline.Finalized();
}

void FlightPathMovementGenerator::SetCurrentNodeAfterTeleport()
{
    if (i_path->empty())
        return;

    uint32 map0 = (*i_path)[0].mapid;

    for (size_t i = 1; i < i_path->size(); ++i)
    {
        if ((*i_path)[i].mapid != map0)
        {
            current_node = i;
            return;
        }
    }
}

void FlightPathMovementGenerator::DoEventIfAny(Unit& player, uint32 nodeId, bool departure)
{
    const TaxiPathNodeEntry& node = (*i_path)[nodeId];

    if (uint32 eventid = departure ? node.departureEventID : node.arrivalEventID)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Taxi %s event %u of node %u of path %u for player %s", departure ? "departure" : "arrival", eventid, node.index, node.path, player.GetName());

        //if (!Script->ProcessEventId(eventid, &player, &player, departure))
            //player.GetMap()->ScriptsStart(sEventScripts, eventid, &player, &player);
    }
}

void FlightPathMovementGenerator::OnEvent(Unit& u, int eventId, int data)
{
    if (eventId == 1)
    {
        MANGOS_ASSERT(data >= 0 && data < i_path->size());
        OnArrived.push_back(data);
        current_node = data;
    }
}
