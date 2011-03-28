
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
};


struct A{};
struct B{};
struct C{};

struct U : public C<B<A> >
{};*/

struct
{
    proc_script
    {
        void Register()
        {
            regs(47509,47511,47515,47753/*,47753*/);
        }

        void OnProc(AuraProc_Args& args) const
        {
            args.basepoints[0] = args.damage * args.triggerAmount/100.f;
            args.triggered_spell_id = 47753;
        }
    } ex_Divine_Aeglis;

    proc_script
    {
        void Register()
        {
            regs(47569, 47570);
        }

        void OnProc(AuraProc_Args& args) const
        {
            if(!roll_chance_i(args.triggerAmount))
                args.result = SPELL_AURA_PROC_FAILED;
            else{
                args.caster->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
                args.caster->RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
            }
        }
    } ex_Improved_Shadowform;

    proc_script
    {
        void Register()
        {
            regs(9799,25988);
        }

        void OnProc(AuraProc_Args& args) const
        {
            args.basepoints[0] = args.triggerAmount*int32(args.damage)/100;
            int32 half_health = (int32)args.caster->GetMaxHealth()/2u;
            if (args.basepoints[0] > half_health)
                args.basepoints[0] = half_health;

            args.triggered_spell_id = 25997;
        }
    }ex_Eye_for_an_Eye;

    proc_script
    {
        void Register()
        {
            reg(24658);
        }

        void OnProc(AuraProc_Args& args) const
        {
            if (!args.procSpell || args.procSpell->Id == 24659)
                args.result = SPELL_AURA_PROC_FAILED;
            else
                // Need remove one 24659 aura
                args.caster->RemoveAuraHolderFromStack(24659);
        }
    }ex_Unstable_Power;

    proc_script
    {
        void Register()
        {
            reg(21063);
        }

        void OnProc(AuraProc_Args& args) const
        {
            args.triggered_spell_id = 21064;
        }
    }ex_Twisted_Reflection;

    proc_script
    {
        void Register()
        {
            reg(28305);
        }

        void OnProc(AuraProc_Args& args) const
        {
            // Cast on owner
            args.target = args.caster->GetOwner();
            if(!args.target)
                args.result = SPELL_AURA_PROC_FAILED;
            else
                args.triggered_spell_id = 34650;
        }
    } ex_Mana_Leech;

}spell_proc_handlers;
