#pragma once

#include "Common.h"
#include "MovementGenerator.h"
#include "Log.h"
#include <sstream>
#include <algorithm>
#include "StateMgr.h"


class NullUnitState : public MovementGenerator
{
public:

    void Interrupt(Unit &) {}
    void Reset(Unit &) {}
    void Initialize(Unit &) {}
    void Finalize(Unit &) {}
    bool Update(Unit &, const uint32&) { return true; }

    virtual MovementGeneratorType GetMovementGeneratorType() const { return IDLE_MOTION_TYPE;}
};

enum eActionType{
    NonRestoreable = 0,
    Restoreable = 1,
};

struct StaticActionInfo
{
    explicit StaticActionInfo() : priority(0), restoreable(Restoreable) {}

    explicit StaticActionInfo(int _priority, eActionType _restoreable = Restoreable) : priority(_priority), restoreable(_restoreable) {}

    void operator()(int _priority, eActionType _restoreable = Restoreable) {
        priority = _priority;
        restoreable = _restoreable;
    }
    int priority;
    eActionType restoreable;
};

class staticActionInfo
{
public:

    staticActionInfo()
    {
        actioInfo[UnitAction_Idle](0);
        actioInfo[UnitAction_DoWaypoints](10);
        actioInfo[UnitAction_Chase](20);
        actioInfo[UnitAction_Confused](50);
        actioInfo[UnitAction_Feared](60);
        actioInfo[UnitAction_Stun](70);
        actioInfo[UnitAction_Root](80);
        actioInfo[UnitAction_Effect](90,NonRestoreable);
    }

    const StaticActionInfo& operator[](uint32 i) const { return actioInfo[i];}

private:
    StaticActionInfo actioInfo[UnitAction_End];
} staticActionInfo;

class ActionPriorityQueue
{
    struct Lock
    {
        Lock(ActionPriorityQueue * _this) : me(_this)
        {
            me->set_Locked(true);
        }
        ~Lock() { me->set_Locked(false);}
        ActionPriorityQueue * me;
    };

public:

    struct ActionInfo2 : public StaticActionInfo
    {
        explicit ActionInfo2(UnitAction * _state, int _stateId, int _priority, eActionType _restoreable = Restoreable)
            : StaticActionInfo(_priority,_restoreable), stateId(_stateId), action(_state) {}

        explicit ActionInfo2(UnitAction * _state, int _stateId, const StaticActionInfo& stat)
            : StaticActionInfo(stat), stateId(_stateId), action(_state) {}

        UnitAction * action;
        int stateId;
    };

    class ActionWrap
    {
    protected:
        UnitAction * action;
        bool initialized;

        ActionWrap& operator = (const ActionWrap&);
        ActionWrap(const ActionWrap&);


    public:

        ActionWrap() : action(new NullUnitState()), priority(0), stateId(0), initialized(false), restoreable(Restoreable)
        {}

        ActionWrap(const ActionInfo2& info)
            : action(info.action), priority(info.priority), stateId(info.stateId), initialized(false), restoreable(info.restoreable)
        {}

        ~ActionWrap() { delete action; }

        void Delete() { if(this != &NullState) delete this;}

        const char* TypeName() const { return (action ? typeid(*action).name() : "<empty>");}
        int priority;
        int stateId;
        eActionType restoreable;

        UnitAction* Action() const { return action;}

        void Initialize(ActionPriorityQueue * m)
        {
            Lock l(m);
            if (!initialized)
            {
                initialized = true;
                action->Initialize(m->owner);
            }
            else
                action->Reset(m->owner);
        }
        void Finalize(ActionPriorityQueue * m)
        {
            Lock l(m);
            action->Finalize(m->owner);
        }
        void Interrupt(ActionPriorityQueue * m)
        {
            Lock l(m);
            action->Interrupt(m->owner);
        }
        bool Update(ActionPriorityQueue * m, uint32 diff)
        {
            Lock l(m);
            return action->Update(m->owner, diff);
        }
    };

    static ActionWrap NullState;

    ActionPriorityQueue(Unit& Owner) : owner(Owner), m_locked(false)
    {
        InitDefaults();
    }

    ~ActionPriorityQueue() { InitDefaults();}

    /**	That set of lock/unlock etc methods must be removed in future.
        They are needed to detect some unsafe deep calls. */
    bool IsLocked() { return m_locked;}
    bool EnsureNotLocked()
    {
        if (IsLocked())
            sLog.outError("attemp to modify!");
        return !IsLocked();
    }
    /**	Locks or unlocks StateMgrImpl to allow or skip state change calls. */
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
        EnsureNotLocked();

