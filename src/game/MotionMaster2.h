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
    /**	random/waypoint/follow movement */
    OutOfCombat,
    /**	chase/ranged movement */
    Combat,
    /** Simple states: short lifetimes, no internal states, should be removed on interrupt.
        Example: charge, jump, home movement, distract  etc. */
    Effect,

    Feared,     
    Confused,
    Root,
    Stun,

    AIActionType_Count,
};

/**	feel free to edit */
enum AIStatePriorities
{
    PriorityIdle,
    PriorityOutOfCombat,
    PriorityCombat,
    PriorityEffect,

    PriorityDied,       // idle slot ?
    PriorityFeared,     // feared slot
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

    typedef AIState* Slot ;

    class AIStateWrap
    {
    protected:
        AIState * m_state;
        bool initialized;

        AIStateWrap& operator = (const AIStateWrap&);
        AIStateWrap(const AIStateWrap&);

    public:

        AIStateWrap() : m_state(0), initialized(false) {}
        ~AIStateWrap() { delete m_state; }

        const char* TypeName() const { return (m_state ? typeid(*m_state).name() : "<empty>");}
        
        operator bool() const { return m_state != NULL;}
        bool operator == (const AIState * state) const { return m_state == state;}
        bool operator != (const AIState * state) const { return m_state != state;}

        void operator = (AIState * state)
        {
            if (m_state == state)
                return;
            delete m_state;
            initialized = false;
            m_state = state;
        }
    };

    class AIStateCurrent : public AIStateWrap
    {
    public:

        AIState* AI() const { return m_state;}

        void Initialize(MotionMasterImpl * m)
        {
            Lock l(m);
            if (!initialized)
            {
                initialized = true;
                if (m_state) m_state->Initialize(m->owner);
            }
            else
                if (m_state) m_state->Reset(m->owner);
        }
        void Finalize(MotionMasterImpl * m)
        {
            Lock l(m);
            if (m_state) m_state->Finalize(m->owner);
        }
        void Interrupt(MotionMasterImpl * m)
        {
            Lock l(m);
            if (m_state) m_state->Interrupt(m->owner);
        }
        bool Update(MotionMasterImpl * m, uint32 diff)
        {
            Lock l(m);
            return m_state->Update(m->owner, diff);
        }
    };
/*
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

        / ** Need lock MotionMasterImpl before call these methods * /

        / ** Only for current, top slot * /
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
        AIState * operator ->() const { return m_state;}
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

*/
    MotionMasterImpl(Unit& Owner);

    ~MotionMasterImpl()
    {
    }

