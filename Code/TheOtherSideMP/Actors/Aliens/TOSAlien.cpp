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
}

// ReSharper disable once CppParameterMayBeConst
bool CTOSAlien::NetSerialize(TSerialize ser, const EEntityAspects aspect, const uint8 profile, const int flags)
{
	if (!CAlien::NetSerialize(ser,aspect,profile,flags))
		return false;

	if (aspect == TOS_NET::SERVER_ASPECT_STATIC)
	{
		ser.Value("health", m_health);
		ser.Value("maxHealth", m_maxHealth);
	}

	if (aspect == TOS_NET::CLIENT_ASPECT_DYNAMIC || aspect == TOS_NET::SERVER_ASPECT_DYNAMIC)
	{
		m_netBodyInfo.Serialize(GetEntity(), ser);// ок

		if (ser.IsReading())
		{

			// Скопировано из CCoopAlien::UpdateMovementState()
			CMovementRequest request;

			const auto pTrooper = static_cast<CTOSTrooper*>(this);
			if (pTrooper)
			{
				request.AddDeltaMovement(m_netBodyInfo.deltaMov);// ок
				//request.SetBodyTarget(m_netBodyInfo.lookTarget); // вообще пришельцами не используется
				request.SetLookTarget(m_netBodyInfo.lookTarget);// ок
				//request.SetAimTarget(m_netBodyInfo.aimTarget);
				//request.SetFireTarget(m_netBodyInfo.fireTarget);// не нужен вроде

				//Заставляет тушу двигаться самостоятельно
				//request.SetMoveTarget(m_netBodyInfo.moveTarget);

				request.SetStance(static_cast<EStance>(m_netBodyInfo.stance));// не проверено
			}
			else
			{
				request.SetMoveTarget(GetEntity()->GetPos() + m_netBodyInfo.moveTarget); // не проверено
				request.SetLookTarget(m_netBodyInfo.lookTarget);// не проверено
				request.SetBodyTarget(GetEntity()->GetWorldRotation() * Vec3(0, 1, 0));// не проверено
				request.SetFireTarget(m_netBodyInfo.fireTarget);// не проверено

				request.SetDesiredSpeed(m_netBodyInfo.desiredSpeed);// не проверено
				m_stats.speed = m_netBodyInfo.desiredSpeed;// не проверено
				m_stats.fireDir = Vec3(ZERO);// не проверено

				request.SetStance(static_cast<EStance>(m_netBodyInfo.stance));// не проверено
			}

			GetMovementController()->RequestMovement(request);
		}

		//if (m_stats.inAir > 0.01)
		//{
			//float z = m_netBodyInfo.worldPos.z;
			//Vec3 orig_pos = GetEntity()->GetWorldPos();
			//orig_pos.z = z;
			//GetEntity()->SetWorldTM(Matrix34::CreateTranslationMat(orig_pos));
		//}

		/*
		if (ser.IsWriting())
		{
			CryLogAlways("[%s] WRITE INPUT:", GetEntity()->GetName());
			CryLogAlways("	m_netBodyInfo.deltaMov = (%1.f, %1.f, %1.f)", m_netBodyInfo.deltaMov.x, m_netBodyInfo.deltaMov.y, m_netBodyInfo.deltaMov.z);
			CryLogAlways("	m_netBodyInfo.lookTarget = (%1.f, %1.f, %1.f)", m_netBodyInfo.lookTarget.x, m_netBodyInfo.lookTarget.y, m_netBodyInfo.lookTarget.z);
		}
		else
		{
			CryLogAlways("[%s] READ INPUT:", GetEntity()->GetName());
			CryLogAlways("	m_netBodyInfo.deltaMov = (%1.f, %1.f, %1.f)", m_netBodyInfo.deltaMov.x, m_netBodyInfo.deltaMov.y, m_netBodyInfo.deltaMov.z);
			CryLogAlways("	m_netBodyInfo.lookTarget = (%1.f, %1.f, %1.f)", m_netBodyInfo.lookTarget.x, m_netBodyInfo.lookTarget.y, m_netBodyInfo.lookTarget.z);
		}
		*/
	}

	if (aspect == TOS_NET::CLIENT_ASPECT_STATIC)
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

		/*
		if (writing)
		{
			CryLogAlways("[%s] WRITE INPUT:", GetEntity()->GetName());
			CryLogAlways("	hasWeapon = %i", hasWeapon);
		}
		else
		{
			CryLogAlways("[%s] READ INPUT:", GetEntity()->GetName());
			CryLogAlways("	hasWeapon = %i", hasWeapon);
		}
		*/
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
	// если раскомментировать, то сервер будет только считывать
	// если оставить как есть, то сервер будет и считывать и записывать (отрицательно не влияет на геймплей)
	//if (!gEnv->bClient)
	//	return;

	const SMovementState currentState = static_cast<CCompatibilityAlienMovementController*>(GetMovementController())->GetCurrentMovementState();

	const auto pTrooper = static_cast<CTOSTrooper*>(this);
	if (pTrooper)
	{
		// Взято наглядно из CAlien::GetActorInfo()
		// 10/18/2023, 18:36 мне удалось достичь очень плавного перемещения трупера у других клиентов.
		// Практически неотличимо от локального управления.
		// Единственное что меня волнует это прыжок.
		// Чуть дольше чем момент начала прыжка трупер у других клиентов немного дёргается.
		const Vec3 eyePos = GetEntity()->GetSlotWorldTM(0) * m_eyeOffset; // ок
		const Vec3 weaponPos = GetEntity()->GetSlotWorldTM(0) * m_weaponOffset; // ок

		// Принцип работы: m_viewMtx.GetColumn1() на владеющем клиенте ->
		// serialize направление вперед(forward) (от владеющего к другим клиентам и серверу) ->
		// m_viewMtx.SetRotationVDir()
		const Vec3 eyeDir = m_viewMtx.GetColumn1(); // ок

		m_netBodyInfo.lookTarget = eyePos + eyeDir * 10.0f;
		m_netBodyInfo.fireTarget = weaponPos + eyeDir * 10.0f;

		m_netBodyInfo.deltaMov = m_input.deltaMovement;

		//m_netBodyInfo.aimTarget = currentState.eyePosition + currentState.eyeDirection; //ура я нашёл //currentState.aimDirection;
		//m_netBodyInfo.lookTarget = currentState.eyePosition + currentState.eyeDirection * 10.0f;

		/* не работает
		m_netBodyInfo.lookTarget = currentState.eyePosition + currentState.bodyDirection;
		m_netBodyInfo.aimTarget = currentState.eyePosition + currentState.aimDirection;
		m_netBodyInfo.fireTarget = currentState.eyePosition + currentState.weaponPosition;
		m_netBodyInfo.bodyTarget = currentState.eyePosition + currentState.bodyDirection;
		*/

		/* не работает
		m_netBodyInfo.moveTarget = GetEntity()->GetWorldPos() + currentState.movementDirection;
		m_netBodyInfo.aimTarget = currentState.eyePosition + currentState.aimDirection;
		m_netBodyInfo.lookTarget = currentState.eyePosition + currentState.bodyDirection;
		*/

		//m_netBodyInfo.fireTarget = currentState.fireTarget;

		//m_netBodyInfo.desiredSpeed = m_moveRequest.velocity.GetLength();

		m_netBodyInfo.stance = static_cast<int>(currentState.stance); // не проверено

		// Bool
		//m_netBodyInfo.hasAimTarget = currentState.isAiming;

		//IPersistantDebug* pPD = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
		//const Vec3 wp(GetEntity()->GetWorldPos() + Vec3(0, 0, 1));

		//pPD->Begin(string("master_input_pre_physics_") + GetEntity()->GetName(), true);
		//pPD->AddSphere(m_netBodyInfo.lookTarget, 0.5f, ColorF(1, 0, 1, 1), 1.0f);
		//pPD->AddSphere(m_netBodyInfo.aimTarget, 0.5f, ColorF(1, 1, 1, 1), 1.0f);
		//pPD->AddSphere(m_netBodyInfo.fireTarget, 0.5f, ColorF(1, 0, 0, 1), 1.0f);

		//pPD->AddDirection(wp, 1.5f, m_input.deltaMovement, ColorF(1, 0, 0, 1), 1.0f);
		//pPD->AddDirection(wp, 1.5f, currentState.aimDirection, ColorF(0, 1, 0, 1), 1.0f);
	}
	else
	{
		//Vec3
		m_netBodyInfo.moveTarget = GetEntity()->GetWorldPos() + currentState.movementDirection; // не проверено
		m_netBodyInfo.aimTarget = currentState.eyePosition + currentState.aimDirection; // не проверено
		m_netBodyInfo.lookTarget = currentState.eyePosition + currentState.eyeDirection; // не проверено
		m_netBodyInfo.fireTarget = currentState.fireTarget; // не проверено

		// Float
		m_netBodyInfo.desiredSpeed = m_moveRequest.velocity.GetLength(); // не проверено

		// Int
		m_netBodyInfo.stance = static_cast<int>(currentState.stance); // не проверено

		// Bool
		//m_netBodyInfo.hasAimTarget = currentState.isAiming;
	}

	if (gEnv->bClient)
		m_netBodyInfo.worldPos = GetEntity()->GetWorldPos();

	//TheOtherSide
	//if (gEnv->bClient)
	//{
	GetGameObject()->ChangedNetworkState(TOS_NET::CLIENT_ASPECT_DYNAMIC);
	//}
	//else
	//{
	//	GetGameObject()->ChangedNetworkState(TOS_NET::SERVER_ASPECT_AI_INPUT);
	//}
	//~TheOtherSide

	//ICharacterInstance* pCharacter = GetEntity() ? GetEntity()->GetCharacter(0) : NULL;
	//if (pCharacter)
	//{
	//	if (IsLocalSlave() || (IsSlave() && gEnv->bServer))
	//		pCharacter->GetISkeletonPose()->SetForceSkeletonUpdate(4);
	//}
	
	//float frameTime = gEnv->pTimer->GetFrameTime();

	//if (m_pMovementController)
	//	m_pMovementController->PostUpdate(frameTime);
}

void CTOSAlien::SetHealth(const int health)
{
	CAlien::SetHealth(health);

	if (gEnv->bServer)
	{
		GetGameObject()->ChangedNetworkState(TOS_NET::SERVER_ASPECT_STATIC);
	}
}

Matrix33 CTOSAlien::GetViewMtx()
{
	return m_viewMtx;
}

Matrix33 CTOSAlien::GetBaseMtx()
{
	return m_baseMtx;
}
Matrix33 CTOSAlien::GetEyeMtx()
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