        int end = m_states.size();
        for (int i = 0; i < end; ++i)
            m_states[i]->Delete();
        m_states.clear();
        m_currentState = &NullState;
    }

    void Update(uint32 diff)
    {
        EnsureNotLocked();

        ActionWrap* state = m_currentState;
        if (!state->Update(this, diff))
            DropState(state);
    }

    /**	Finalizes slot's state and enters into most prioritized state.
        Hm.. should i allow drop inactive states?
        State will be deleted, but 'Finalize' will not be called if state is not current state. And it should not!*/
    void DropState(int stateId)
    {
        if (ActionWrap* state = FindState(stateId))
            DropState(state);
    }

    void DropState(ActionWrap* state)
    {
        EnsureNotLocked();
        /** Finalize/Initialize are unsafe virtual calls, noone knows what they may cause
            So, better dereference old state, replace it with null state and finalize it.
            Then enter into most prioritized state
        */
        m_states.erase(std::remove(m_states.begin(),m_states.end(),state));
        if (m_currentState == state) {
            m_currentState = &NullState;
            state->Finalize(this);
            EnterMaxPrioritized();
        }
        state->Delete();
    }

    void EnterMaxPrioritized()
    {
        ActionWrap* new_slot = FindMaxPrioritizedState();
        MANGOS_ASSERT(new_slot);
        if (new_slot != m_currentState)
        {
            m_currentState = new_slot;
            m_currentState->Initialize(this);
        }
    }

    /**	Tries to create a new standart state for given slot and enters into most prioritized state.
        TODO: 'standart state' type shouldn't be determined by slot type,
        need implement state creator, to create a new state for specified unit and stateId. */
    void AIStatePut(int stateId)
    {
        ActionInfo2 info(CreateStandartState(stateId,owner), stateId, staticActionInfo[stateId]);
        AIStatePut(info);
    }

    /**	Tries to create a new standart state for given slot and enters into most prioritized state.
        TODO: 'standart state' type shouldn't be determined by slot type,
        need implement state creator, to create a new state for specified unit and stateId. */
    void AIStatePut(int stateId, int priority)
    {
        ActionInfo2 info(CreateStandartState(stateId,owner), stateId, priority);
        AIStatePut(info);
    }

    /**	Places an AI state with given priority and enters into most prioritized state. */
    void AIStatePut(int stateId, UnitAction* state, int priority)
    {
        ActionInfo2 info(state, stateId, priority);
        AIStatePut(info);
    }

    void AIStatePut(const ActionInfo2& newStateInfo)
    {
        EnsureNotLocked();

        ActionWrap * new_state = new ActionWrap(newStateInfo);
        if (m_currentState->priority <= new_state->priority)
        {
            ActionWrap * oldState = m_currentState;
            m_currentState = &NullState;

            if (new_state->stateId == oldState->stateId || oldState->restoreable == NonRestoreable)
            {
                m_states.erase(std::remove(m_states.begin(),m_states.end(),oldState));
                oldState->Finalize(this);
                oldState->Delete();
            }
            else
                oldState->Interrupt(this);

            m_states.push_back(new_state);
            EnterMaxPrioritized();
        } 
        else
        {
            if (ActionWrap * oldState = FindState(new_state->stateId)) {
                m_states.erase(std::remove(m_states.begin(),m_states.end(),oldState));
                oldState->Delete();
            }
            m_states.push_back(new_state);
        }
    }

    void EnterState(UnitActionId stateId)
    {
        ActionInfo2 info(CreateStandartState(stateId,owner), stateId, 1000);
        AIStatePut(info);
    }

    /**	Searches for the slot with highest priority. Empty slots are included into the search.
        If no slots found Idle state will be choosen. */
    ActionWrap* FindMaxPrioritizedState() const
    {
        int prio = 0;
        ActionWrap* selected = &NullState;
        int end = m_states.size();
        for (int slot = 0; slot < end; ++slot)
        {
            ActionWrap* state = m_states[slot];
            if (state->priority > prio)
            {
                prio = state->priority;
                selected = state;
            }
        }
        return selected;
    }

    ActionWrap* FindState(int stateId) const
    {
        int end = m_states.size();
        for (int slot = 0; slot < end; ++slot)
            if (m_states[slot]->stateId == stateId)
                return m_states[slot];
        return NULL;
    }

    UnitAction* get_CurrentState() const { return m_currentState->Action();}

    std::string ToString();

    static UnitAction * CreateStandartState(int stateId, Unit& owner);

private:

    Unit & owner;
    ActionWrap * m_currentState;
    std::vector<ActionWrap*> m_states;
    bool m_locked;
};

//StateMgrImpl::AIStateWrap StateMgrImpl::NullState();