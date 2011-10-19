#pragma once
#include "Common.h"

//stun > root > conf > fear

/** List of default unit's state providen by MaNGOS */
enum UnitActionId
{
    UnitAction_Idle,
    UnitAction_DoWaypoints,
    UnitAction_Chase,
    UnitAction_Feared,
    UnitAction_Confused,
    UnitAction_Root,
    UnitAction_Stun,
    UnitAction_Effect,
    UnitAction_End,
};

/** The ideology is:
    command that cames from AI has low AI priority (but still depends on command type),
    command that cames from auras/spell effects has higher priority */
enum UnitCommandSource
{
    UnitCommand_AI,
    UnitCommand_Aura,
    UnitCommand_Effect, // should be removed probably
    UnitCommand_End,
};

class UnitAction;

struct ActionInfo
{
    ActionInfo(UnitAction* _state, int _priority, UnitCommandSource _source, bool _restoreable)
        : state(_state), priority(_priority), source(_source), restoreable(_restoreable)
    {
    }

    UnitAction* state;
    int priority;
    UnitCommandSource source; 
    bool restoreable;
};

class UnitStateMgr
{
    class ActionPriorityQueue& m;
    UnitStateMgr(const UnitStateMgr&);
    UnitStateMgr& operator = (const UnitStateMgr&);
public:

    explicit UnitStateMgr(Unit* owner);
    ~UnitStateMgr();

    void InitDefaults();

    /** Traditional MaNGOS way to update timers which i don't like */
    void Update(uint32 diff);

    /**	Finalizes slot's state and enters into most prioritized state.
        Hm.. should i allow drop inactive states?
        State will be deleted, but 'Finalize' will not be called if state is not current state. And it should not!*/
    void DropAction(int actionId);

    void DropAllStates();

    void PushAction(UnitActionId actionId);

    //void PushState(ActionInfo info);

    void PushAction(UnitActionId actionId, int priority);

    void PushAction(int actionId, UnitAction* state, int priority);

    void PushAction(int actionId, UnitAction* state);

    void EnterAction(UnitActionId stateId);

    UnitAction* CurrentAction();

    std::string ToString();
};

class Unit;
class UnitAction
{
public:
    virtual ~UnitAction() {}

    virtual void Interrupt(Unit &) = 0;
    virtual void Reset(Unit &) = 0;
    virtual void Initialize(Unit &) = 0;
    virtual void Finalize(Unit &) = 0;

    /* Returns true to show that state expired and can be finalized. */
    virtual bool Update(Unit &, const uint32& diff) = 0;
};