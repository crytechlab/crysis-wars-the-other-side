#include "StdAfx.h"
#include "TOSHunter.h"
#include "CompatibilityAlienMovementController.h"

//Created by Crysis co-op
//Adapted to TOS by AlienKeeper

//Crysis co-op
#include "Coop\CoopSystem.h"
#include <Coop\Utilities\DedicatedServerHackScope.h>
#include <TheOtherSideMP/Helpers/TOS_NET.h>
#include "TOSAlienMovementController.h"
//Crysis co-op

CTOSHunter::CTOSHunter()
{
}

CTOSHunter::~CTOSHunter()
{
}

bool CTOSHunter::Init(IGameObject* pGameObject)
{
	CHunter::Init(pGameObject);

	return true;
}

void CTOSHunter::PostInit(IGameObject* pGameObject)
{
	CHunter::PostInit(pGameObject);

	pGameObject->SetAIActivation(eGOAIAM_Always);
}

void CTOSHunter::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));
	if ((pRenderProxy == nullptr) || !pRenderProxy->IsCharactersUpdatedBeforePhysics())
		PrePhysicsUpdate();

	CHunter::Update(ctx, updateSlot);

	EAutoDisablePhysicsMode adpm = eADPM_WhenAIDeactivated; // Значение по умолчанию

	if (m_stats.isRagDoll || IsLocalSlave() || (gEnv->bMultiplayer && gEnv->bServer))
	{
		adpm = eADPM_Never;
	}
	else if (IsPlayer() || IsSlave())
	{
		adpm = eADPM_WhenInvisibleAndFarAway;
	}

	GetGameObject()->SetAutoDisablePhysicsMode(adpm);

	if (IAnimationGraphState* pGraphState = this->GetAnimationGraphState())
	{
		// Only update on dedicated server.
		if (gEnv->bServer && !gEnv->bClient)
		{
			CDedicatedServerHackScope::Enter();
			pGraphState->Update();
			CDedicatedServerHackScope::Exit();
		}
	}
}

void CTOSHunter::ProcessEvent(SEntityEvent& event)
{
	CHunter::ProcessEvent(event);
}

void CTOSHunter::PrePhysicsUpdate()
{
	CHunter::PrePhysicsUpdate();

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_DYNAMIC);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_DYNAMIC);
	}
}

bool CTOSHunter::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (!CHunter::NetSerialize(ser, aspect, profile, flags))
		return false;
	return true;
}

