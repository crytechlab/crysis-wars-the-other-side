#ifndef __TOSSCOUT_H__
#define __TOSSCOUT_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Scout.h"


class CTOSScout : public CScout
{
public:
	CTOSScout();
	virtual ~CTOSScout();

	//CScout
	virtual bool Init(IGameObject* pGameObject);
	virtual void PostInit(IGameObject* pGameObject);
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot);
	virtual void PrePhysicsUpdate();
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void ProcessEvent(SEntityEvent& event);
	//~CScout
};


#endif //__TOSSCOUT_H__