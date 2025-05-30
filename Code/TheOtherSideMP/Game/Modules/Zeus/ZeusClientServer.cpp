#include "StdAfx.h"
#include "GameActions.h"
#include "ZeusModule.h"
#include "ZeusSynchronizer.h"
#include <TheOtherSideMP/Helpers/TOS_AI.h>
#include <TheOtherSideMP/Helpers/TOS_Entity.h>
#include <TheOtherSideMP/Helpers/TOS_Inventory.h>
#include <TheOtherSideMP/Helpers/TOS_Vehicle.h>
#include <TheOtherSideMP/Actors/Player/TOSPlayer.h>
#include <IPhysics.h>

// ПОКА НЕ ИСПОЛЬЗУЕТСЯ
void CTOSZeusModule::ClientServer::SetPP(int amount)
{
	if (!gEnv->bServer)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	IScriptTable *pScriptTable = pGameRules->GetEntity()->GetScriptTable();
	if (pScriptTable)
	{
		// FIXME: НУЖНО СИНХРОНИТЬ И НЕ ИСПОЛЬЗОВТАЬ ЛОК. АКТЕРА
		pGameRules->SetSynchedEntityValue(
			pParent->GetPlayer()->GetEntityId(),
			TSynchedKey(ZEUS_PP_AMOUNT_KEY),
			amount);
	}
}

int CTOSZeusModule::ClientServer::GetPP()
{
	if (!pParent->GetPlayer())
		return 0;

	int pp = 0;
	CGameRules *pGameRules = g_pGame->GetGameRules();
	IScriptTable *pScriptTable = pGameRules->GetEntity()->GetScriptTable();
	if (pScriptTable)
		pGameRules->GetSynchedEntityValue(
			pParent->GetPlayer()->GetEntityId(),
			TSynchedKey(ZEUS_PP_AMOUNT_KEY),
			pp);

	return pp;
}
// ~ПОКА НЕ ИСПОЛЬЗУЕТСЯ

void CTOSZeusModule::ClientServer::ServerOnEntitySpawned(EntityId id, const Vec3 &pos, int clientChannelId)
{
	auto pSpawned = TOS_GET_ENTITY(id);
	assert(pSpawned != nullptr);
	assert(clientChannelId > 0);

	// CryLogAlways("[TOS][ServerOnEntitySpawned] %s", pSpawned->GetName());

	if (pSpawned == nullptr)
		return;

	// char buffer[64];
	// sprintf(buffer, "%d", id);
	// pSpawned->SetName(string(pSpawned->GetName()) + "_" + buffer);

	// Извещаем клиента, о том, что он может перемещать заспавненную сущность
	CTOSZeusSynchronizer::NetSpawnedInfo info;
	info.spawnedId = id;
	info.spawnedPos = pos;

	pParent->GetSynchronizer()->GetGameObject()->InvokeRMIWithDependentObject(
		CTOSZeusSynchronizer::ClSpawnEntity(),
		info,
		eRMI_ToClientChannel,
		info.spawnedId,
		clientChannelId);
}

void CTOSZeusModule::ClientServer::ServerOnEntityCopied(EntityId id, const Vec3 &pos, int clientChannelId, EntityId originalId)
{
	auto pSpawned = TOS_GET_ENTITY(id);
	assert(pSpawned != nullptr);

	if (pSpawned == nullptr)
		return;

	// CryLogAlways("[TOS][ServerOnEntityCopied] %s", pSpawned->GetName());

	// char buffer[64];
	// sprintf(buffer, "%d", id);
	// pSpawned->SetName(string(pSpawned->GetName()) + "_" + buffer);

	// Извещаем клиента, о том, что он может перемещать заспавненную сущность
	CTOSZeusSynchronizer::NetCopiedInfo info;
	info.originalId = originalId;
	info.copiedId = id;
	info.copiedPos = pos;

	pParent->GetSynchronizer()->GetGameObject()->InvokeRMIWithDependentObject(
		CTOSZeusSynchronizer::ClCopyEntity(),
		info,
		eRMI_ToClientChannel,
		info.copiedId,
		clientChannelId);
}

bool CTOSZeusModule::ClientServer::DispatchMakeZeus(IActor *pPlayer, bool bMake, const char *desiredTeam)
{
	if (!pPlayer)
		return false;

	auto pSync = g_pTOSGame->GetZeusModule()->GetSynchronizer();
	if (!pSync)
		return false;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (!pGameRules)
		return false;

	// Нужно для того чтобы игрок летал и был невидимым
	pGameRules->ChangeSpectatorMode(static_cast<CActor *>(pPlayer), bMake ? CActor::eASM_Zeus : CActor::eASM_None, 0, true);

	if (gEnv->bServer)
	{
		return CTOSZeusModule::ClientServer::ServerMakeZeus(
			pPlayer->GetChannelId(),
			bMake,
			desiredTeam);
	}
	else
	{
		// pSync->GetGameObject()->InvokeRMI(
		// 	CTOSZeusSynchronizer::SvRequestMakeZeus(),
		// 	CTOSZeusSynchronizer::NetMakeParams(pPlayer->GetChannelId(), bMake, desiredTeam),
		// 	eRMI_ToServer);

		return CTOSZeusModule::ClientServer::ClientMakeZeus(bMake);
	}

	// CTOSZeusSynchronizer::NetMakeParams params;
	// params.bMake = bMake;
	// params.playerChannelId = pTOSPlayer->GetChannelId();
	// params.desiredTeam = desiredTeam;

	// if (gEnv->bClient)
	// {
	// 	if (params.bMake && pTOSPlayer->GetHealth() <= 0)
	// 	{
	// 		CGameRules* pGameRules = g_pGame->GetGameRules();
	// 		if (pGameRules)
	// 		{
	// 			const int teamCount = pGameRules->GetTeamCount();
	// 			if (teamCount > 0)
	// 			{
	// 				pGameRules->ChangeTeam(pPlayer, "zeus");
	// 			}
	// 			else
	// 			{
	// 				pGameRules->ChangeSpectatorMode(pTOSPlayer, 0, 0, true);
	// 			}
	// 		}
	// 	}

	// 	pSync->GetGameObject()->InvokeRMI(
	// 		CTOSZeusSynchronizer::SvRequestMakeZeus(),
	// 		params,
	// 		eRMI_ToServer);

	// 	return true;
	// }
	// else
	// {
	// 	return CTOSZeusModule::ClientServer::ServerMakeZeus(
	// 		params.playerChannelId,
	// 		params.bMake,
	// 		desiredTeam);
	// }
}

