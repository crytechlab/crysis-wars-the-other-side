/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#include "StdAfx.h"
#include "GameActions.h"
#include "ZeusModule.h"
#include "ZeusSynchronizer.h"
#include <TheOtherSideMP/Helpers/TOS_AI.h>
#include <TheOtherSideMP/Helpers/TOS_Inventory.h>
#include <TheOtherSideMP/Helpers/TOS_NET.h>
#include <TheOtherSideMP/Helpers/TOS_Entity.h>

// ПОКА НЕ ИСПОЛЬЗУЕТСЯ
void CTOSZeusModule::Network::SetPP(int amount)
{
	if (!gEnv->bServer)
		return;

	CGameRules* pGameRules = g_pGame->GetGameRules();
	IScriptTable* pScriptTable = pGameRules->GetEntity()->GetScriptTable();
	if (pScriptTable)
	{
		//FIXME: НУЖНО СИНХРОНИТЬ И НЕ ИСПОЛЬЗОВТАЬ ЛОК. АКТЕРА
		pGameRules->SetSynchedEntityValue(
			pParent->GetPlayer()->GetEntityId(), 
			TSynchedKey(ZEUS_PP_AMOUNT_KEY), 
			amount);
	}
}

int CTOSZeusModule::Network::GetPP()
{
	if (!pParent->GetPlayer())
		return 0;

	int pp = 0;
	CGameRules* pGameRules = g_pGame->GetGameRules();
	IScriptTable* pScriptTable = pGameRules->GetEntity()->GetScriptTable();
	if (pScriptTable)
		pGameRules->GetSynchedEntityValue(
			pParent->GetPlayer()->GetEntityId(), 
			TSynchedKey(ZEUS_PP_AMOUNT_KEY), 
			pp);

	return pp;
}
// ~ПОКА НЕ ИСПОЛЬЗУЕТСЯ

void CTOSZeusModule::Network::ServerEntitySpawned(EntityId id, const Vec3& pos, int clientChannelId)
{
	auto pSpawned = TOS_GET_ENTITY(id);
	assert(pSpawned != nullptr);
	assert(clientChannelId > 0);

	char buffer[64];
	sprintf(buffer, "%d", id);
	pSpawned->SetName(string(pSpawned->GetName()) + "_" + buffer);

	// Извещаем клиента, о том, что он может перемещать заспавненную сущность
	CTOSZeusSynchronizer::NetSpawnedInfo info;
	CTOSZeusSynchronizer::NetHideParams params;

	info.spawnedId = id;
	info.spawnedPos = pos;

	params.id = id;
	params.bHide = false;

	auto pSync = static_cast<CTOSZeusSynchronizer*>(pParent->GetSynchronizer());
	assert(pSync != nullptr);
	pSync->RMISend(
		CTOSZeusSynchronizer::ClSpawnEntity(), 
		info, 
		eRMI_ToClientChannel, 
		clientChannelId);
	// pSync->RMISend(CTOSZeusSynchronizer::ClHideEntity(), params, eRMI_ToAllClients | eRMI_NoLocalCalls);
}

void CTOSZeusModule::Network::ServerEntityCopied(EntityId id, const Vec3& pos, int clientChannelId)
{
	auto pSpawned = TOS_GET_ENTITY(id);
	assert(pSpawned != nullptr);
	assert(clientChannelId > 0);

	char buffer[64];
	sprintf(buffer, "%d", id);
	pSpawned->SetName(string(pSpawned->GetName()) + "_" + buffer);

	// Извещаем клиента, о том, что он может перемещать заспавненную сущность
	CTOSZeusSynchronizer::NetSpawnedInfo info;

	info.spawnedId = id;
	info.spawnedPos = pos;

	auto pSync = static_cast<CTOSZeusSynchronizer*>(pParent->GetSynchronizer());
	assert(pSync != nullptr);
	pSync->RMISend(
		CTOSZeusSynchronizer::ClSpawnEntity(), 
		info,
		eRMI_ToClientChannel,
		clientChannelId);
}

void CTOSZeusModule::Network::MakeZeus(IActor* pPlayer, bool bMake)
{
	CTOSZeusSynchronizer::NetMakeParams params;
	params.bMake = bMake;
	params.playerChannelId = pPlayer->GetChannelId();

	if (gEnv->bClient)
	{

		CGameRules* pGameRules = g_pGame->GetGameRules();
		if (pGameRules)
		{
			const int teamCount = pGameRules->GetTeamCount();
			if (teamCount > 0)
			{
				pGameRules->ChangeTeam(pPlayer, "zeus");
			}
			else
			{
				auto pTOSPlayer = static_cast<CTOSActor*>(pPlayer);
				pGameRules->ChangeSpectatorMode(pTOSPlayer, 0, 0, true);
			}
		}

		auto pSync = static_cast<CTOSZeusSynchronizer*>(pParent->GetSynchronizer());
		pSync->GetGameObject()->InvokeRMI(
			CTOSZeusSynchronizer::SvRequestMakeZeus(), 
			params, 
			eRMI_ToServer);
	}
}
