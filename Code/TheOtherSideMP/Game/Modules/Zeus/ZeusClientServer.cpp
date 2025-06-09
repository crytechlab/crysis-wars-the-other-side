#include "StdAfx.h"
#include "GameActions.h"
#include "ZeusModule.h"
#include "ZeusSynchronizer.h"
#include <TheOtherSideMP/Helpers/TOS_AI.h>
#include <TheOtherSideMP/Helpers/TOS_Entity.h>
#include <TheOtherSideMP/Helpers/TOS_Inventory.h>
#include <TheOtherSideMP/Helpers/TOS_NET.h>
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
		CRY_FIXME(9,6,2025,"НУЖНО СИНХРОНИТЬ И НЕ ИСПОЛЬЗОВТАЬ ЛОК. АКТЕРА");
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

bool CTOSZeusModule::ClientServer::DispatchMakeZeus(IActor *pActor, bool bMake, const char *desiredTeam)
{
	if (!pActor)
		return false;

	auto pModule = g_pTOSGame->GetZeusModule();
	auto pSync = pModule->GetSynchronizer();
	if (!pSync)
		return false;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (!pGameRules)
		return false;

	CTOSPlayer *pPlayer = static_cast<CTOSPlayer *>(pActor);

	if (bMake == false && pModule->GetLocal().GetFlag(CTOSZeusModule::EFlag::Zeusing) && pPlayer->GetSpectatorMode() != CActor::eASM_None)
	{
		// Если выходим из режима Зевса, то переключаемся в режим зрителя
		pGameRules->ChangeSpectatorMode(pPlayer, CActor::eASM_Fixed, 0, true);
	}
	else
		// Если не были в режиме Зевса, то переключаемся в режим Зрителя Зевса в зависимости от параметра
		pGameRules->ChangeSpectatorMode(pPlayer, bMake ? CActor::eASM_Zeus : CActor::eASM_None, 0, true);

	if (gEnv->bServer)
	{
		return CTOSZeusModule::ClientServer::ServerMakeZeus(
			pPlayer->GetChannelId(),
			bMake,
			desiredTeam);
	}
	else if (pPlayer->IsClient())
	{
		return CTOSZeusModule::ClientServer::ClientMakeZeus(bMake);
	}
}