    AIStateWrap& get_AIState(AIStateType slot)     { return m_states[slot];}
    AIStateCurrent& get_CurrentState()             { return (AIStateCurrent&)m_states[m_slot];}
    //Slot& get_CurrentSlot()                      { return m_states[m_slot];}
    //Slot& get_Slot(AIStateType slot)             { return m_states[slot];}
    AIStateType get_CurrentSlotType()        const { return m_slot;}
    void set_AIState(AIStateType slot, AIState* s) { Lock l(this); m_states[slot] = s;}
    void set_CurrentSlot(AIStateType slot)         { if (EnsureNotLocked()) m_slot = slot;}
    int get_Priority(AIStateType slot)       const { return m_priorities[slot];}
    void set_Priority(AIStateType slot, int prio)  { if (EnsureNotLocked()) m_priorities[slot] = prio;}
    void reset_Priority(AIStateType slot);

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
       memset(&m_priorities[0], 0, sizeof m_priorities);
       EnterState(Idle);
    }

    void Update(uint32 diff)
    {
        if (!EnsureNotLocked())
            return;
        if (!get_CurrentState().Update(this, diff))
            AIStateDrop(get_CurrentSlotType());
    }

    bool IsOnTop(AIStateType slot) { return get_CurrentSlotType() == slot && get_CurrentState();}
    bool Initialized()  { return get_CurrentState();}

    /**	Finalizes slot's state and enters into most prioritized state.
        Hm.. should i allow drop inactive states?
        State will be deleted, but 'Finalize' will not be called if state is not current state. And it should not!*/
    void AIStateDrop(AIStateType slot)
    {
        if (!EnsureNotLocked())
            return;
        set_Priority(slot, -1);
        EnterMaxPrioritized();
        if (get_CurrentSlotType() != slot)
            set_AIState(slot, NULL);
    }

    /**	Tries to create a new standart state for given slot and enters into most prioritized state.
        TODO: 'standart state' type shouldn't be determined by slot type,
        need implement state creator, to create a new state for specified unit and stateId. */
    void AIStatePut(AIStateType slot)
    {
        if (!EnsureNotLocked())
            return;
        AIStatePut(slot, InitStandartState(slot,owner));
    }

    /**	Tries to create a new standart state for given slot and enters into most prioritized state.
        TODO: 'standart state' type shouldn't be determined by slot type,
        need implement state creator, to create a new state for specified unit and stateId. */
    void AIStatePut(AIStateType slot, int priority)
    {
        if (!EnsureNotLocked())
            return;
        AIStatePut(slot, InitStandartState(slot,owner), priority);
    }

    /**	Places an AI state with given priority into the slot and enters into most prioritized state. */
    void AIStatePut(AIStateType slot, AIState * new_state, int priority)
    {
        if (!EnsureNotLocked())
            return;

        set_Priority(slot, priority);
        AIStateType active_slot = FindMaxPrioritizedSlot();
        if (slot == active_slot)
            PushNewState(active_slot, new_state);
        else
        {
            EnterState(active_slot);
            set_AIState(slot, new_state);   // put non initialized state
        }
    }

    /**	Places an AI state into the slot and enters into most prioritized state. */
    void AIStatePut(AIStateType slot, AIState * new_state)
    {
        if (!EnsureNotLocked())
            return;

        /**	Strategy is: mark 'slot' as existent and find the most prioritized slot,
            in case prioritized slot is 'new_state' slot - push 'new_state' as current state
            or enter into max prioritized state and put not initialized 'new_state' into 'slot'
        */
        reset_Priority(slot);
        AIStateType selected_slot = FindMaxPrioritizedSlot();
        if (slot == selected_slot)
            PushNewState(selected_slot, new_state);
        else
        {
            EnterState(selected_slot);
            set_AIState(slot, new_state);   // put non initialized state, it will be initialized when became current
        }
    }

    /**	Enters into most prioritized state. */
    void EnterMaxPrioritized()
    {
        if (!EnsureNotLocked())
            return;
        AIStateType new_slot = FindMaxPrioritizedSlot();
        EnterState(new_slot);
    }

    /**	Searches for the slot with highest priority. Empty slots are included into the search.
        If no slots found Idle state will be choosen. */
    AIStateType FindMaxPrioritizedSlot() const
    {
        int prio = 0;
        AIStateType max_slot = Idle;
        for (AIStateType slot = Idle; slot < AIActionType_Count; ++((int&)slot))
        {
            if (/*get_AIState(slot) &&*/ get_Priority(slot) > prio)
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

        if (slot == get_CurrentSlotType())
        {
            get_CurrentState().Finalize(this);
        }
        else
        {
            get_CurrentState().Interrupt(this);
        }

        if (!get_AIState(slot))
        {
            set_AIState(slot, InitStandartState(slot,owner));
        }

        set_CurrentSlot(slot);
        get_CurrentState().Initialize(this);
    }

    /** Finalizes or interrupts current state and makes a new_state to be current.*/
    void PushNewState(AIStateType slot, AIState * new_state)
    {
        // same pointers
        if (get_AIState(slot) == new_state)
            return;

        if (slot == get_CurrentSlotType())
        {
            get_CurrentState().Finalize(this);
        }
        else
        {
            get_CurrentState().Interrupt(this);
        }
        set_AIState(slot, new_state);
        set_CurrentSlot(slot);
        get_CurrentState().Initialize(this);
    }

    std::string ToString();

private:

    static AIState * InitStandartState(AIStateType slot, Unit& owner);

    bool m_locked;
    Unit & owner;
    AIStateType m_slot;
    AIStateWrap m_states[AIActionType_Count];
    int m_priorities[AIActionType_Count];
};

