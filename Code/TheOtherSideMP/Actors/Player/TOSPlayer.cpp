/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#include "StdAfx.h"
#include "TOSPlayer.h"

#include "../../Game/TOSGame.h"
#include "../../Game/Modules/Master/MasterClient.h"
#include "../../Game/Modules/Master/MasterSynchronizer.h"

#include "HUD/HUD.h"

#include "TheOtherSideMP/Extensions/EnergyСonsumer.h"
#include "TheOtherSideMP/Game/TOSGameEventRecorder.h"
#include <Claymore.h>

#include "NetInputChainDebug.h"
#include <Coop/Utilities/DedicatedServerHackScope.h>

CTOSPlayer::CTOSPlayer()
	: m_pMasterClient(nullptr) ,
	m_fDetectionTimer(0),
	m_fDetectionValue(0),
	m_fLastDetectionValue(0),
	m_fNetDetectionDelay(0.f),
	m_fMusicIntensity(0.f),
	m_bMusicForceMood(false),
	m_fMusicDelay(0.f)
{
	
}

CTOSPlayer::~CTOSPlayer()
{
	if (m_pEnergyConsumer)
	{
		GetGameObject()->ReleaseExtension("CTOSEnergyConsumer");
		m_pEnergyConsumer = nullptr;
	}
	m_pSystemUpdateRate = 0;
}

bool CTOSPlayer::Init(IGameObject* pGameObject)
{
	if (!CPlayer::Init(pGameObject))
		return false;

	//Crysis co-op
	m_pSystemUpdateRate = gEnv->pConsole->GetCVar("ai_UpdateInterval");
	//~Crysis co-op

	return true;
}

void CTOSPlayer::PostInit(IGameObject* pGameObject)
{
	CPlayer::PostInit(pGameObject);

	//Case 1 - Master Client was created only on local machine
	// Not working at dedicated server
	//if (GetEntityId() == g_pGame->GetIGameFramework()->GetClientActorId())
	//{
	//	m_pLocalMasterClient = new CTOSMasterClient(this);
	//}

	//Case 2 - Master Client was created on client
	// It is ok on dedicated but calling two cases on not dedicated
	//if (!m_pMasterClient)
	//{
	//	m_pMasterClient = new CTOSMasterClient(this);
	//}

	//if (IsClient())
	//{
	//	gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("AlienInterference_Amount", 0.0f);
	//	SAFE_HUD_FUNC(StartInterference(0, 0, 0, 0));
	//}
}

void CTOSPlayer::InitClient(const int channelId)
{
	CPlayer::InitClient(channelId);

	//if (gEnv->bServer)
	//{
	//	CryLogAlways(" ");
	//	CryLogAlways("<C++>[SERVER][FUNC CALL][CPlayer::InitClient] channelId: %i, ThisPlayer: %s", channelId, GetEntity()->GetName());
	//}
	//else if(gEnv->bClient)
	//{
	//	CryLogAlways(" ");
	//	CryLogAlways("<C++>[CLIENT][FUNC CALL][CPlayer::InitClient] channelId: %i, ThisPlayer: %s", channelId, GetEntity()->GetName());
	//}

	//Case 3
	//if (!m_pMasterClient)
	//{
	//	m_pMasterClient = new CTOSMasterClient(this);
	//}
}

void CTOSPlayer::InitLocalPlayer()
{
	CPlayer::InitLocalPlayer();

	CryLog("[%s] Init local player", GetEntity()->GetName());

	//Case 4 perfect - Master Client was created only on local machine
	if (!m_pMasterClient)
	{
		m_pMasterClient = new CTOSMasterClient(this);
		g_pTOSGame->GetMasterModule()->RegisterMasterClient(m_pMasterClient);
	}

	// Исправление бага https://github.com/akeeperctl/crysis-wars-the-other-side/issues/5
	ClearInterference();
}

void CTOSPlayer::SetSpectatorMode(uint8 mode, EntityId targetId)
{
	const int oldMode = GetSpectatorMode();

	switch (mode)
	{
	case eASM_None:
	{
		if (oldMode > eASM_None)
		{
			//TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_PlayerJoinedGame, "", true));
		}

		break;
	}
	case eASM_Fixed:
	case eASM_Free:
	case eASM_Follow:
	{
		if (oldMode == eASM_None)
		{
			TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_PlayerJoinedSpectator, "", true, false, nullptr, 0.0f, mode));
		}
		break;
	}
	case eASM_Cutscene:
	{
		if (oldMode == eASM_None)
		{
			TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_PlayerJoinedCutscene, "", true, false, nullptr, 0.0f, mode));
		}
		break;
	}
	default:
		break;
	}

	CPlayer::SetSpectatorMode(mode, targetId);
}

