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

#ifndef MANGOS_WAYPOINTMOVEMENTGENERATOR_H
#define MANGOS_WAYPOINTMOVEMENTGENERATOR_H

/** @page PathMovementGenerator is used to generate movements
 * of waypoints and flight paths.  Each serves the purpose
 * of generate activities so that it generates updated
 * packets for the players.
 */

#include "MovementGenerator.h"
#include "DestinationHolder.h"
#include "WaypointManager.h"
#include "Path.h"
#include "Traveller.h"

#include "Player.h"

#include <vector>
#include <set>

#define FLIGHT_TRAVEL_UPDATE  100
#define STOP_TIME_FOR_PLAYER  3 * MINUTE * IN_MILLISECONDS  // 3 Minutes

template<class T, class P>
class MANGOS_DLL_SPEC PathMovementBase
{
    public:
        PathMovementBase() : current_node(0) {}
        virtual ~PathMovementBase() {};

        bool MovementInProgress(void) const { return current_node < i_path->size()-1; }

        // template pattern, not defined .. override required
        void LoadPath(T &);
        uint32 GetCurrentNode() const { return current_node; }

    protected:
        uint32 current_node;
        P i_path;
};

/** WaypointMovementGenerator loads a series of way points
 * from the DB and apply it to the creature's movement generator.
 * Hence, the creature will move according to its predefined way points.
 */

template<class T>
class MANGOS_DLL_SPEC WaypointMovementGenerator;

template<>
class MANGOS_DLL_SPEC WaypointMovementGenerator<Creature>
: public MovementGeneratorMedium< Creature, WaypointMovementGenerator<Creature> >,
public PathMovementBase<Creature, WaypointPath const*>
{
    public:
        WaypointMovementGenerator(Creature &) : current_node_index(0), b_Stopped(false), i_stopTimer(0) {}

        ~WaypointMovementGenerator() { i_path = NULL; }
        void Initialize(Creature &u);
        void Interrupt(Creature &);
        void Finalize(Creature &);
        void Reset(Creature &u);
        bool Update(Creature &u, const uint32 &diff);

        void MovementInform(Creature &);

        MovementGeneratorType GetMovementGeneratorType() const { return WAYPOINT_MOTION_TYPE; }

        // now path movement implmementation
        bool LoadPath(Creature &c);

        // Player stoping creature
        bool IsPaused() { return b_Stopped; }

        void PauseMovement(int32 delay)
        {
            b_Stopped = true;
            i_stopTimer.Reset(delay);
        }

        bool GetResetPosition(Creature&, float& x, float& y, float& z);
        void OnSplineDone(Unit&);
        void OnEvent(Unit&, int eventId, int data);

    private:

        struct Node
        {
            Node(uint32 first, uint32 last) : firstIdx(first), lastIdx(last) {}
            uint32 firstIdx;
            uint32 lastIdx;
            uint32 size() const { return lastIdx-firstIdx+1;}
        };

        void movebyNode(Unit &u, uint32 node_index = 0);
        void processNodeScripts(Creature& u);

        std::vector<Node> node_indexes;
        struct Pos
        {
            float x, y, z;
        } reset_position;
        uint32 current_node_index;
        bool is_cyclic;
        bool b_Stopped;

        ShortTimeTracker i_stopTimer;
};

/** FlightPathMovementGenerator generates movement of the player for the paths
 * and hence generates ground and activities for the player.
 */
class MANGOS_DLL_SPEC FlightPathMovementGenerator
: public MovementGeneratorMedium< Player, FlightPathMovementGenerator >,
public PathMovementBase<Player,TaxiPathNodeList const*>
{
    public:
        explicit FlightPathMovementGenerator(TaxiPathNodeList const& pathnodes, uint32 startNode = 0)
        {
            i_path = &pathnodes;
            current_node = startNode;
        }
        void Initialize(Player &);
        void Finalize(Player &);
        void Interrupt(Player &);
        void Reset(Player &);
        bool Update(Player &, const uint32 &);
        MovementGeneratorType GetMovementGeneratorType() const { return FLIGHT_MOTION_TYPE; }

        TaxiPathNodeList const& GetPath() { return *i_path; }
        uint32 GetPathAtMapEnd() const;
        bool HasArrived() const { return !MovementInProgress(); }
        void SetCurrentNodeAfterTeleport();
        void SkipCurrentNode() { ++current_node; }
        void DoEventIfAny(Unit& player, uint32 node, bool departure);
        void OnEvent(Unit&, int eventId, int data);
    private:
        std::list<int32> OnArrived;
};
#endif
