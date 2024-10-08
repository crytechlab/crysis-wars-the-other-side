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

CTOSPlayer::CTOSPlayer()
	: m_pMasterClient(nullptr)
{
	
}

CTOSPlayer::~CTOSPlayer()
{
	if (m_pEnergyConsumer)
	{
		GetGameObject()->ReleaseExtension("CTOSEnergyConsumer");
		m_pEnergyConsumer = nullptr;
	}
}

bool CTOSPlayer::Init(IGameObject* pGameObject)
{
	if (!CPlayer::Init(pGameObject))
		return false;

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
	//	CryLogAlways("[C++][SERVER][FUNC CALL][CPlayer::InitClient] channelId: %i, ThisPlayer: %s", channelId, GetEntity()->GetName());
	//}
	//else if(gEnv->bClient)
	//{
	//	CryLogAlways(" ");
	//	CryLogAlways("[C++][CLIENT][FUNC CALL][CPlayer::InitClient] channelId: %i, ThisPlayer: %s", channelId, GetEntity()->GetName());
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