void CTOSPlayer::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	CPlayer::Update(ctx,updateSlot);

	NETINPUT_TRACE(GetEntityId(), m_stats.velocity);
	NETINPUT_TRACE(GetEntityId(), m_stats.speed);
	NETINPUT_TRACE(GetEntityId(), GetEntity()->GetWorldPos());

	//Crysis co-op
	UpdateMusic(ctx.fFrameTime);

	if (gEnv->bServer)
	{
		if (m_fNetDetectionDelay > 0.1f)
		{
			m_fNetDetectionDelay = 0.f;
			GetGameObject()->InvokeRMI(
				CTOSPlayer::ClUpdateAwareness(),
				SAwarenessParams(m_fDetectionValue),
				eRMI_ToClientChannel | eRMI_NoLocalCalls,
				GetChannelId());
		}
		else
			m_fNetDetectionDelay += ctx.fFrameTime;
	}


	if (IsPlayer() && gEnv->bServer)
	{
		/*if (!GetEntity()->GetAI() && GetSpectatorMode() == eASM_None)
		{
			gEnv->bMultiplayer = false;

			IScriptTable* pScriptTable = GetEntity()->GetScriptTable();

			gEnv->pScriptSystem->BeginCall(pScriptTable, "CoopForceAI");
			gEnv->pScriptSystem->PushFuncParam(pScriptTable);
			gEnv->pScriptSystem->EndCall(pScriptTable);

			if (CCoopSystem::GetInstance()->GetDebugLog() > 0)
				CryLogAlways("AI Registered for Player %s", GetEntity()->GetName());

			gEnv->bMultiplayer = true;
		}*/
	}

	// Fixes cloaking in MP for non-host players
	/*CNanoSuit* pNanoSuit = GetNanoSuit();
	if (pNanoSuit && gEnv->bServer)
	{
		IAIObject* pAI = GetEntity()->GetAI();
		if (pAI && pAI->CastToIAIActor() &&
			GetEntityId() != g_pGame->GetIGameFramework()->GetClientActorId())
		{
			AgentParameters& agentParams = (AgentParameters&)pAI->CastToIAIActor()->GetParameters();
			if (pNanoSuit->GetMode() == NANOMODE_CLOAK )
				agentParams.m_fCloakScale = 1.f;
			else
				agentParams.m_fCloakScale = 0.f;
		}
	}*/

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

	//Crysis co-op
}

// ReSharper disable once CppParameterMayBeConst
bool CTOSPlayer::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (!CPlayer::NetSerialize(ser,aspect,profile,flags))
		return false;

	return true;
}

void CTOSPlayer::Release()
{
	SAFE_DELETE(m_pMasterClient);

	// Если локальный игрок, то снимает удаляем мастер-клиент на локальной машине 
	if (IsClient())
	{
		g_pTOSGame->GetMasterModule()->UnregisterMasterClient();
	}

	CPlayer::Release();
}

void CTOSPlayer::UpdateView(SViewParams& viewParams)
{
	if (m_pMasterClient && m_pMasterClient->GetSlaveEntity())
	{
		m_pMasterClient->UpdateView(viewParams);
	}
	else
	{
		CPlayer::UpdateView(viewParams);
	}
}

void CTOSPlayer::PostUpdateView(SViewParams& viewParams)
{
	CPlayer::PostUpdateView(viewParams);
}

void CTOSPlayer::Kill()
{
	if (CNanoSuit* pSuit = GetNanoSuit())
		pSuit->Death();

	// notify any claymores/mines that this player has died
	//	(they will be removed 30s later)
	RemoveAllExplosives(EXPLOSIVE_REMOVAL_TIME);

	CTOSActor::Kill();
}

IEntity* CTOSPlayer::LinkToVehicle(EntityId vehicleId)
{
	return CPlayer::LinkToVehicle(vehicleId);
}

IEntity* CTOSPlayer::LinkToEntity(EntityId entityId, bool bKeepTransformOnDetach)
{
	return CPlayer::LinkToEntity(entityId, bKeepTransformOnDetach);
}

void CTOSPlayer::LinkToMountedWeapon(EntityId weaponId)
{
	CPlayer::LinkToMountedWeapon(weaponId);
}

Matrix33 CTOSPlayer::GetViewMtx()
{
	//TODO: 10/05/2023, 15:56 проверить правильность конвертации
	const auto mat33 = static_cast<Matrix33>(m_viewQuatFinal);
	assert(mat33.IsValid());

	return mat33;
}

Matrix33 CTOSPlayer::GetBaseMtx()
{
	//TODO: 10/05/2023, 15:56 проверить правильность конвертации
	const auto mat33 = static_cast<Matrix33>(m_baseQuat);
	assert(mat33.IsValid());

	return mat33;
}
Matrix33 CTOSPlayer::GetEyeMtx()
{
	//TODO: 10/05/2023, 15:56 проверить правильность конвертации
	const auto mat33 = static_cast<Matrix33>(this->m_viewQuatFinal);
	assert(mat33.IsValid());

	return mat33;

}

bool CTOSPlayer::ApplyActions(int actions)
{
	m_actions = actions;
	return true;
}

CTOSMasterClient* CTOSPlayer::GetMasterClient() const
{
	assert(m_pMasterClient);
	return m_pMasterClient;
}

void CTOSPlayer::ClearInterference()
{
	m_clientPostEffects.clear();
	gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("AlienInterference_Amount", 0.0f);
	SAFE_HUD_FUNC(StartInterference(0, 0, 0, 0));
}