bool CTOSZeusModule::ClientServer::ServerMakeZeus(int playerChannelId, bool make, const char *desiredTeam)
{
	auto pSync = g_pTOSGame->GetZeusModule()->GetSynchronizer();
	if (!pSync)
		return false;

	auto pTOSPlayer = static_cast<CTOSPlayer *>(TOS_GET_ACTOR_CHANNELID(playerChannelId));
	if (!gEnv->bServer || !pTOSPlayer)
		return false;

	if (make)
	{
		pSync->GetGameObject()->InvokeRMIWithDependentObject(
			CTOSZeusSynchronizer::ClMakeZeus(),
			CTOSZeusSynchronizer::NetMakeParams(playerChannelId, make, desiredTeam),
			eRMI_ToClientChannel,
			pTOSPlayer->GetEntityId(),
			playerChannelId);

		tos::inventory::GiveItem(pTOSPlayer, "NightVision", false, false, false);
	}
	else
	{
		pSync->GetGameObject()->InvokeRMIWithDependentObject(
			CTOSZeusSynchronizer::ClMakeZeus(),
			CTOSZeusSynchronizer::NetMakeParams(playerChannelId, make, desiredTeam),
			eRMI_ToClientChannel,
			pTOSPlayer->GetEntityId(),
			playerChannelId);
	}

	return true;
}

bool CTOSZeusModule::ClientServer::ClientMakeZeus(bool make, bool bfromInit)
{
	const auto pZeusModule = g_pTOSGame->GetZeusModule();
	if (!pZeusModule)
		return false;

	const auto pLocalPlayer = pZeusModule->GetPlayer();
	if (!pLocalPlayer)
		return false;

	const bool alreadyZeusing = pZeusModule->GetLocal().GetFlag(CTOSZeusModule::EFlag::Zeusing);

	if (make)
	{
		if (!alreadyZeusing || bfromInit)
		{
			// Убираем лишние действия
			g_pGameActions->FilterZeus()->Enable(true);

			// Скрываем HUD игрока
			pZeusModule->GetHUD().ShowPlayerHUD(false);
			pZeusModule->GetHUD().ShowZeusMenu(true);

			// Включаем мышь
			if (!pZeusModule->GetLocal().IsMouseDisplayed())
				pZeusModule->GetLocal().ShowMouse(true);
			pZeusModule->GetLocal().SetFlag(CTOSZeusModule::EFlag::CanUseMouse, true);

			// Включаем режим зевса
			pZeusModule->GetLocal().SetFlag(CTOSZeusModule::EFlag::Zeusing, true);
		}
	}
	else
	{
		if (alreadyZeusing)
		{
			// Возвращаем доступные дейтсвия
			g_pGameActions->FilterZeus()->Enable(false);

			// Показываем HUD игрока
			pZeusModule->GetHUD().ShowPlayerHUD(true);
			pZeusModule->GetHUD().ShowZeusMenu(false);

			// Выключаем мышь
			if (pZeusModule->GetLocal().IsMouseDisplayed())
				pZeusModule->GetLocal().ShowMouse(false);

			// Сбрасываем все флаги и состояния
			pZeusModule->GetLocal().Reset();
		}
	}

	return true;
}

bool CTOSZeusModule::ClientServer::DispatchEnterVehicle(IActor *pActor, IVehicle *pVehicle, bool fast)
{
	if (!pActor || !pVehicle)
		return false;

	if (gEnv->bServer)
		return ServerEnterVehicle(pActor, pVehicle, fast);
	else
		return ClientEnterVehicle(pActor, pVehicle, fast);
}

bool CTOSZeusModule::ClientServer::ClientEnterVehicle(IActor *pActor, IVehicle *pVehicle, bool fast)
{
	auto pSync = g_pTOSGame->GetZeusModule()->GetSynchronizer();
	if (!pSync)
		return false;

	if (!pActor || !pVehicle || !pSync)
		return false;

	const auto pZeusModule = g_pTOSGame->GetZeusModule();
	if (!pZeusModule)
		return false;

	if (!gEnv->bClient)
		return false;

	pSync->GetGameObject()->InvokeRMI(
		CTOSZeusSynchronizer::SvRequestVehicleEnter(),
		CTOSZeusSynchronizer::NetServerEnterVehicleParams(
			pActor->GetEntityId(),
			pVehicle->GetEntityId(),
			fast),
		eRMI_ToServer);

	return true;
}

bool CTOSZeusModule::ClientServer::ServerEnterVehicle(IActor *pActor, IVehicle *pVehicle, bool fast)
{
	if (!pActor || !pVehicle)
		return false;

	if (!gEnv->bServer)
		return false;

	return tos::vehicle::Enter(pActor, pVehicle, fast);
}
