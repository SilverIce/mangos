#include "MotionMaster2.h"
#include "ConfusedMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "Timer.h"

/*
enum AIStateType
{
    Idle,
    OutOfCombat,
    Follow,
    Combat,
    / ** Slot for state with short lifetime that cannot be reseted. * /
    Effect,

    Feared,
    Confuse,
    Root,
    Stun,


    AIActionType_Count,
};*/

//std::vector<int> AIStatePriorities(AIActionType_Count);

void MotionMasterImpl::InitDefaultStatePriorities(int * AIStatePriorities)
{
    AIStatePriorities[Idle] = 0;
    AIStatePriorities[OutOfCombat] = 10;
    //AIStatePriorities[Follow] = 20;
    AIStatePriorities[Combat] = 30;
    AIStatePriorities[Effect] = 40;
    AIStatePriorities[Feared] = 50;
    AIStatePriorities[Confused] = 60;
    AIStatePriorities[Root] = 70;
    AIStatePriorities[Stun] = 80;
}


MotionMasterImpl::MotionMasterImpl(Unit& Owner) :
    m_states(AIActionType_Count), m_priorities(AIActionType_Count),
    owner(Owner), m_locked(false), m_slot(Idle)
{
    InitDefaults();
}

std::string MotionMasterImpl::ToString() const
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
        str << "current slot " << AIStateType_Name[get_CurrentSlotType()] << ": " << typeid(*get_CurrentState()).name() << std::endl;
    for (AIStateType slot = Idle; slot < AIActionType_Count; ++((int&)slot))
    {
        str << "slot " << AIStateType_Name[slot] << ": ";
        if (get_AIState(slot))
            str << typeid(*get_AIState(slot)).name();
        else
            str << "empty";
        str << std::endl;
    }

    return str.str();
}

class IdleState_ : public MovementGenerator
{
    ShortTimeTracker tr;
public:

    IdleState_() : tr(10000) {}

    void Interrupt(Unit &) {}
    void Reset(Unit &) {}
    void Initialize(Unit &) {}
    void Finalize(Unit &) {}
    bool Update(Unit &, const uint32& diff) { tr.Update(diff); return !tr.Passed(); }

    virtual MovementGeneratorType GetMovementGeneratorType() const
    {
        return IDLE_MOTION_TYPE;
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