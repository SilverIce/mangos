#pragma once

#include "Common.h"
#include "DBCEnums.h"
#include "Policies/Singleton.h"
//#include "Utilities/UnorderedMapSet.h"
//#include <map>

struct EffectHandler;
struct AuraHandler2;
struct SpellScriptBase;

struct flag96
{
    uint32 p3, p2, p1;

    /** To create 0x00000001 00000000 00004000 mask you should write:
        flag96(0x1, 0, 0x4000). */
    flag96(uint32 _p1=0,uint32 _p2=0,uint32 _p3=0)
    {
        p3 = _p3;
        p2 = _p2;
        p1 = _p1;
    }

    bool operator & (const flag96& other) const
    {
        return (p3 & other.p3) || (p2 & other.p2) || (p1 & other.p1);
    }
};

class SpellScriptManager
{
    struct ScriptsSet
    {
        ScriptsSet() {memset(this,0,sizeof this);}
        EffectHandler * spell[MAX_EFFECT_INDEX];
        AuraHandler2 * proc[MAX_EFFECT_INDEX];
    };
public:
    enum{
        MAX_SPELL_ID = 120000,
    };

    explicit SpellScriptManager();
    ~SpellScriptManager();

    // shoul be called at startup, only once, after all *.dbc files loaded
    void Initialize();

    EffectHandler const* GetScript(uint32 spellId, SpellEffectIndex Idx) const
    {
        if (const ScriptsSet * sc = GetScriptsSet(spellId))
            return sc->spell[Idx];
        return NULL;
    }

    AuraHandler2 const* GetProcScript(uint32 spellId, SpellEffectIndex Idx) const
    {
        if (const ScriptsSet * sc = GetScriptsSet(spellId))
            return sc->proc[Idx];
        return NULL;
    }

    static bool Validate(uint32 spellId, SpellEffects effname, SpellEffectIndex Idx, EffectHandler* sc);
    static bool Validate(uint32 spellId, AuraType auraname, SpellEffectIndex Idx, AuraHandler2* sc);

    void Register(uint32 spellId, SpellEffects effname, SpellEffectIndex Idx, EffectHandler* sc);
    void Register(uint32 spellId, AuraType auraname, SpellEffectIndex Idx, AuraHandler2* sc);

    void Register(SpellFamily family, flag96 familyFlags, SpellEffects effname, SpellEffectIndex Idx, EffectHandler* sc);
    void Register(SpellFamily family, flag96 familyFlags, AuraType auraname, SpellEffectIndex Idx, AuraHandler2* sc);

private:
    friend struct SpellScriptBase;

    const ScriptsSet * GetScriptsSet(uint32 spellId) const
    {
        assert(spellId <= MAX_SPELL_ID);
        return spell_handlers[spellId];
    }

    ScriptsSet * InitScriptsSet(uint32 spellId)
    {
        assert(spellId <= MAX_SPELL_ID);
        ScriptsSet *& sc = spell_handlers[spellId];
        if (!sc)
            sc = new ScriptsSet();
        return sc;
    }

    //typedef std::map<uint32, SpellScript*> SpellHandlerMap;
    //SpellHandlerMap spell_handlers;
    ScriptsSet* spell_handlers[MAX_SPELL_ID+1];
    std::deque<SpellScriptBase*> scripts_to_initialize;
    typedef std::multimap<uint8, const struct SpellEntry*> SpellsbyFamily;
    typedef std::pair<SpellsbyFamily::const_iterator,SpellsbyFamily::const_iterator> SpellsbyFamilyBounds;
    /** Spell entries sorted by their families for fast sript registration.
        Can be cleaned after registration has ended. */
    SpellsbyFamily spells_by_family;
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


class Spell;
class SpellAuraHolder;
class Aura;
class Unit;
class Item;
struct SpellEntry;

#define handled_by_script(code)             // marks selected code, disables code generation

struct SpellScriptBase
{
    explicit SpellScriptBase();
    virtual void Register() = 0;
};

struct SpellScriptInitializer {}; 

struct AuraHandler2 : public SpellScriptBase
{
    virtual void OnApply(struct OnApplyArgs& e, bool apply) const {}
    virtual void OnProc(struct AuraProc_Args&) const {}
    virtual void OnEffectPeriodic(SpellAuraHolder&, SpellEffectIndex) const {}

    // handler for specific, rarely used calls
    //virtual void OnSpecificCall(struct SpellScriptCallArgs& data) const {}

    #pragma region register stuff

