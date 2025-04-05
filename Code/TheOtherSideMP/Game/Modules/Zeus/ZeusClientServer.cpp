#include "StdAfx.h"
#include "GameActions.h"
#include "ZeusModule.h"
#include "ZeusSynchronizer.h"
#include <TheOtherSideMP/Helpers/TOS_AI.h>
#include <TheOtherSideMP/Helpers/TOS_Entity.h>
#include <TheOtherSideMP/Helpers/TOS_Inventory.h>
#include <TheOtherSideMP/Actors/Player/TOSPlayer.h>

// ПОКА НЕ ИСПОЛЬЗУЕТСЯ
void CTOSZeusModule::ClientServer::SetPP(int amount)
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

int CTOSZeusModule::ClientServer::GetPP()
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

void CTOSZeusModule::ClientServer::ServerOnEntitySpawned(EntityId id, const Vec3& pos, int clientChannelId)
{
	auto pSpawned = TOS_GET_ENTITY(id);
	assert(pSpawned != nullptr);
	assert(clientChannelId > 0);

	if (pSpawned == nullptr)
		return;

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

	pParent->GetSynchronizer()->RMISend(
		CTOSZeusSynchronizer::ClSpawnEntity(),
		info,
		eRMI_ToClientChannel,
		clientChannelId);
}

void CTOSZeusModule::ClientServer::ServerOnEntityCopied(EntityId id, const Vec3& pos, int clientChannelId)
{
	auto pSpawned = TOS_GET_ENTITY(id);
	assert(pSpawned != nullptr);
	assert(clientChannelId > 0);

	if (pSpawned == nullptr)
		return;

	char buffer[64];
	sprintf(buffer, "%d", id);
	pSpawned->SetName(string(pSpawned->GetName()) + "_" + buffer);

	// Извещаем клиента, о том, что он может перемещать заспавненную сущность
	CTOSZeusSynchronizer::NetSpawnedInfo info;
	info.spawnedId = id;
	info.spawnedPos = pos;

	pParent->GetSynchronizer()->RMISend(
		CTOSZeusSynchronizer::ClSpawnEntity(),
		info,
		eRMI_ToClientChannel,
		clientChannelId);
}

void CTOSZeusModule::ClientServer::DispatchMakeZeus(IActor* pPlayer, bool bMake)
{
	auto pTOSPlayer = static_cast<CTOSActor*>(pPlayer);

	CTOSZeusSynchronizer::NetMakeParams params;
	params.bMake = bMake;
	params.playerChannelId = pTOSPlayer->GetChannelId();

	if (gEnv->bClient)
	{
		if (params.bMake && pTOSPlayer->GetHealth() <= 0)
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
					pGameRules->ChangeSpectatorMode(pTOSPlayer, 0, 0, true);
				}
			}
		}

		pParent->GetSynchronizer()->RMISend(
			CTOSZeusSynchronizer::SvRequestMakeZeus(),
			params,
			eRMI_ToServer);
	}
	else
	{
		CTOSZeusModule::ClientServer::ServerMakeZeus(
			pParent->GetSynchronizer(),
			params.playerChannelId,
			params.bMake);
	}
}

bool CTOSZeusModule::ClientServer::ServerMakeZeus(const CTOSGenericSynchronizer* pZeusSync, int playerChannelId, bool make)
{
	auto pTOSPlayer = static_cast<CTOSPlayer*>(TOS_GET_ACTOR_CHANNELID(playerChannelId));
	if (!gEnv->bServer || !pZeusSync || !pTOSPlayer)
		return false;

	// Сбрасываем статы
	pTOSPlayer->GetActorStats()->inAir = 0.0f;
	pTOSPlayer->GetActorStats()->onGround = 0.0f;

	if (make)
	{
		// Становимся неуязвимым к урону
		pTOSPlayer->SetMeZeus(true);
		pTOSPlayer->GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Spectator);

		// Откл. ИИ для перса зевса
		auto pAI = pTOSPlayer->GetEntity()->GetAI();
		if (pAI)
			tos::ai::SendEvent(pAI, AIEVENT_DISABLE);

		// Режим полета со столкновениями
		pTOSPlayer->SetFlyMode(1);

		// убираем нанокостюм
		CNanoSuit* pSuit = pTOSPlayer->GetNanoSuit();
		if (pSuit)
		{
			pSuit->SetMode(NANOMODE_DEFENSE);
			pSuit->SetModeDefect(NANOMODE_CLOAK, true);
			pSuit->SetModeDefect(NANOMODE_SPEED, true);
			pSuit->SetModeDefect(NANOMODE_STRENGTH, true);
		}

		if (pTOSPlayer->GetAnimatedCharacter())
		{
			pTOSPlayer->GetAnimatedCharacter()->ForceRefreshPhysicalColliderMode();
			pTOSPlayer->GetAnimatedCharacter()->RequestPhysicalColliderMode(
				eColliderMode_Spectator,
				eColliderModeLayer_Game,
				"CTOSZeusModule::DispatchMakeZeus");
		}

		pTOSPlayer->GetGameObject()->InvokeRMI(
			CTOSActor::ClClearInventory(),
			CActor::NoParams(),
			eRMI_ToAllClients);

		pZeusSync->GetGameObject()->InvokeRMI(
			CTOSZeusSynchronizer::ClMakeZeus(),
			CTOSZeusSynchronizer::NetMakeParams(playerChannelId, make),
			eRMI_ToClientChannel,
			playerChannelId);

		tos::inventory::GiveItem(pTOSPlayer, "NightVision", false, false, false);
	}
	else
	{
		// Становимся уязвимым к урону
		pTOSPlayer->SetMeZeus(false);
		pTOSPlayer->GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);

		// Откл. ИИ для перса зевса
		auto pAI = pTOSPlayer->GetEntity()->GetAI();
		if (pAI)
			tos::ai::SendEvent(pAI, AIEVENT_ENABLE);

		// Отключаем режим полета
		pTOSPlayer->SetFlyMode(0);

		// убираем нанокостюм
		CNanoSuit* pSuit = pTOSPlayer->GetNanoSuit();
		if (pSuit)
		{
			pSuit->SetMode(NANOMODE_DEFENSE);
			pSuit->SetModeDefect(NANOMODE_CLOAK, false);
			pSuit->SetModeDefect(NANOMODE_SPEED, false);
			pSuit->SetModeDefect(NANOMODE_STRENGTH, false);
		}

		if (pTOSPlayer->GetAnimatedCharacter())
		{
			pTOSPlayer->GetAnimatedCharacter()->ForceRefreshPhysicalColliderMode();
			pTOSPlayer->GetAnimatedCharacter()->RequestPhysicalColliderMode(
				eColliderMode_Undefined,
				eColliderModeLayer_Game,
				"CTOSZeusModule::DispatchMakeZeus");
		}

		pTOSPlayer->GetGameObject()->InvokeRMI(
			CTOSActor::ClClearInventory(),
			CActor::NoParams(),
			eRMI_ToAllClients);

		pZeusSync->GetGameObject()->InvokeRMI(
			CTOSZeusSynchronizer::ClMakeZeus(),
			CTOSZeusSynchronizer::NetMakeParams(playerChannelId, make),
			eRMI_ToClientChannel,
			playerChannelId);
	}

    return true;
}

