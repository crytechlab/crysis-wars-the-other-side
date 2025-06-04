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
	SMovementState currMovement = static_cast<CTOSAlienMovementController*>(GetMovementController())->GetCurrentMovementState();

	m_netBodyInfo.lookTarget = currMovement.eyePosition + currMovement.bodyDirection;
	m_netBodyInfo.aimTarget = currMovement.eyePosition + currMovement.aimDirection;

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

	if (aspect == tos::net::SERVER_ASPECT_STATIC)
	{
		ser.Value("health", m_health);
		ser.Value("maxHealth", m_maxHealth);
	}

	if (aspect == tos::net::CLIENT_ASPECT_DYNAMIC || aspect == tos::net::SERVER_ASPECT_DYNAMIC)
	{
		m_netBodyInfo.Serialize(GetEntity(), ser);// ок

		if (ser.IsReading())
		{
			// Скопировано из CCoopAlien::UpdateMovementState()
			CMovementRequest request;
			request.AddDeltaMovement(m_netBodyInfo.deltaMov);// ок
			request.SetBodyTarget(m_netBodyInfo.lookTarget); // не проверено вообще пришельцами не используется
			request.SetLookTarget(m_netBodyInfo.lookTarget);// ок
			request.SetAimTarget(m_netBodyInfo.aimTarget); // не проверено

			GetMovementController()->RequestMovement(request);
		}
	}

	if (aspect == tos::net::CLIENT_ASPECT_STATIC)
	{
		//Блок скопирован из CPlayer::NetSerialize()

		const bool writing = ser.IsWriting();
		bool	   hasWeapon = false;

		if (writing)
			hasWeapon = NetGetCurrentItem() != 0;

		ser.Value("hasWeapon", hasWeapon, 'bool');
		ser.Value("currentItemId", static_cast<CActor*>(this), &CActor::NetGetCurrentItem, &CActor::NetSetCurrentItem, 'eid');

		if (!writing && hasWeapon && NetGetCurrentItem() == 0)
			ser.FlagPartialRead();
	}


	return true;
}