    inline void Register(SpellFamily family, flag96 familyFlags, AuraType auraname, SpellEffectIndex Idx)
    {
        sSpellScriptMgr.Register(family,familyFlags,auraname,Idx,this);
    }

    /// helpers, 'syntactic sugar' ;)
    inline void reg(AuraType auraname, SpellEffectIndex Idx, uint32 spellId)
    {
        sSpellScriptMgr.Register(spellId,auraname,Idx,this);
    }

    template<int v>
    struct Int2Type
    {
        enum { value = v};
    };

    template<int N>
    inline void reg(AuraType effname, SpellEffectIndex effIdx, uint32 (&array_)[N])
    {
        _reg(effname, effIdx, array_, Int2Type<N-1>());
    }

private:

    template<int N, typename Idx>
    inline void _reg(AuraType effname, SpellEffectIndex effIdx, uint32 (&array_)[N], Idx)
    {
        reg(effname, effIdx, array_[Idx::value]);
        _reg(effname, effIdx, array_, Int2Type<Idx::value-1>());
    }

    template<int N>
    inline void _reg(AuraType effname, SpellEffectIndex effIdx, uint32 (&array_)[N], Int2Type<0>)
    {
        reg(effname, effIdx, array_[0]);
    }
    #pragma endregion
};

struct EffectHandler : public SpellScriptBase
{
    virtual void OnEffect(struct OnEffectArgs& effect) const {}
    //virtual void BeforeHit(Spell& spell, SpellEffectIndex) const {}
    //virtual void OnHit(Spell& spell, SpellEffectIndex) const {}
    //virtual void AfterHit(Spell& spell, SpellEffectIndex) const {}

    #pragma region register stuff
    /// helpers, 'syntactic sugar' ;)

    inline void reg(SpellFamily family, flag96 familyFlags, SpellEffects effname, SpellEffectIndex Idx)
    {
        sSpellScriptMgr.Register(family,familyFlags,effname,Idx,this);
    }

    /// helpers, 'syntactic sugar' ;)
    inline void reg(SpellEffects effname, SpellEffectIndex Idx, uint32 spellId)
    {
        sSpellScriptMgr.Register(spellId,effname,Idx,this);
    }

    template<int v>
    struct Int2Type
    {
        enum { value = v};
    };

    template<int N>
    inline void reg(SpellEffects effname, SpellEffectIndex effIdx, uint32 (&array_)[N])
    {
        _reg(effname, effIdx, array_, Int2Type<N-1>());
    }

private:

    template<int N, typename Idx>
    inline void _reg(SpellEffects effname, SpellEffectIndex effIdx, uint32 (&array_)[N], Idx)
    {
        reg(effname, effIdx, array_[Idx::value]);
        _reg(effname, effIdx, array_, Int2Type<Idx::value-1>());
    }

    template<int N>
    inline void _reg(SpellEffects effname, SpellEffectIndex effIdx, uint32 (&array_)[N], Int2Type<0>)
    {
        reg(effname, effIdx, array_[0]);
    }
    #pragma endregion
};

struct AuraProc_Args : public SpellScriptArgs
{
    AuraProc_Args(Unit* th, int32 * bp, SpellEffectIndex idx) :

        SpellScriptArgs(SpellScriptCall_DummyAuraProc),
        procSpell(0), dummySpell(0), basepoints(bp), caster(th),
        target(0), castItem(0),
        result(SPELL_AURA_PROC_OK), effIndex(idx),
        damage(0), triggered_spell_id(0), triggerAmount(0), need_terminate(false)
    {
    }

    void terminate(SpellAuraProcResult res)
    {
        need_terminate = true;
        result = res;
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
    bool                need_terminate;
};

struct OnEffectArgs
{
    OnEffectArgs(Spell * _spell, Unit * _caster, Unit * _target, int32& _damage, SpellEffectIndex _effIdx) :
        spell(_spell), caster(_caster), target(_target), damage(_damage), effIdx(_effIdx)
    {
    }

    Spell * spell;
    Unit * caster;
    Unit * target;
    int32& damage;
    SpellEffectIndex effIdx;
};

struct OnApplyArgs
{
    OnApplyArgs(Aura & _aura, Unit * _caster, Unit * _target, SpellEffectIndex _effIdx) :
        aura(_aura), caster(_caster), target(_target), effIdx(_effIdx)
    {
    }

    Aura & aura;
    Unit * caster;
    Unit * target;
    SpellEffectIndex effIdx;
};