bool CTOSZeusModule::ClientServer::ClientMakeZeus(bool make)
{
	const auto pZeusModule = g_pTOSGame->GetZeusModule();
	if (!pZeusModule)
		return false;

	const auto pTOSPlayer = pZeusModule->GetPlayer();
	if (!gEnv->bClient || !pTOSPlayer)
		return false;

	if (make)
	{
		// Убираем лишние действия
		g_pGameActions->FilterZeus()->Enable(true);

		// Скрываем HUD игрока
		pZeusModule->GetHUD().ShowPlayerHUD(false);
		pZeusModule->GetHUD().ShowZeusMenu(true);

		//Включаем мышь
		pZeusModule->GetLocal().ShowMouse(true);
		pZeusModule->GetLocal().SetFlag(CTOSZeusModule::EFlag::CanUseMouse, true);

		//Включаем режим зевса
		pZeusModule->GetLocal().SetFlag(CTOSZeusModule::EFlag::Zeusing, true);
		// pZeusModule->SetPlayer(pTOSPlayer);

		pTOSPlayer->GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Spectator);

		// Режим полета со столкновениями
		pTOSPlayer->SetFlyMode(1);
		pTOSPlayer->SetMeZeus(true);

		// убираем нанокостюм
		CNanoSuit* pSuit = pTOSPlayer->GetNanoSuit();
		if (pSuit)
		{
			pSuit->SetMode(NANOMODE_DEFENSE);
			pSuit->SetModeDefect(NANOMODE_CLOAK, true);
			pSuit->SetModeDefect(NANOMODE_SPEED, true);
			pSuit->SetModeDefect(NANOMODE_STRENGTH, true);
		}

		if (pTOSPlayer->GetAnimatedCharacter())
		{
			pTOSPlayer->GetAnimatedCharacter()->ForceRefreshPhysicalColliderMode();
			pTOSPlayer->GetAnimatedCharacter()->RequestPhysicalColliderMode(
				eColliderMode_Spectator,
				eColliderModeLayer_Game,
				"CTOSZeusModule::DispatchMakeZeus");
		}

		pTOSPlayer->HideMe(true);
	}
	else
	{
		// Возвращаем доступные дейтсвия
		g_pGameActions->FilterZeus()->Enable(false);

		// Показываем HUD игрока
		pZeusModule->GetHUD().ShowPlayerHUD(true);
		pZeusModule->GetHUD().ShowZeusMenu(false);

		//Выключаем мышь
		if (pZeusModule->GetLocal().IsMouseDisplayed())
			pZeusModule->GetLocal().ShowMouse(false);		
		pZeusModule->GetLocal().SetFlag(CTOSZeusModule::EFlag::CanUseMouse, false);

		//Выключаем режим зевса
		pZeusModule->GetLocal().SetFlag(CTOSZeusModule::EFlag::Zeusing, false);
		// pZeusModule->SetPlayer(pTOSPlayer);

		pTOSPlayer->GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);

		// Режим полета со столкновениями
		pTOSPlayer->SetFlyMode(0);
		pTOSPlayer->SetMeZeus(false);

		// убираем нанокостюм
		CNanoSuit* pSuit = pTOSPlayer->GetNanoSuit();
		if (pSuit)
		{
			pSuit->SetMode(NANOMODE_DEFENSE);
			pSuit->SetModeDefect(NANOMODE_CLOAK, false);
			pSuit->SetModeDefect(NANOMODE_SPEED, false);
			pSuit->SetModeDefect(NANOMODE_STRENGTH, false);
		}

		if (pTOSPlayer->GetAnimatedCharacter())
		{
			pTOSPlayer->GetAnimatedCharacter()->ForceRefreshPhysicalColliderMode();
			pTOSPlayer->GetAnimatedCharacter()->RequestPhysicalColliderMode(
				eColliderMode_Undefined,
				eColliderModeLayer_Game,
				"CTOSZeusModule::DispatchMakeZeus");
		}

		pTOSPlayer->HideMe(false);
	}

	return true;
}
