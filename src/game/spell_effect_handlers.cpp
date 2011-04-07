#include "spell_script_includes.h"

struct EffectHandlers
{
    struct EH_Noggenfogger_Elixir : public EffectHandler
    {
        USE_IN(SPELL_EFFECT_DUMMY,EFFECT_INDEX_0,16589);

        void OnEffect(OnEffectArgs& e) const
        {
            uint32 spell_id = 0;
            switch(urand(1, 3))
            {
            case 1: spell_id = 16595; break;
            case 2: spell_id = 16593; break;
            default:spell_id = 16591; break;
            }

            e.caster->CastSpell(e.caster, spell_id, true, NULL);
        }
    } sc_field;

    struct EH_Death_Coil : public EffectHandler
    {
        void Register()
        {
            reg(SPELLFAMILY_DEATHKNIGHT,flag96(0,0,0x2000),SPELL_EFFECT_DUMMY,EFFECT_INDEX_0);
        }

        void OnEffect(OnEffectArgs& e) const
        {
            Unit * caster = e.caster;
            Unit * unitTarget = e.target;
            if (caster->IsFriendlyTo(unitTarget))
            {
                if (!unitTarget || unitTarget->GetCreatureType() != CREATURE_TYPE_UNDEAD)
                    return;

                int32 bp = int32(e.damage * 1.5f);
                caster->CastCustomSpell(unitTarget, 47633, &bp, NULL, NULL, true);
            }
            else
            {
                int32 bp = e.damage;
                caster->CastCustomSpell(unitTarget, 47632, &bp, NULL, NULL, true);
            }
        }
    }sc_field;

    struct EH_Death_Strike : EffectHandler
    {
        void Register()
        {
            reg(SPELLFAMILY_DEATHKNIGHT,flag96(0,0,0x10),SPELL_EFFECT_DUMMY,EFFECT_INDEX_2);
        }

        void OnEffect(OnEffectArgs& e) const
        {
            Unit * caster = e.caster;
            Unit * unitTarget = e.target;
            uint32 count = 0;
            Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
            for(Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr!=auras.end(); ++itr)
            {
                if (itr->second->GetSpellProto()->Dispel == DISPEL_DISEASE &&
                    itr->second->GetCasterGuid() == caster->GetObjectGuid())
                {
                    ++count;
                    // max. 15%
                    if (count == 3)
                        break;
                }
            }

            int32 bp = int32(count * caster->GetMaxHealth() * e.spell->m_spellInfo->DmgMultiplier[EFFECT_INDEX_0] / 100);

            // Improved Death Strike (percent stored in nonexistent EFFECT_INDEX_2 effect base points)
            Unit::AuraList const& auraMod = caster->GetAurasByType(SPELL_AURA_ADD_FLAT_MODIFIER);
            for(Unit::AuraList::const_iterator iter = auraMod.begin(); iter != auraMod.end(); ++iter)
            {
                // only required spell have spellicon for SPELL_AURA_ADD_FLAT_MODIFIER
                if ((*iter)->GetSpellProto()->SpellIconID == 2751 && (*iter)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT)
                {
                    bp += (*iter)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_2) * bp / 100;
                    break;
                }
            }

            caster->CastCustomSpell(caster, 45470, &bp, NULL, NULL, true);
        }
    } sc_field;


};

void Init_EffectHandlers()
{
    new EffectHandlers();
}