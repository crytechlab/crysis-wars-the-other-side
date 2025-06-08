/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#include "StdAfx.h"

#include "TOSAlien.h"
#include "TOSAlienMovementController.h"
#include "TOSTrooper.h"

#include "TheOtherSideMP/Game/Modules/Master/MasterClient.h"
#include "TheOtherSideMP/Helpers/TOS_NET.h"
#include <Coop/Utilities/DedicatedServerHackScope.h>

CTOSAlien::CTOSAlien()
{
}

CTOSAlien::~CTOSAlien()
{
}

void CTOSAlien::PostInit(IGameObject* pGameObject)
{
	CAlien::PostInit(pGameObject);

	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));
	if (pRenderProxy)
	{
		pRenderProxy->UpdateCharactersBeforePhysics(false);
	}
}

void CTOSAlien::PostPhysicalize()
{	
	CAlien::PostPhysicalize();

	IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
	if (!pScriptTable)
		return;

	SmartScriptTable physicsParams;
	if (pScriptTable->GetValue("physicsParams", physicsParams))
	{
		SmartScriptTable livingTab;
		if (physicsParams->GetValue("Living", livingTab))
		{
			livingTab->GetValue("inertia",m_params.inertia);
			livingTab->GetValue("inertiaAccel",m_params.inertiaAccel);
		}
	}
}

