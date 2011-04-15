#pragma once

#include "Common.h"
#include "MovementGenerator.h"
#include "Log.h"
#include <sstream>

typedef MovementGenerator AIState;

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
//stun > root > conf > fear

/**	To future devs: do not add a new slot for each new movement/AI state,
    you should determine minimal and optimal amount of slots.
    One non stackable group of states per slot. */
enum AIStateType
{
    Idle,
    OutOfCombat,
    //Follow,     // permanent
    Combat,     // permanent
    /** Slot for state with short lifetime that cannot be reseted.
        Example: charge, jump etc*/
    Effect,

    Feared,     
    Confused,
    Root,
    Stun,

    AIActionType_Count,
};

enum AIStatePriorities
{
    PriorityIdle,
    PriorityOutOfCombat,
    PriorityFollow,     // permanent, how to clean?
    PriorityCombat,     // permanent, how to clean?
    /** Slot for state with short lifetime that cannot be reseted. */
    PriorityEffect,

    PriorityFeared,     
    PriorityConfused,
    PriorityRoot,
    PriorityStun,
};

class MotionMasterImpl
{
    struct Lock
    {
        Lock(MotionMasterImpl * _this) : me(_this)
        {
            me->set_Locked(true);
        }
        ~Lock() { me->set_Locked(false);}
        MotionMasterImpl * me;
    };

public:

    struct Slot
    {
    private:
        AIState * m_state;
        bool initialized; // initialized/reseted
        bool interrupted; // finalized/interrupted

    public:
        Slot() : m_state(0), initialized(false), interrupted(true) {}

        /** Need lock MotionMasterImpl before call these methods */
        void Drop(Unit &u)
        {
            if (!getAI())
                return;
            initialized = false;
            interrupted = false;
            AIState * old = m_state;
            m_state = 0;
            old->Finalize(u);
            delete old;
        }

        void Put(Unit &u, AIState * state)
        {
            if (state == m_state)
            {
                sLog.outError("strange");
                return;
            }
            Drop(u);
            m_state = state;
        }

        void OnBecameOnTop(Unit &u)
        {
            if (!getAI())
            {
                sLog.outError("strange");
                return;
            }

            if (initialized)
            {
                if (!interrupted)
                {
                    sLog.outError("strange");
                    return;
                }
                interrupted = false;
                getAI()->Reset(u);
            }
            else
            {
                initialized = true;
                getAI()->Initialize(u);
            }
        }

        void OnGoDown(Unit &u)
        {
            if (interrupted || !getAI())
            {
                sLog.outError("strange");
                return;
            }

            interrupted = true;
            getAI()->Interrupt(u);
        }

        operator bool() const { return m_state != NULL; }
        AIState * getAI() const { return m_state;}
    };

    MotionMasterImpl(Unit& Owner);

    ~MotionMasterImpl()
    {
        _DropEverything();
    }

    AIState* get_AIState(AIStateType slot)   const { return m_states[slot].getAI();}
    AIState* get_CurrentState()              const { return m_states[m_slot].getAI();}
    Slot& get_CurrentSlot()                        { return m_states[m_slot];}
    Slot& get_Slot(AIStateType slot)               { return m_states[slot];}
    AIStateType get_CurrentSlotType()        const { return m_slot;}
    //void set_State(AIStateType slot, AIState* s)   { if (EnsureNotLocked()) m_states[slot] = s;}
    void set_CurrentSlot(AIStateType slot)         { if (EnsureNotLocked()) m_slot = slot;}
    int get_Priority(AIStateType slot)       const { return m_priorities[slot];}
    void set_Priority(AIStateType slot, int prio)  { if (EnsureNotLocked()) m_priorities[slot] = prio;}
    //void set_Expired(AIStateType slot, bool exp);
    //bool is_Expired(AIStateType slot) const;

    bool IsLocked() { return m_locked;}
    bool EnsureNotLocked()
    {
        if (IsLocked())
            sLog.outError("attemp to modify!");
        return !IsLocked();
    }
    /**	Locks or unlocks MotionMasterImpl to allow or skip state change calls. */
    void set_Locked(bool lock)
    {
        if (lock && m_locked)
        {
            sLog.outError("recursive lock!");
        }
        else
            m_locked = lock;
    }

public:

    void InitDefaults()
    {
       _DropEverything();
       InitDefaultStatePriorities(&m_priorities[0]);
       EnterMaxPrioritized();
    }

    void _DropEverything()
    {
        Lock(this);
        for (AIStateType slot = Idle; slot < AIActionType_Count; ++((int&)slot))
        {
            if (get_AIState(slot))
                get_Slot(slot).Drop(owner);
        }
    }

    void Update(uint32 diff)
    {
        if (!get_CurrentState()->Update(owner, diff))
        {
            AIStateDrop(get_CurrentSlotType());
        }
    }

    bool IsOnTop(AIStateType slot) const { return get_CurrentSlotType() == slot && get_CurrentState();}

    /**	Finalizes slot and selects slot with highest priority */
    void AIStateDrop(AIStateType slot)
    {
        if (!EnsureNotLocked())
            return;
        get_Slot(slot).Drop(owner);
        EnterMaxPrioritized();
    }

    /**	Places new ai into the slot and selects slot with highest priority */
    void AIStatePut(AIStateType slot, AIState * new_state)
    {
        if (!EnsureNotLocked())
            return;
        get_Slot(slot).Put(owner, new_state);
        EnterMaxPrioritized();
    }

    /**	Enters into max prioritized state. */
    void EnterMaxPrioritized()
    {
        if (!EnsureNotLocked())
            return;
        AIStateType new_state = FindMaxPrioritizedSlot();
        EnterState(new_state);
    }

    /**	Searches for the slot with highest priority. Empty slots are excluded from the search.
        If no slots Idle state will be choosen. */
    AIStateType FindMaxPrioritizedSlot() const
    {
        int prio = 0;
        AIStateType max_slot = Idle;
        for (AIStateType slot = Idle; slot < AIActionType_Count; ++((int&)slot))
        {
            if (get_AIState(slot) && get_Priority(slot) > prio)
            {
                prio = get_Priority(slot);
                max_slot = slot;
            }
        }
        return max_slot;
    }

    void EnterState(AIStateType slot)
    {
        // slot is current slot already and exists
        if (IsOnTop(slot))
            return;

        AIState * new_state = get_AIState(slot);
        if (!new_state)
        {
            new_state = InitStandartState(slot, owner);
        } 

        PushNewState(slot, new_state);

        if (!get_AIState(slot))
        {
             AIState * new_state = InitStandartState(slot, owner);
        } 

        PushNewState(slot, new_state);
    }

    /** Finalizes or interrupts current state and makes a new_state to be current.*/
    void PushNewState(AIStateType slot, AIState * new_state)
    {
        // same pointers
        if (get_AIState(slot) != new_state)
            get_Slot(slot).Put(owner,new_state);

        // initialized and another slot
        if (slot != get_CurrentSlotType())
            get_CurrentSlot().OnGoDown(owner);

        set_CurrentSlot(slot);
        get_Slot(slot).OnBecameOnTop(owner);
    }

    std::string ToString() const;

private:

    static void InitDefaultStatePriorities(int * AIStatePriorities);
    static AIState * InitStandartState(AIStateType slot, Unit& owner);

    bool m_locked;
    Unit & owner;
    AIStateType m_slot;
    std::vector<Slot> m_states;
    std::vector<int> m_priorities;
};

