#pragma once

#include "Common.h"
#include "MovementGenerator.h"
#include "Log.h"
#include <sstream>

typedef MovementGenerator AIState;

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

        bool ontop;

        Slot& operator = (const Slot&);
        Slot(const Slot&);

    public:
        Slot() : m_state(0), initialized(false), interrupted(false), ontop(false) {}
        ~Slot()
        {
            if (m_state)
                sLog.outError("memleak");
        }

        /** Need lock MotionMasterImpl before call these methods */

        /** Only for current, top slot */
        void OnRefreshTop(MotionMasterImpl * u, AIState * new_state)
        {
            Put(u, new_state);
            OnBecameOnTop(u);
        }

        void Drop(MotionMasterImpl * u)
        {
            _put(u, NULL);
        }

        void Put(MotionMasterImpl * u, AIState * state)
        {
            _put(u, state);
        }

        void OnBecameOnTop(MotionMasterImpl * u)
        {
            ontop = true;
            if (initialized)
            {
                if (!interrupted)
                {
                    sLog.outError("strange");
                    return;
                }
                interrupted = false;
                Lock l(u);
                getAI()->Reset(u->owner);
            }
            else
            {
                if (interrupted)
                {
                    sLog.outError("strange");
                    return;
                }
                initialized = true;
                Lock l(u);
                getAI()->Initialize(u->owner);
            }
        }

        void OnGoDown(MotionMasterImpl * u)
        {
            ontop = false;
            if (interrupted || !getAI())
            {
                sLog.outError("strange");
                return;
            }

            interrupted = true;
            Lock l(u);
            getAI()->Interrupt(u->owner);
        }

        operator bool() const { return m_state != NULL; }
        AIState * getAI() const { return m_state;}
    private:
        void _put(MotionMasterImpl * u, AIState * new_state)
        {
            if (new_state == m_state)
            {
                sLog.outError("strange");
                return;
            }
            if (m_state)
            {
                Lock l(u);
                if (initialized && !interrupted)
                    m_state->Finalize(u->owner);
                delete m_state;
            }
            ontop = false;
            initialized = false;
            interrupted = false;
            m_state = new_state;
        }
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

    /**	That set of lock/unlock etc methods must be removed in future.
        They are needed to detect some unsafe deep calls. */
    bool IsLocked() { return m_locked;}
    bool EnsureNotLocked()
    {
        if (IsLocked())
            sLog.outError("attemp to modify!");
        return !IsLocked();
    }
    /**	Locks or unlocks MotionMasterImpl to allow or skip state change calls. */
    void lock() {set_Locked(true);}
    void unlock() {set_Locked(false);}
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
        if (!EnsureNotLocked())
            return;
       _DropEverything();
       InitDefaultStatePriorities(&m_priorities[0]);
       EnterMaxPrioritized();
    }

    void _DropEverything()
    {
        for (AIStateType slot = Idle; slot < AIActionType_Count; ++((int&)slot))
        {
            if (get_AIState(slot))
                get_Slot(slot).Drop(this);
        }
    }

    void Update(uint32 diff)
    {
        lock();
        bool expired = !get_CurrentState()->Update(owner, diff);
        unlock();

        if (expired)
            AIStateDrop(get_CurrentSlotType());
    }

    bool IsOnTop(AIStateType slot) const { return get_CurrentSlotType() == slot && get_CurrentState();}
    bool Initialized()  { return get_CurrentSlot();}

    /**	Finalizes slot's state and enters into max prioritized state. */
    void AIStateDrop(AIStateType slot)
    {
        if (!EnsureNotLocked())
            return;
        get_Slot(slot).Drop(this);
        EnterMaxPrioritized();
    }

    /**	Places an new AI state with not standart priority into the slot and enters AI into max prioritized state. */
    void AIStatePut(AIStateType slot, AIState * new_state, int priority)
    {
        if (!EnsureNotLocked())
            return;
        AIStatePut(slot, new_state);
    }

    /**	Places an AI state into the slot and enters into max prioritized state. */
    void AIStatePut(AIStateType slot, AIState * new_state)
    {
        if (!EnsureNotLocked())
            return;
        get_Slot(slot).Put(this, new_state);
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
        // slot is already current slot and exists
        if (IsOnTop(slot))
            return;

        if (!get_AIState(slot))
            get_Slot(slot).Put(this, InitStandartState(slot,owner));

        // initialized and another slot
        if (slot != get_CurrentSlotType())
        {
            get_CurrentSlot().OnGoDown(this);
            set_CurrentSlot(slot);
        }
        get_CurrentSlot().OnBecameOnTop(this);
    }

    void SetCurrent(AIState * new_state)
    {
        ;
    }

    /** Finalizes or interrupts current state and makes a new_state to be current.*/
    void PushNewState(AIStateType slot, AIState * new_state)
    {
        // same pointers
        if (get_AIState(slot) == new_state)
            return;

        // initialized and another slot
        if (slot == get_CurrentSlotType())
        {
            get_CurrentSlot().OnRefreshTop(this, new_state);
        }
        else
        {
            get_Slot(slot).Put(this,new_state);
            get_CurrentSlot().OnGoDown(this);
            set_CurrentSlot(slot);
            get_CurrentSlot().OnBecameOnTop(this);
        }
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

