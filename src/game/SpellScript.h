#pragma once

#include "Common.h"
#include "DBCEnums.h"
#include "Policies/Singleton.h"
//#include "Utilities/UnorderedMapSet.h"

struct SpellScript;
struct ProcHandler;

class SpellScriptManager
{
    struct ScriptsSet
    {
        ScriptsSet() : spell(0), proc(0) {}
        SpellScript * spell;
        ProcHandler * proc;
    };
public:
    enum{
        MAX_SPELL_ID = 120000,
    };

    explicit SpellScriptManager();
    ~SpellScriptManager();

    // shoul be called at startup, only once, after all *.dbc files loaded
    void Initialize();

    SpellScript const* GetScript(uint32 spellId) const
    {
        if (const ScriptsSet * sc = GetScriptsSet(spellId))
            return sc->spell;
        return NULL;
    }

    ProcHandler const* GetProcScript(uint32 spellId) const
    {
        if (const ScriptsSet * sc = GetScriptsSet(spellId))
            return sc->proc;
        return NULL;
    }

    void Register(uint32 spellId, SpellScript* sc);
    void Register(uint32 spellId, ProcHandler* sc);

private:
    friend struct SpellScript;

    const ScriptsSet * GetScriptsSet(uint32 spellId) const
    {
        assert(spellId <= MAX_SPELL_ID);
        return spell_handlers[spellId];
    }

    ScriptsSet * GetScriptsSet(uint32 spellId)
    {
        assert(spellId <= MAX_SPELL_ID);
        if (!spell_handlers[spellId])
            spell_handlers[spellId] = new ScriptsSet();
        return spell_handlers[spellId];
    }

    //typedef std::map<uint32, SpellScript*> SpellHandlerMap;
    //SpellHandlerMap spell_handlers;
    ScriptsSet* spell_handlers[MAX_SPELL_ID+1];
};

#define sSpellScriptMgr MaNGOS::Singleton<SpellScriptManager>::Instance()

enum SpellScriptCallReason
{
    SpellScriptCall_None,
    SpellScriptCall_DummyAuraProc,
};

struct SpellScriptArgs
{
    explicit SpellScriptArgs(SpellScriptCallReason r) : reason(r) {}

    SpellScriptCallReason const reason;
};

#define handled_by_scripts(code)

class Spell;
class SpellAuraHolder;
class Aura;
class Unit;
class Item;
struct SpellEntry;

#define effect_script   struct : public SpellScript
#define proc_script     struct : public ProcHandler
#define regs(...)       uint32 spellIds[] = { __VA_ARGS__ }; reg(spellIds);

struct SpellScriptBase
{
    explicit SpellScriptBase();
    virtual void Register() = 0;
};

struct ProcHandler : SpellScriptBase
{
    // Aura related hoooks
    virtual void OnProc(struct AuraProc_Args&) const {}

    // handler for specific, rarely used calls
    virtual void OnSpecificCall(struct SpellScriptCallArgs& data) const {}

    #pragma region register stuff
    /// helpers, 'syntactic sugar' ;)
    inline void reg(uint32 spellId) { sSpellScriptMgr.Register(spellId,this);}

    template<int v>
    struct Int2Type
    {
        enum { value = v};
    };

    template<int N>
    inline void reg(uint32 (&array_)[N])
    {
        _reg(array_,Int2Type<N-1>());
    }

private:

    template<int N, typename Idx>
    inline void _reg(uint32 (&array_)[N], Idx)
    {
        reg(array_[Idx::value]);
        _reg(array_, Int2Type<Idx::value-1>());
    }

    template<int N>
    inline void _reg(uint32 (&array_)[N], Int2Type<0>)
    {
        reg(array_[0]);
    }
    #pragma endregion
};

struct SpellScript : SpellScriptBase
{
    /// Spell related hooks
    virtual void OnEffect(Spell&, SpellEffectIndex) const {}
    virtual void BeforeHit(Spell&, SpellEffectIndex) const {}
    virtual void OnHit(Spell&, SpellEffectIndex) const {}
    virtual void AfterHit(Spell&, SpellEffectIndex) const {}

    /// Aura related hoooks
    virtual void OnApply(SpellAuraHolder&, SpellEffectIndex) const {}
    virtual void OnRemove(SpellAuraHolder&, SpellEffectIndex) const {}
    virtual void OnEffectPeriodic(SpellAuraHolder&, SpellEffectIndex) const {}

    /// handler for specific, rarely used calls
    virtual void OnSpecificCall(SpellScriptArgs& data) const {}

    #pragma region register stuff
    /// helpers, 'syntactic sugar' ;)
    inline void reg(uint32 spellId) { sSpellScriptMgr.Register(spellId,this);}

    template<int v>
    struct Int2Type
    {
        enum { value = v};
    };

    template<int N>
    inline void reg(uint32 (&array_)[N])
    {
        _reg(array_,Int2Type<N-1>());
    }

private:

    template<int N, typename Idx>
    inline void _reg(uint32 (&array_)[N], Idx)
    {
        reg(array_[Idx::value]);
        _reg(array_, Int2Type<Idx::value-1>());
    }

    template<int N>
    inline void _reg(uint32 (&array_)[N], Int2Type<0>)
    {
        reg(array_[0]);
    }
    #pragma endregion

protected:
    uint8 sub;
};

struct AuraProc_Args : public SpellScriptArgs
{
    AuraProc_Args(Unit* th, int32 * bp, SpellEffectIndex idx) :

        SpellScriptArgs(SpellScriptCall_DummyAuraProc),
        procSpell(0), dummySpell(0), basepoints(bp), caster(th),
        target(0), castItem(0),
        result(SPELL_AURA_PROC_OK), effIndex(idx),
        damage(0), triggered_spell_id(0), triggerAmount(0)
    {
    }

    SpellEntry const *  procSpell;
    SpellEntry const *  dummySpell;
    int32 *             basepoints;
    Unit *              caster;
    Unit *              target;
    Item *              castItem;

    SpellAuraProcResult result;
    SpellEffectIndex    effIndex;
    uint32              triggered_spell_id;
    uint32              damage;
    int32               triggerAmount;
};