//Crysis co-op
void CTOSPlayer::UpdateDetectionValue(float frameTime)
{
	if (!GetEntity()->GetAI())
	{
		m_fDetectionValue = 0.0f;
		m_fLastDetectionValue = 0.0f;
		return;
	}

	// Force players to group 0.
	if (this->GetEntity()->GetAI()->GetGroupId() != 0)
		this->GetEntity()->GetAI()->SetGroupId(0);

	SAIDetectionLevels sDetectionLevelSnapshot;
	gEnv->pAISystem->GetDetectionLevels(
		GetEntity()->GetAI(),
		sDetectionLevelSnapshot);

	m_fDetectionValue = max(
		max(sDetectionLevelSnapshot.puppetExposure,
			sDetectionLevelSnapshot.puppetThreat),
		max(sDetectionLevelSnapshot.vehicleExposure,
			sDetectionLevelSnapshot.vehicleThreat));

	m_fLastDetectionValue = m_fDetectionValue;
	// Local player can use AI system's default method.
	/*if (GetEntityId() == g_pGame->GetIGameFramework()->GetClientActorId())
	{


		SAIDetectionLevels aiDetectionLevels;
		gEnv->pAISystem->GetDetectionLevels(0, aiDetectionLevels);
		m_fDetectionValue = max(max(aiDetectionLevels.puppetExposure, aiDetectionLevels.puppetThreat),
								max(aiDetectionLevels.vehicleExposure, aiDetectionLevels.vehicleThreat));
		return;
	}

	m_fDetectionTimer += frameTime;

	// Only detect with AI system update intervals.
	if (m_fDetectionTimer >= m_pSystemUpdateRate->GetFVal() + 0.05f)
	{
		// No AI for player nothing to be detected
		if (!GetEntity()->GetAI())
			return;

		m_fDetectionTimer = 0.0f;

		float* pAIActorFloat = (float*)GetEntity()->GetAI()->CastToIAIActor();

		// Varies between X86 and X64
		int nDataIndex = (sizeof(void*) == 8) ? 502 : 473;

		// Create snapshot
		pAIActorFloat[nDataIndex + 4] = pAIActorFloat[nDataIndex + 0];
		pAIActorFloat[nDataIndex + 5] = pAIActorFloat[nDataIndex + 1];
		pAIActorFloat[nDataIndex + 6] = pAIActorFloat[nDataIndex + 2];
		pAIActorFloat[nDataIndex + 7] = pAIActorFloat[nDataIndex + 3];

		SAIDetectionLevels aiDetectionLevels;
		aiDetectionLevels.puppetExposure = pAIActorFloat[nDataIndex + 0];
		aiDetectionLevels.puppetThreat = pAIActorFloat[nDataIndex + 1];
		aiDetectionLevels.vehicleExposure = pAIActorFloat[nDataIndex + 2];
		aiDetectionLevels.vehicleThreat = pAIActorFloat[nDataIndex + 3];

		// Reset originals
		pAIActorFloat[nDataIndex + 0] = 0;
		pAIActorFloat[nDataIndex + 1] = 0;
		pAIActorFloat[nDataIndex + 2] = 0;
		pAIActorFloat[nDataIndex + 3] = 0;

		m_fDetectionValue = max(max(aiDetectionLevels.puppetExposure, aiDetectionLevels.puppetThreat),
								max(aiDetectionLevels.vehicleExposure, aiDetectionLevels.vehicleThreat));

	}*/
}

void CTOSPlayer::UpdateMusic(float frameTime)
{
	m_fMusicDelay += frameTime;

	if (IsClient() && m_fMusicDelay > 3.0f && !gEnv->bServer)
	{
		m_fMusicDelay = 0.f;

		const char* mood = gEnv->pMusicSystem->GetMood();

		if (!m_bMusicForceMood)
			m_fMusicIntensity = m_fDetectionValue;

		if (m_fMusicIntensity < 0.1f)
		{
			if (strcmp(mood, "incidental") != 0)
				gEnv->pMusicSystem->SetMood("incidental", false);
		}
		else if (m_fMusicIntensity < 0.2f)
		{
			if (strcmp(mood, "ambient") != 0)
				gEnv->pMusicSystem->SetMood("ambient", false);
		}
		else if (m_fMusicIntensity < 0.65f)
		{
			if (strcmp(mood, "middle") != 0)
				gEnv->pMusicSystem->SetMood("middle", false);
		}
		else
		{
			if (strcmp(mood, "action") != 0)
				gEnv->pMusicSystem->SetMood("action", false);
		}
	}
}

void CTOSPlayer::PostUpdate(float frameTime)
{
	//Crysis co-op
	if (gEnv->bServer)
	{
		// Called here not to interfere with AI system.
		UpdateDetectionValue(frameTime);
	}
	//~Crysis co-op
}

IMPLEMENT_RMI(CTOSPlayer, ClUpdateAwareness)
{
	m_fDetectionValue = params.awarenessFloat;
	return true;    // Always return true - false will drop connection
}
//~Crysis co-op