void CTOSAlien::Update(SEntityUpdateContext& ctx, const int updateSlot)
{
	CAlien::Update(ctx, updateSlot);

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

// ReSharper disable once CppParameterMayBeConst
bool CTOSAlien::NetSerialize(TSerialize ser, const EEntityAspects aspect, const uint8 profile, const int flags)
{
	if (!CAlien::NetSerialize(ser,aspect,profile,flags))
		return false;

	if (aspect == tos::net::SERVER_ASPECT_STATIC)
	{
		ser.Value("health", m_health);
		ser.Value("maxHealth", m_maxHealth);
	}

	if (aspect == tos::net::CLIENT_ASPECT_DYNAMIC || aspect == tos::net::SERVER_ASPECT_DYNAMIC)
	{
		m_netBodyInfo.Serialize(GetEntity(), ser);

		if (ser.IsReading())
		{
			CMovementRequest request;
		
			request.SetMoveTarget(GetEntity()->GetPos() + m_netBodyInfo.moveTarget);
			request.SetLookTarget(m_netBodyInfo.lookTarget);
			request.SetBodyTarget(m_netBodyInfo.bodyTarget);
			request.SetFireTarget(m_netBodyInfo.fireTarget);
			request.AddDeltaMovement(m_netBodyInfo.deltaMov);

			request.SetDesiredSpeed(m_netBodyInfo.desiredSpeed);
			m_stats.speed = m_netBodyInfo.desiredSpeed;
			m_stats.fireDir = Vec3(ZERO);

			request.SetStance(static_cast<EStance>(m_netBodyInfo.stance));
			
			if (m_netBodyInfo.hasAimTarget)
				request.SetAimTarget(m_netBodyInfo.aimTarget);
			else
				request.ClearAimTarget();

			GetMovementController()->RequestMovement(request);

			// Update view matrices
			Vec3 viewDir = (m_netBodyInfo.lookTarget - GetEntity()->GetWorldPos()).GetNormalized();
			Vec3 bodyDir = (m_netBodyInfo.bodyTarget - GetEntity()->GetWorldPos()).GetNormalized();
			Vec3 aimDir = (m_netBodyInfo.aimTarget - GetEntity()->GetWorldPos()).GetNormalized();

			if (viewDir.len2() > 0.001f)
				m_viewMtx.SetRotationVDir(viewDir);
			if (bodyDir.len2() > 0.001f)
				m_baseMtx.SetRotationVDir(bodyDir);
			if (aimDir.len2() > 0.001f)
				m_eyeMtx.SetRotationVDir(aimDir);
		}
	}

	if (aspect == tos::net::CLIENT_ASPECT_STATIC)
	{
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

void CTOSAlien::ProcessEvent(SEntityEvent& event)
{
	CAlien::ProcessEvent(event);
}

void CTOSAlien::PrePhysicsUpdate()
{
	CAlien::PrePhysicsUpdate();

	const SMovementState currentState = static_cast<CTOSAlienMovementController*>(GetMovementController())->GetCurrentMovementState();

	m_netBodyInfo.moveTarget = GetEntity()->GetWorldPos() + currentState.movementDirection;
	// m_netBodyInfo.aimTarget = currentState.eyePosition + currentState.aimDirection;
	// m_netBodyInfo.lookTarget = currentState.eyePosition + currentState.eyeDirection;
	// m_netBodyInfo.bodyTarget = currentState.eyePosition + currentState.bodyDirection;
	m_netBodyInfo.fireTarget = currentState.fireTarget;
	m_netBodyInfo.deltaMov = m_input.deltaMovement;

	// Float
	m_netBodyInfo.desiredSpeed = m_moveRequest.velocity.GetLength();

	// Int
	m_netBodyInfo.stance = static_cast<int>(currentState.stance);

	// Bool
	m_netBodyInfo.hasAimTarget = currentState.isAiming;

	// View direction sync
	m_netBodyInfo.lookTarget = GetEntity()->GetWorldPos() + m_viewMtx.GetColumn(1) * 10.0f;
	m_netBodyInfo.bodyTarget = GetEntity()->GetWorldPos() + m_baseMtx.GetColumn(1) * 10.0f;
	m_netBodyInfo.aimTarget = GetEntity()->GetWorldPos() + m_eyeMtx.GetColumn(1) * 10.0f;

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

void CTOSAlien::SetHealth(const int health)
{
	CAlien::SetHealth(health);

	if (gEnv->bServer)
	{
		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
	}
}

Matrix33 &CTOSAlien::GetViewMtx()
{
	return m_viewMtx;
}

Matrix33 &CTOSAlien::GetBaseMtx()
{
	return m_baseMtx;
}
Matrix33 &CTOSAlien::GetEyeMtx()
{
	return m_eyeMtx;
}

void CTOSAlien::Kill()
{
	CAlien::Kill();
}

void CTOSAlien::Revive(bool fromInit)
{
	CAlien::Revive(fromInit);
}

bool CTOSAlien::ApplyActions(int actions)
{
	m_input.actions = actions;
	return true;
}

void CTOSAlien::SetParams(SmartScriptTable& rTable, bool resetFirst)
{
	CAlien::SetParams(rTable, resetFirst);
}

void CTOSAlien::ApplyMasterMovement(const Vec3& delta)
{
	//m_input.deltaMovement = FilterDeltaMovement(delta);

	m_input.deltaMovement.x = clamp_tpl(m_input.deltaMovement.x + delta.x, -1.0f, 1.0f);
	m_input.deltaMovement.y = clamp_tpl(m_input.deltaMovement.y + delta.y, -1.0f, 1.0f);
	m_input.deltaMovement.z = clamp_tpl(m_input.deltaMovement.z + delta.z, -1.0f, 1.0f);

	m_input.deltaMovement.x = (delta.x < 0.0f || delta.x > 0.0f) ? m_input.deltaMovement.x : 0;
	m_input.deltaMovement.y = (delta.y < 0.0f || delta.y > 0.0f) ? m_input.deltaMovement.y : 0;
	m_input.deltaMovement.z = (delta.z < 0.0f || delta.z > 0.0f) ? m_input.deltaMovement.z : 0;
}

IActorMovementController* CTOSAlien::CreateMovementController()
{
	return new CTOSAlienMovementController(this);
}

void CTOSAlien::SendSpecialMovementRequest(uint32 reqFlags, const SActorTargetParams& targetParams)
{
	if (targetParams.animation.c_str() && targetParams.animation.c_str()[0] != 0)
		CryLog("[%s] Sending actor target with %s animation %s.", GetEntity()->GetName(), targetParams.signalAnimation ? "signal" : "action", targetParams.animation.c_str());
	else
		CryLog("[%s] Sending actor target removal.", GetEntity()->GetName());
	
	GetGameObject()->InvokeRMI(ClSpecialMovementRequest(), SSpecialMovementRequestParams(reqFlags, targetParams, targetParams.animation), eRMI_ToAllClients | eRMI_NoLocalCalls);
}

IMPLEMENT_RMI(CTOSAlien, ClSpecialMovementRequest)
{
	if (params.targetParams.animation.c_str() != nullptr && params.targetParams.animation.c_str()[0] != 0)
		CryLog("[%s] Received actor target with %s animation %s.", GetEntity()->GetName(), params.targetParams.signalAnimation ? "signal" : "action", params.targetParams.animation.c_str());
	else
		CryLog("[%s] Received actor target removal.", GetEntity()->GetName());

	if (!gEnv->bServer)
	{
		CMovementRequest movRequest = CMovementRequest();
		if ((params.flags & CMovementRequest::eMRF_ActorTarget) != 0)
		{
			movRequest.SetActorTarget(params.targetParams);
		}
		else if ((params.flags & CMovementRequest::eMRF_RemoveActorTarget) != 0)
		{
			movRequest.ClearActorTarget();
		}


		this->GetMovementController()->RequestMovement(movRequest);
	}
	

	return true;
}
