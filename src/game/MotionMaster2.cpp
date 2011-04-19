#include "MotionMaster2.h"
#include "ConfusedMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "Timer.h"


class DefaultStatePriorities
{
    int AIStatePriorities[AIActionType_Count];

public:
    DefaultStatePriorities()
    {
        AIStatePriorities[Idle] = 0;
        AIStatePriorities[OutOfCombat] = 10;
        AIStatePriorities[Combat] = 30;
        AIStatePriorities[Effect] = 40;
        AIStatePriorities[Feared] = 50;
        AIStatePriorities[Confused] = 60;
        AIStatePriorities[Root] = 70;
        AIStatePriorities[Stun] = 80;
    }

    int operator[](int i) const { return AIStatePriorities[i];}

} g_DefaultStatePriorities;

MotionMasterImpl::MotionMasterImpl(Unit& Owner) :
    owner(Owner), m_locked(false), m_slot(Idle)
{
    InitDefaults();
}

std::string MotionMasterImpl::ToString()
{
    const char * AIStateType_Name[AIActionType_Count] =
    {
        "Idle",
        "OutOfCombat",
        "Combat",
        "Effect",
        "Feared",
        "Confused",
        "Root",
        "Stun",
    };

    std::stringstream str;
    if (get_CurrentState())
        str << "current slot " << AIStateType_Name[get_CurrentSlotType()] << ": " << get_CurrentState().TypeName() << std::endl;
    for (AIStateType slot = Idle; slot < AIActionType_Count; ++((int&)slot))
    {
        str << "slot " << AIStateType_Name[slot] << ": ";
        if (get_AIState(slot))
            str << get_AIState(slot).TypeName();
        else
            str << "empty";
        str << std::endl;
    }

    return str.str();
}

class IdleState_ : public MovementGenerator
{
public:

    void Interrupt(Unit &) {}
    void Reset(Unit &) {}
    void Initialize(Unit &) {}
    void Finalize(Unit &) {}
    bool Update(Unit &, const uint32&) { return true; }

    virtual MovementGeneratorType GetMovementGeneratorType() const
    {
        return IDLE_MOTION_TYPE;
    }
};

// derived from IdleState_ to not write new GetMovementGeneratorType, Update
class StunnedState : public IdleState_
{
public:

    void Interrupt(Unit &u) {Finalize(u);}
    void Reset(Unit &u) {Initialize(u);}
    void Initialize(Unit &u)
    {
        Unit * const target = &u;
        target->addUnitState(UNIT_STAT_STUNNED);
        target->SetTargetGuid(ObjectGuid());

        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        // Creature specific
        if (target->GetTypeId() != TYPEID_PLAYER)
            target->StopMoving();
        else
        {
            ((Player*)target)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
            target->SetStandState(UNIT_STAND_STATE_STAND);// in 1.5 client
        }

        WorldPacket data(SMSG_FORCE_MOVE_ROOT, 8);
        data << target->GetPackGUID();
        data << uint32(0);
        target->SendMessageToSet(&data, true);
    }

    void Finalize(Unit &u)
    {
        Unit * const target = &u;

        target->clearUnitState(UNIT_STAT_STUNNED);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 8+4);
        data << target->GetPackGUID();
        data << uint32(0);
        target->SendMessageToSet(&data, true);
    }

};

class RootState : public IdleState_
{
public:

    void Interrupt(Unit &u) {Finalize(u);}
    void Reset(Unit &u) {Initialize(u);}
    void Initialize(Unit &u)
    {
        Unit * const target = &u;
        target->addUnitState(UNIT_STAT_ROOT);
        target->SetTargetGuid(ObjectGuid());

        //Save last orientation
        if( target->getVictim() )
            target->SetOrientation(target->GetAngle(target->getVictim()));

        if(target->GetTypeId() == TYPEID_PLAYER)
        {
            WorldPacket data(SMSG_FORCE_MOVE_ROOT, 10);
            data << target->GetPackGUID();
            data << (uint32)2;
            target->SendMessageToSet(&data, true);

            //Clear unit movement flags
            ((Player*)target)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
        }
        else
            target->StopMoving();
    }

    void Finalize(Unit &u)
    {
        Unit * const target = &u;
        target->clearUnitState(UNIT_STAT_ROOT);
        if(target->GetTypeId() == TYPEID_PLAYER)
        {
            WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 10);
            data << target->GetPackGUID();
            data << (uint32)2;
            target->SendMessageToSet(&data, true);
        }
    }
};

bool isPlayer(Unit& owner)
{
    return owner.isType(TYPEMASK_PLAYER);
}

AIState * MotionMasterImpl::InitStandartState(AIStateType slot, Unit& owner)
{
    AIState * state = NULL;
    switch(slot)
    {
    case Confused:
        if (isPlayer(owner))
            state = new ConfusedMovementGenerator<Player>();
        else
            state = new ConfusedMovementGenerator<Creature>();
        break;
    case Stun:
        state = new StunnedState();
    case Root:
        state = new RootState();
    case Combat:
        {
            //if (Unit * victim = owner.getVictim())
               // state = isPlayer(owner) ? new ChaseMovementGenerator<Player>(*victim) : new ChaseMovementGenerator<Creature>(*victim);
            break;
        }
    default:
        break;
    }

    if (!state)
        state = new IdleState_();

    return state;
}

void MotionMasterImpl::reset_Priority(AIStateType slot)
{
    m_priorities[slot] = g_DefaultStatePriorities[slot];
}