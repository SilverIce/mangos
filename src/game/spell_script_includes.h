#pragma once

#include "SpellScript.h"
#include "Unit.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Spell.h"

#define CONCAT(x, y) CONCAT1 (x, y)
#define CONCAT1(x, y) x##y


/** Generates unique script name*/
#define sc_field                    CONCAT(spell_script_,__LINE__)

#define USE_IN(eff,Idx,...)         void Register() { regs(eff,Idx,__VA_ARGS__);}
#define regs(eff,Idx,...)           uint32 spellIds[] = { __VA_ARGS__ }; reg(eff,Idx,spellIds);


// #define regs(...)\
//     uint32 ids[] = { __VA_ARGS__ };\
//     reg(ids);\
