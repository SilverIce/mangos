#include "Movement/MaNGOS_API.h"
#include "Player.h"
#include "WorldSession.h"
#include "CellImpl.h"
#include "Movement/Location.h"
#include "Movement/ClientMovement.h"

namespace MaNGOS_API
{
    void UpdateMapPosition(WorldObject* obj, const Movement::Location& loc)
    {
        if (obj->IsInWorld())
        {
            if (obj->GetTypeId() == TYPEID_PLAYER)
                ((Player*)obj)->SetPosition(loc.x,loc.y,loc.z,loc.orientation);
            else if (obj->GetTypeId() == TYPEID_UNIT)
                obj->GetMap()->CreatureRelocation((Creature*)obj,loc.x,loc.y,loc.z,loc.orientation);
            else
                MANGOS_ASSERT(false && "Unexpected object typeId");
        }
        else
            obj->Relocate(loc.x,loc.y,loc.z,loc.orientation);
    }

    using Movement::MovementMessage;
    struct MoveMsgDeliverer
    {
        MovementMessage& m_msg;
        uint32 m_phase_mask;

        MoveMsgDeliverer(WorldObject const* obj, MovementMessage& msg) :
            m_msg(msg), m_phase_mask(obj->GetPhaseMask()) {}

        template<class T> void Visit(GridRefManager<T>&) {}

        void Visit(CameraMapType& m)
        {
            for (CameraMapType::iterator it = m.begin(); it!= m.end(); ++it)
            {
                Player * owner = it->getSource()->GetOwner();
                if (owner->InSamePhase(m_phase_mask))
                    owner->GetSession()->MoveClient()->SendMoveMessage(m_msg);
            }
        }
    };

    void BroadcastMessage(WorldObject const* obj, Movement::MovementMessage& msg)
    {
        if (obj->IsInWorld())
        {
            MoveMsgDeliverer del(obj, msg);
            Cell::VisitWorldObjects(obj, del, obj->GetMap()->GetVisibilityDistance());
        }
    }

    void BroadcastMessage( WorldObject const* obj, WorldPacket& msg )
    {
        const_cast<WorldObject*>(obj)->SendMessageToSet(&msg, true);
    }

    void SendPacket( HANDLE socket, const WorldPacket& data )
    {
        ((WorldSession*)socket)->SendPacket(&data);
    }
}
