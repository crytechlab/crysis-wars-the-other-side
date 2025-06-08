#ifndef __TOSSCOUT_H__
#define __TOSSCOUT_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Scout.h"


class CTOSScout : public CScout
{
public:
	//CScout
	bool Init(IGameObject* pGameObject) override;
	void PostInit(IGameObject* pGameObject) override;
	void Update(SEntityUpdateContext& ctx, int updateSlot) override;
    void SelectItem(const EntityId itemId, const bool keepHistory) override;
    void PrePhysicsUpdate() override;
    bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) override;
	void ProcessEvent(SEntityEvent& event) override;
	//~CScout
};


#endif //__TOSSCOUT_H__