bool CTOSZeusModule::ClientServer::ServerMakeZeus(int playerChannelId, bool make, const char *desiredTeam)
{
	const auto pSync = g_pTOSGame->GetZeusModule()->GetSynchronizer();
	if (!pSync)
		return false;

	const auto pTOSPlayer = static_cast<CTOSPlayer *>(TOS_GET_ACTOR_CHANNELID(playerChannelId));
	if (!gEnv->bServer || !pTOSPlayer)
		return false;

	const auto params = CTOSZeusSynchronizer::NetMakeParams(playerChannelId, make, desiredTeam);
	const auto playerId = pTOSPlayer->GetEntityId();
	const bool isLocalClient = pTOSPlayer->IsClient();

	if (isLocalClient)
	{
		pSync->GetGameObject()->InvokeRMI(
			CTOSZeusSynchronizer::ClMakeZeus(),
			params,
			eRMI_ToClientChannel,
			playerChannelId);
	}
	else
	{
		// Если есть Sv+CL и игрок локальный, то почему то этот RMI не срабатывает
		pSync->GetGameObject()->InvokeRMIWithDependentObject(
			CTOSZeusSynchronizer::ClMakeZeus(),
			params,
			eRMI_ToClientChannel,
			playerId,
			playerChannelId);
	}

	if (make)
		tos::inventory::GiveItem(pTOSPlayer, "NightVision", false, false, false);

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

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestMakeZeus)
{
	CryLog("[%s][%s][SvRequestMakeZeus]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	return CTOSZeusModule::ClientServer::ServerMakeZeus(
		params.playerChannelId,
		params.bMake,
		params.desiredTeam);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, ClMakeZeus)
{
	CryLog("[%s][%s][ClMakeZeus]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	return CTOSZeusModule::ClientServer::ClientMakeZeus(params.bMake);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestSpawnEntity)
{
	if (gEnv->bServer)
	{
		CryLog("[%s][%s][SvRequestSpawnEntity]",
			   tos::debug::GetEnv(), tos::debug::GetAct(3));

		auto pZeusModule = g_pTOSGame->GetZeusModule();
		assert(pZeusModule != nullptr);

		STOSEntityDelaySpawnParams spawnParams;
		spawnParams.clientChannelId = params.playerChannelId;
		spawnParams.hide = true;
		spawnParams.spawnDelay = 1.0f;
		spawnParams.saveParams = false;
		spawnParams.vanilla.bStaticEntityId = false; // true - вылетает в редакторе и медленно работает O(n), false O(1)
		spawnParams.vanilla.bIgnoreLock = false;	 // spawn lock игнор
		spawnParams.callback = [clientServer = &pZeusModule->GetClientServer()](EntityId id, const Vec3 &pos, int clientChannelId)
		{
			clientServer->ServerOnEntitySpawned(id, pos, clientChannelId);
		};

		// auto pPlayer = TOS_GET_ACTOR_CHANNELID(params.playerChannelId);
		// if (pPlayer)
		// spawnParams.authorityPlayerName = pPlayer->GetEntity()->GetName();

		const string *const psClassName = &params.className;
		IEntityClass *pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(psClassName->c_str());

		const string name = string("zeus_") + psClassName->c_str();
		spawnParams.name = name;
		spawnParams.vanilla.pClass = pClass;
		spawnParams.vanilla.vPosition = params.pos;
		spawnParams.vanilla.nFlags |= ENTITY_FLAG_NET_PRESENT | ENTITY_FLAG_CASTSHADOW | ENTITY_FLAG_TRIGGER_AREAS;

		const auto pArchetype = gEnv->pEntitySystem->LoadEntityArchetype(psClassName->c_str());
		if (pArchetype)
			spawnParams.archetypeName = pArchetype->GetName();

		if (!pClass && !pArchetype)
		{
			CryLogError("[Zeus] not defined entity class '%s'", psClassName->c_str());
			return true;
		}

		bool bSpawned = tos::entity::SpawnDelay(spawnParams, true);
		if (!bSpawned)
		{
			CryLogError("[Zeus] entity with class '%s' spawn failed!", psClassName->c_str());
			return true;
		}
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, ClSpawnEntity)
{
	// Здесь пишем всё, что должно выполниться на клиенте

	CryLog("[%s][%s][ClSpawnEntity] %s",
		   tos::debug::GetEnv(), tos::debug::GetAct(3), TOS_GET_ENTITY(params.spawnedId)->GetName());

	auto pZeusModule = g_pTOSGame->GetZeusModule();
	assert(pZeusModule != nullptr);

	pZeusModule->GetLocal().DeselectEntities();

	pZeusModule->GetLocal().m_dragging = true;
	pZeusModule->GetHUD().m_menuSpawnHandling = true;

	pZeusModule->GetLocal().SelectEntity(params.spawnedId);
	pZeusModule->GetLocal().ClickEntity(params.spawnedId, params.spawnedPos);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, ClCopyEntity)
{
	// Здесь пишем всё, что должно выполниться на клиенте

	CryLog("[%s][%s][ClCopyEntity] %s",
		   tos::debug::GetEnv(), tos::debug::GetAct(3), TOS_GET_ENTITY(params.copiedId)->GetName());

	auto pZeusModule = g_pTOSGame->GetZeusModule();
	assert(pZeusModule != nullptr);

	pZeusModule->GetLocal().DeselectEntity(params.originalId);
	pZeusModule->GetLocal().m_dragging = true;
	pZeusModule->GetLocal().SelectEntity(params.copiedId);
	pZeusModule->GetLocal().ClickEntity(params.copiedId, params.copiedPos);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestTransformEntity)
{
	CryLog("[%s][%s][SvRequestTransformEntity]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	auto pEntity = TOS_GET_ENTITY(params.id);
	if (!pEntity)
		return true;

	auto pPhys = pEntity->GetPhysics();
	if (pPhys)
	{
		pe_action_awake awake;
		awake.bAwake = 1;
		pPhys->Action(&awake);
	}

	Vec3 pos = params.pos;
	Quat rot = Quat::CreateRotationVDir(params.dir);
	pEntity->SetWorldTM(Matrix34::Create(Vec3(1, 1, 1), rot, pos));
	pEntity->SetRotation(rot);

	RMISend(ClTransformEntity(), params, eRMI_ToAllClients | eRMI_NoLocalCalls);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, ClTransformEntity)
{
	CryLog("[%s][%s][SvRequestTransformEntity]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	auto pEntity = TOS_GET_ENTITY(params.id);
	if (!pEntity)
		return true;

	auto pPhys = pEntity->GetPhysics();
	if (pPhys)
	{
		pe_action_awake awake;
		awake.bAwake = 1;
		pPhys->Action(&awake);
	}

	Vec3 pos = params.pos;
	Quat rot = Quat::CreateRotationVDir(params.dir);
	pEntity->SetWorldTM(Matrix34::Create(Vec3(1, 1, 1), rot, pos));
	pEntity->SetRotation(rot);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestRemoveEntity)
{
	CryLog("[%s][%s][SvRequestRemoveEntity]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	auto pActor = TOS_GET_ACTOR(params.id);
	if (pActor)
	{
		auto pActorVeh = pActor->GetLinkedVehicle();
		if (pActorVeh)
		{
			tos::vehicle::Exit(pActor, false, true);
		}
	}

	auto pEntity = TOS_GET_ENTITY(params.id);
	if (pEntity)
	{
		pEntity->Hide(true);
		pEntity->Activate(false);
	}

	tos::entity::RemoveEntityDelayed(params.id, 2);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestKillEntity)
{
	CryLog("[%s][%s][SvRequestKillEntity]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	string hitType = "event";

	HitInfo info;
	info.SetDamage(99999.0f);
	info.shooterId = params.shooterId;
	info.targetId = params.targetId;
	info.type = g_pGame->GetGameRules()->GetHitTypeId(hitType.c_str());

	g_pGame->GetGameRules()->ServerHit(info);

	auto pVehicle = TOS_GET_VEHICLE(params.targetId);
	if (pVehicle)
		tos::vehicle::Destroy(pVehicle);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestExecuteOrder)
{
	CryLog("[%s][%s][SvRequestExecuteOrder]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	IScriptSystem *pSS = gEnv->pScriptSystem;
	if (pSS->ExecuteFile("Scripts/AI/TOS/TOSHandleOrder.lua", true, true))
	{
		SmartScriptTable executorInfo;
		SmartScriptTable orderInfo;

		executorInfo.Create(pSS);
		orderInfo.Create(pSS);

		executorInfo->SetValue("entityId", params.id);
		executorInfo->SetValue("maxCount", params.maxCount); // макс. кол-во исполнителей
		executorInfo->SetValue("index", params.index);		 // текущий номер исполнителя

		orderInfo->SetValue("goalPipeId", params.id); // так надо
		orderInfo->SetValue("pos", params.pos);
		orderInfo->SetValue("targetId", params.targetId);

		pSS->BeginCall("HandleOrder");
		pSS->PushFuncParam(executorInfo);
		pSS->PushFuncParam(orderInfo);
		pSS->EndCall();
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestHideEntity)
{
	CryLog("[%s][%s][SvRequestHideEntity]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	auto pEntity = TOS_GET_ENTITY(params.id);
	if (pEntity)
	{
		pEntity->Hide(params.bHide);
	}

	RMISend(ClHideEntity(), params, eRMI_ToAllClients | eRMI_NoLocalCalls);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, ClHideEntity)
{
	CryLog("[%s][%s][ClHideEntity]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	auto pEntity = TOS_GET_ENTITY(params.id);
	if (pEntity)
	{
		pEntity->Hide(params.bHide);
		pEntity->EnablePhysics(!params.bHide);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestAIMakeHostile)
{
	CryLog("[%s][%s][SvRequestAIMakeHostile]",
		   tos::debug::GetEnv(), tos::debug::GetAct(3));

	auto pEntity = TOS_GET_ENTITY(params.id);
	if (pEntity)
	{
		bool hostile = params.bHostile;
		tos::script::GetEntityProperty(pEntity, "bSpeciesHostility", hostile);
		tos::ai::MakeHostile(pEntity->GetAI(), hostile);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestCopyEntity)
{
	if (gEnv->bServer)
	{
		CryLog("[%s][%s][SvRequestCopyEntity]",
			   tos::debug::GetEnv(), tos::debug::GetAct(3));

		auto pZeusModule = g_pTOSGame->GetZeusModule();
		assert(pZeusModule != nullptr);

		STOSEntityDelaySpawnParams spawnParams;
		spawnParams.clientChannelId = params.playerChannelId;
		spawnParams.hide = true;
		spawnParams.spawnDelay = 1.0f;
		spawnParams.saveParams = false;
		spawnParams.vanilla.bStaticEntityId = false; // true - вылетает в редакторе и медленно работает O(n), false O(1)
		spawnParams.vanilla.bIgnoreLock = false;	 // spawn lock игнор
		spawnParams.callback = [clientServer = &pZeusModule->GetClientServer(),
								copiedId = params.copiedId](EntityId id, const Vec3 &pos, int clientChannelId)
		{
			clientServer->ServerOnEntityCopied(id, pos, clientChannelId, copiedId);
		};

		auto pCopiedEntity = TOS_GET_ENTITY(params.copiedId);
		if (!pCopiedEntity)
		{
			CryLogError("[Zeus] copied entity with id '%i' not found!", params.copiedId);
			return true;
		}

		SmartScriptTable props;
		SmartScriptTable propsIns;
		tos::script::GetEntityScriptValue(pCopiedEntity, "Properties", props);
		tos::script::GetEntityScriptValue(pCopiedEntity, "PropertiesInstance", propsIns);

		spawnParams.properties = props;
		spawnParams.propertiesInstance = propsIns;
		spawnParams.archetypeName = pCopiedEntity->GetArchetype() ? pCopiedEntity->GetArchetype()->GetName() : "";
		spawnParams.name = string("zeus_") + pCopiedEntity->GetClass()->GetName();
		spawnParams.vanilla.pClass = pCopiedEntity->GetClass();
		spawnParams.vanilla.vPosition = pCopiedEntity->GetWorldPos();
		spawnParams.vanilla.nFlags = pCopiedEntity->GetFlags();

		tos::entity::SpawnDelay(spawnParams, true);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestVehicleEnter)
{
	IVehicle *pVehicle = TOS_GET_VEHICLE(params.vehicleId);
	IActor *pActor = TOS_GET_ACTOR(params.actorId);

	if (!pVehicle || !pActor)
		return true;

	CTOSZeusModule::ClientServer::ServerEnterVehicle(pActor, pVehicle, params.fast);

	return true;
}