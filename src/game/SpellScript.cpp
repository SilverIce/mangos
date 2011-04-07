#include "SpellScript.h"
#include "Utilities/UnorderedMapSet.h"
#include "Policies/SingletonImp.h"
#include "DBCStores.h"
#include "Log.h"

SpellScriptBase::SpellScriptBase()
{
    sSpellScriptMgr.scripts_to_initialize.push_back(this);
}

INSTANTIATE_SINGLETON_1(SpellScriptManager);

extern void Init_EffectHandlers();
extern void Init_AuraHandlers();

void SpellScriptManager::Initialize()
{
    Init_EffectHandlers();
    Init_AuraHandlers();

    // TODO: get real max spell Id
    for (uint32 Id = 0; Id < 80000; ++Id)
    {
        if (const SpellEntry * entry = sSpellStore.LookupEntry(Id))
            spells_by_family.insert(SpellsbyFamily::value_type(entry->SpellFamilyName,entry));
    }

    struct _initializer{
        void operator () (SpellScriptBase * sc) const { sc->Register(); }
    };

    struct _initializer2{
        void operator () (SpellScriptBase * sc) const {
            if (sc) sLog.outError("SpellScript %s doesn't affects any spell", typeid(sc).name());
        }
    };

    std::for_each(scripts_to_initialize.begin(),scripts_to_initialize.end(), _initializer());
    //std::for_each(scripts_to_initialize.begin(),scripts_to_initialize.end(), _initializer2());

    sLog.outString("SpellScriptManager: %u spell scripts initialized", scripts_to_initialize.size());
    // should i clean lists?
    scripts_to_initialize.clear();
    spells_by_family.clear();
}

void SpellScriptManager::Register(uint32 spellId, SpellEffects effname, SpellEffectIndex Idx, EffectHandler* sc)
{
    if (!Validate(spellId,effname,Idx,sc))
        return;

    EffectHandler *& sc2 = InitScriptsSet(spellId)->spell[Idx];
    if (sc2)
        sLog.outError("EffectHandler '%s' attemps to bind to spell %u that already occupied by '%s'", typeid(*sc).name(), spellId, typeid(sc2).name());
    else
        sc2 = sc;
}

void SpellScriptManager::Register(uint32 spellId, AuraType auraname, SpellEffectIndex Idx, AuraHandler2* sc)
{
    if (!Validate(spellId,auraname,Idx,sc))
        return;

    AuraHandler2 *& sc2 = InitScriptsSet(spellId)->proc[Idx];
    if (sc2)
        sLog.outError("AuraHandler '%s' attemps to bind to spell %u that already occupied by '%s'", typeid(*sc).name(), spellId, typeid(sc2).name());
    else
        sc2 = sc;
}

void SpellScriptManager::Register(SpellFamily family, flag96 familyFlags, SpellEffects effname, SpellEffectIndex Idx, EffectHandler* sc)
{
    SpellsbyFamilyBounds bounds = spells_by_family.equal_range(family);
    for (SpellsbyFamily::const_iterator it = bounds.first; it!= bounds.second; ++it)
    {
        const SpellEntry * entry = it->second;
        if ( ((flag96&)entry->SpellFamilyFlags) & familyFlags )
            Register(entry->Id, effname, Idx, sc);
    }
}

void SpellScriptManager::Register(SpellFamily family, flag96 familyFlags, AuraType auraname, SpellEffectIndex Idx, AuraHandler2* sc)
{
    SpellsbyFamilyBounds bounds = spells_by_family.equal_range(family);
    for (SpellsbyFamily::const_iterator it = bounds.first; it!= bounds.second; ++it)
    {
        const SpellEntry * entry = it->second;
        if ( ((flag96&)entry->SpellFamilyFlags) & familyFlags )
            Register(entry->Id, auraname, Idx, sc);
    }
}

SpellScriptManager::~SpellScriptManager()
{
    struct _initializer{
        inline void operator () (ScriptsSet * sc) const { delete sc; }
    };
    std::for_each(&spell_handlers[0], &spell_handlers[MAX_SPELL_ID+1], _initializer());
}

SpellScriptManager::SpellScriptManager()
{
    memset(spell_handlers, 0, sizeof SpellScriptManager::spell_handlers);
}

bool SpellScriptManager::Validate(uint32 spellId, SpellEffects effname, SpellEffectIndex Idx, EffectHandler* sc)
{
    const SpellEntry * entry = sSpellStore.LookupEntry(spellId);
    if (!entry)
    {
        sLog.outError("EffectHandler '%s' attemps to bind to not existing spell %u", typeid(*sc).name(), spellId);
        return false;
    }

    if (entry->Effect[Idx] != effname)
    {
        // This is not an error
        sLog.outError("EffectHandler '%s' attemps to bind to spell %u (%s), but ", typeid(*sc).name(), spellId, entry->SpellName[0]);
        return false;
    }

    return true;
}

bool SpellScriptManager::Validate(uint32 spellId, AuraType auraname, SpellEffectIndex Idx, AuraHandler2* sc)
{
    const SpellEntry * entry = sSpellStore.LookupEntry(spellId);
    if (!entry)
    {
        sLog.outError("AuraHandler '%s' attemps to bind to not existing spell %u", typeid(*sc).name(), spellId);
        return false;
    }

    if (entry->EffectApplyAuraName[Idx] != auraname)
    {
        // This is not an error
        sLog.outError("AuraHandler '%s' attemps to bind to spell %u (%s), but ", typeid(*sc).name(), spellId, entry->SpellName[0]);
        return false;
    }

    return true;
}