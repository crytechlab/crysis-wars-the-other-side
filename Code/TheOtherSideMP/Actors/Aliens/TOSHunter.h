#ifndef __TOSHUNTER_H__
#define __TOSHUNTER_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Hunter.h"


class CTOSHunter : public CHunter
{
public:
	CTOSHunter();
	virtual ~CTOSHunter();

	//CHunter
	virtual bool Init(IGameObject* pGameObject);
	virtual void PostInit(IGameObject* pGameObject);
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot);
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void ProcessEvent(SEntityEvent& event);
	virtual void PrePhysicsUpdate();
	//~CHunter

private:
};


#endif //__TOSHUNTER_H__