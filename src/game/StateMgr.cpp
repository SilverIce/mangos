
#include "ConfusedMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "Timer.h"
#include "StateMgr_Impl.h"
#include "StateMgr.h"

#include "Player.h"
#include "Creature.h"


std::string ActionPriorityQueue::ToString()
{

    std::stringstream str;
    str << "";
   /* if (get_CurrentState())
        str << "current slot " << AIStateType_Name[get_CurrentSlot()] << ": " << get_CurrentState().TypeName() << std::endl;
    for (UnitStateId slot = Idle; slot < UnitState_End; ++((int&)slot))
    {
        str << "slot " << AIStateType_Name[slot] << ": ";
        if (get_AIState(slot))
            str << get_AIState(slot).TypeName();
        else
            str << "empty";
        str << std::endl;
    }*/

    return str.str();
}

// derived from IdleState_ to not write new GetMovementGeneratorType, Update
class StunnedState : public NullUnitState
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

class RootState : public NullUnitState
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
        if(target->getVictim())
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

UnitAction * constructFuckingMMgen(Unit& owner)
{
    if (Unit * victim = owner.getVictim())
    {
        if (isPlayer(owner))
            return new ChaseMovementGenerator<Player>((Unit&)*victim);
        else
            return new ChaseMovementGenerator<Creature>((Unit&)*victim);
    }
    return NULL;
}

UnitAction * ActionPriorityQueue::CreateStandartState(int stateId, Unit& owner)
{
    UnitAction * state = NULL;
    switch(stateId)
    {
    case UnitAction_Confused:
        if (isPlayer(owner))
            state = new ConfusedMovementGenerator<Player>();
        else
            state = new ConfusedMovementGenerator<Creature>();
        break;
    case UnitAction_Stun:
        state = new StunnedState();
        break;
    case UnitAction_Root:
        state = new RootState();
        break;
    case UnitAction_Chase:
        /*if (Unit * victim = owner.getVictim())
        {
            if (isPlayer(owner))
                state = new ChaseMovementGenerator<Player>((Unit&)*victim);
            else
                state = new ChaseMovementGenerator<Creature>((Unit&)*victim);
        }*/
        break;
    default:
        break;
    }

    if (!state)
        state = new NullUnitState();

    return state;
}

ActionPriorityQueue::ActionWrap ActionPriorityQueue::NullState;

UnitStateMgr::UnitStateMgr(Unit* owner) : m(*new ActionPriorityQueue(*owner))
{
}

UnitStateMgr::~UnitStateMgr()
{
    delete &m;
}

void UnitStateMgr::InitDefaults()
{
    m.InitDefaults();
}

void UnitStateMgr::Update(uint32 diff)
{
    m.Update(diff);
}

void UnitStateMgr::DropAction(int slot)
{
    m.DropState(slot);
}

void UnitStateMgr::PushAction(UnitActionId slot)
{
    m.AIStatePut(slot);
}

void UnitStateMgr::PushAction(UnitActionId slot, int priority)
{
    m.AIStatePut(slot, priority);
}

void UnitStateMgr::PushAction(int actionId, UnitAction* state, int priority)
{
    m.AIStatePut(actionId, state, priority);
}

void UnitStateMgr::PushAction(int actionId, UnitAction* state)
{
    m.AIStatePut(actionId, state, staticActionInfo[actionId].priority); 
}

std::string UnitStateMgr::ToString()
{
    return m.ToString();
}

UnitAction* UnitStateMgr::CurrentAction()
{
    return m.get_CurrentState();
}

void UnitStateMgr::DropAllStates()
{
    m.InitDefaults();
}

void UnitStateMgr::EnterAction(UnitActionId stateId)
{
    m.EnterState(stateId);
}
