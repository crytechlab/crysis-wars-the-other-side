#include "StdAfx.h"
#include "TOSScout.h"

#include "Coop\CoopSystem.h"
#include <Coop\Utilities\DedicatedServerHackScope.h>
#include "TheOtherSideMP/Actors/Aliens/TOSAlienMovementController.h"
#include <TheOtherSideMP/Helpers/TOS_NET.h>
#include <NetInputChainDebug.h>


CTOSScout::CTOSScout()
{
}

CTOSScout::~CTOSScout()
{
}

bool CTOSScout::Init(IGameObject* pGameObject)
{
	CScout::Init(pGameObject);

	return true;
}

void CTOSScout::PostInit(IGameObject* pGameObject)
{
	CScout::PostInit(pGameObject);
	pGameObject->SetAIActivation(eGOAIAM_Always);
}

void CTOSScout::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));
	if ((pRenderProxy == nullptr) || !pRenderProxy->IsCharactersUpdatedBeforePhysics())
		PrePhysicsUpdate();

	CScout::Update(ctx, updateSlot);

	NETINPUT_TRACE(GetEntityId(), m_input.deltaMovement);
	NETINPUT_TRACE(GetEntityId(), m_input.viewDir);
	NETINPUT_TRACE(GetEntityId(), m_input.actions);
	NETINPUT_TRACE(GetEntityId(), m_input.viewVector);
	NETINPUT_TRACE(GetEntityId(), m_netBodyInfo.desiredSpeed);
	NETINPUT_TRACE(GetEntityId(), m_netBodyInfo.deltaMov);
	NETINPUT_TRACE(GetEntityId(), m_netBodyInfo.lookTarget);
	NETINPUT_TRACE(GetEntityId(), GetEntity()->GetWorldPos());
	NETINPUT_TRACE(GetEntityId(), m_stats.inAir);
	NETINPUT_TRACE(GetEntityId(), m_stats.onGround);
	NETINPUT_TRACE(GetEntityId(), InZeroG());
	NETINPUT_TRACE(GetEntityId(), IsSlave());

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

void CTOSScout::PrePhysicsUpdate()
{
	CTOSAlien::PrePhysicsUpdate();

	const SMovementState currentState = static_cast<CTOSAlienMovementController*>(GetMovementController())->GetCurrentMovementState();

	//m_netBodyInfo.moveTarget = GetEntity()->GetWorldPos() + currentState.movementDirection; // не проверено
	//m_netBodyInfo.fireTarget = currentState.fireTarget; // не проверено

	m_netBodyInfo.aimTarget = currentState.eyePosition + currentState.bodyDirection;
	m_netBodyInfo.lookTarget = currentState.eyePosition + currentState.aimDirection;

	// Float
	//m_netBodyInfo.desiredSpeed = m_moveRequest.velocity.GetLength(); // не проверено

	// Int
	//m_netBodyInfo.stance = static_cast<int>(currentState.stance); // не проверено

	// Bool
	//m_netBodyInfo.hasAimTarget = currentState.isAiming;

	if (gEnv->bClient)
	{
		m_netBodyInfo.worldPos = GetEntity()->GetWorldPos();
		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_DYNAMIC);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_DYNAMIC);
	}
}

//void CTOSScout::UpdateMovementState()
//{
//	CMovementRequest request;
//	request.SetBodyTarget(m_vLookTarget);
//	request.SetLookTarget(m_vLookTarget);
//	request.SetAimTarget(m_vAimTarget);
//
//
//	SetActorMovement(SMovementRequestParams(request));
//}

void CTOSScout::ProcessEvent(SEntityEvent& event)
{
	CScout::ProcessEvent(event);
}

bool CTOSScout::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (!CTOSAlien::NetSerialize(ser, aspect, profile, flags))
		return false;

	return true;
}

