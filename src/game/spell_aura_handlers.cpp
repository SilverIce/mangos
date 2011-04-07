#include "spell_script_includes.h"
#include "GameObject.h"

struct AuraHandlers
{
    struct AH_Impaling_Spine : AuraHandler2
    {
        USE_IN(SPELL_AURA_MOD_STUN,EFFECT_INDEX_2,39837);

        void OnApply(struct OnApplyArgs& e, bool apply) const
        {
            if (!apply)
                return;

            Unit * target = e.target;
            GameObject* pObj = new GameObject;
            if(pObj->Create(target->GetMap()->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), 185584, target->GetMap(), target->GetPhaseMask(),
                target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
            {
                pObj->SetRespawnTime(e.aura.GetAuraDuration()/IN_MILLISECONDS);
                pObj->SetSpellId(e.aura.GetId());
                target->AddGameObject(pObj);
                target->GetMap()->Add(pObj);
            }
            else
                delete pObj;
        }
    } sc_field;

};

void Init_AuraHandlers()
{
    new AuraHandlers();
};
