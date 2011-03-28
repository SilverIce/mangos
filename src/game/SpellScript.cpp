#include "SpellScript.h"
#include "Utilities/UnorderedMapSet.h"
#include "Policies/SingletonImp.h"
#include "DBCStores.h"

std::deque<SpellScriptBase*> scripts_to_initialize;

SpellScriptBase::SpellScriptBase()
{
    scripts_to_initialize.push_back(this);
}

INSTANTIATE_SINGLETON_1(SpellScriptManager);

void SpellScriptManager::Initialize()
{
    struct _initializer{
        void operator () (SpellScriptBase * sc) const { sc->Register(); }
    };

    std::for_each(scripts_to_initialize.begin(),scripts_to_initialize.end(), _initializer());
    // should i clear list?
    scripts_to_initialize.clear();
}

void SpellScriptManager::Register(uint32 spellId, SpellScript* sc)
{
    GetScriptsSet(spellId)->spell = sc;
}

void SpellScriptManager::Register(uint32 spellId, ProcHandler* sc)
{
    GetScriptsSet(spellId)->proc = sc;
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