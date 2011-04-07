
#include "spell_script_includes.h"

enum SpellHandlerTypemask{

    TYPE_PROC_HANDLER   = 0x1,

};

/*
template<typename Base>
struct ProcHandler : public Base
{
    explicit ProcHandler() { sub |= TYPE_PROC_HANDLER; }

    virtual void OnProc(AuraProc_Args& data) const = 0;
} sc_field;


struct A{} sc_field;
struct B{} sc_field;
struct C{} sc_field;

struct U : public C<B<A> >
{} sc_field;*/


/*
struct GGG
{
    struct ex_Divine_Aegis : public ProcHandler
    {
        void Register()
        {
            regs(EFFECT_INDEX_0,47509,47511,47515,47753/ *,47753* /);
        }

        void OnProc(AuraProc_Args& proc) const
        {
            proc.basepoints[0] = proc.damage * proc.triggerAmount/100;
            proc.triggered_spell_id = 47753;
        }
    } sc_field;

    struct ex_Improved_Shadowform : public ProcHandler
    {
        void Register()
        {
            regs(EFFECT_INDEX_0,47569, 47570);
        }

        void OnProc(AuraProc_Args& proc) const
        {
            if(!roll_chance_i(proc.triggerAmount))
                proc.result = SPELL_AURA_PROC_FAILED;
            else{
                proc.caster->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
                proc.caster->RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
            }
        }
    } sc_field;

    struct ex_Eye_for_an_Eye : public ProcHandler
    {
        void Register()
        {
            regs(EFFECT_INDEX_0,9799,25988);
        }

        void OnProc(AuraProc_Args& proc) const
        {
            proc.basepoints[0] = proc.triggerAmount*int32(proc.damage)/100;
            int32 half_health = (int32)proc.caster->GetMaxHealth()/2;
            if (proc.basepoints[0] > half_health)
                proc.basepoints[0] = half_health;

            proc.triggered_spell_id = 25997;
        }
    } sc_field;

    struct ex_Unstable_Power : public ProcHandler
    {
        void Register()
        {
            reg(EFFECT_INDEX_0,24658);
        }

        void OnProc(AuraProc_Args& proc) const
        {
            if (!proc.procSpell || proc.procSpell->Id == 24659)
                proc.result = SPELL_AURA_PROC_FAILED;
            else
                // Need remove one 24659 aura
                proc.caster->RemoveAuraHolderFromStack(24659);
        }
    } sc_field;

    struct ex_Twisted_Reflection : public ProcHandler
    {
        void Register()
        {
            reg(EFFECT_INDEX_0,21063);
        }

        void OnProc(AuraProc_Args& proc) const
        {
            proc.triggered_spell_id = 21064;
        }
    } sc_field;

    struct ex_Mana_Leech : public ProcHandler
    {
        void Register()
        {
            reg(EFFECT_INDEX_0,28305);
        }

        void OnProc(AuraProc_Args& proc) const
        {
            // Cast on owner
            proc.target = proc.caster->GetOwner();
            if(!proc.target)
                proc.terminate(SPELL_AURA_PROC_FAILED);
            else
                proc.triggered_spell_id = 34650;
        }
    } sc_field;

    struct ex_Empowered_Renew : public ProcHandler
    {
        USE_IN(EFFECT_INDEX_0,63534,63542,63543);

        void OnProc(AuraProc_Args& proc) const
        {
            struct AuraProc_Args2 : AuraProc_Args {
                void OnProc()
                {
                    if (!procSpell)
                    {
                        terminate(SPELL_AURA_PROC_FAILED);
                        return;
                    }

                    // Renew
                    Aura* healingAura = target->GetAura(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_PRIEST, UI64LIT(0x40), 0, caster->GetGUID());
                    if (!healingAura)
                    {
                        terminate(SPELL_AURA_PROC_FAILED);
                        return;
                    }

                    int32 healingfromticks = healingAura->GetModifier()->m_amount * GetSpellAuraMaxTicks(procSpell);
                    basepoints[0] = healingfromticks * triggerAmount / 100;
                    triggered_spell_id = 63544;
                }
            };

            ((AuraProc_Args2&)proc).OnProc();
        }

    } sc_field;

    struct ex_Improved_Devouring_Plague : public ProcHandler
    {
        USE_IN(EFFECT_INDEX_0,63625,63626,63627);

        void OnProc(AuraProc_Args& proc) const
        {
            if (!proc.procSpell)
            {
                proc.terminate(SPELL_AURA_PROC_FAILED);
                return;
            }

            Aura* leechAura = proc.target->GetAura(SPELL_AURA_PERIODIC_LEECH, SPELLFAMILY_PRIEST, UI64LIT(0x02000000), 0, proc.caster->GetGUID());
            if (leechAura)
            {
                int32 damagefromticks = leechAura->GetModifier()->m_amount * GetSpellAuraMaxTicks(proc.procSpell);
                proc.basepoints[0] = damagefromticks * proc.triggerAmount / 100;
                proc.triggered_spell_id = 63675;
            }
            else
                proc.terminate(SPELL_AURA_PROC_FAILED);
        }
    } sc_field;

/ *
    struct Vampiric_Embrace : public ProcHandler
    {
        void Register()
        {
            reg(15286);
        }

        void OnProc(AuraProc_Args& proc) const
        {
            // Return if self damage
            if (proc.caster == proc.target)
            {
                proc.terminate(SPELL_AURA_PROC_FAILED);
                return;
            }

            // Heal amount - Self/Team
            int32 team = proc.triggerAmount*proc.damage/500;
            int32 self = proc.triggerAmount*proc.damage/100 - team;
            proc.caster->CastCustomSpell(proc.caster,15290,&team,&self,NULL,true,proc.castItem,proc.triggeredByAura);
            proc.terminate(SPELL_AURA_PROC_OK);
        }
    } sc_field;* /

} proc_handlers;
*/

