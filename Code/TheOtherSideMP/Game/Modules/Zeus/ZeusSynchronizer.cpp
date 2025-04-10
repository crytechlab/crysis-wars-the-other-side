#include "StdAfx.h"
#include "GameActions.h"
#include "ZeusSynchronizer.h"
#include "ZeusModule.h"
#include <TheOtherSideMP/Helpers/TOS_AI.h>
#include <TheOtherSideMP/Helpers/TOS_Entity.h>
#include <TheOtherSideMP/Helpers/TOS_Inventory.h>
#include <TheOtherSideMP/Helpers/TOS_NET.h>
#include <TheOtherSideMP/Helpers/TOS_Vehicle.h>

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestMakeZeus)
{
	//TODO:
	// 2) Через меню паузы можно кликнуть по объектам...

	CryLog("<C++>[%s][%s][SvRequestMakeZeus]",
		tos::debug::GetEnv(), tos::debug::GetAct(3));

	return CTOSZeusModule::ClientServer::ServerMakeZeus(
		this, 
		params.playerChannelId, 
		params.bMake);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, ClMakeZeus)
{
	CryLog("<C++>[%s][%s][ClMakeZeus]",
		tos::debug::GetEnv(), tos::debug::GetAct(3));

	return CTOSZeusModule::ClientServer::ClientMakeZeus(params.bMake);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestSpawnEntity)
{
	if (gEnv->bServer)
	{
		CryLog("<C++>[%s][%s][SvRequestSpawnEntity]",
			tos::debug::GetEnv(), tos::debug::GetAct(3));

		auto pZeusModule = g_pTOSGame->GetZeusModule();
		assert(pZeusModule != nullptr);

		STOSEntityDelaySpawnParams spawnParams;
		spawnParams.pCallback = std::bind(
			&CTOSZeusModule::ClientServer::ServerOnEntitySpawned, 
			&pZeusModule->GetClientServer(), 
			std::placeholders::_1, 
			std::placeholders::_2,
			std::placeholders::_3);

		spawnParams.clientChannelId = params.playerChannelId;
		spawnParams.hide = true;
		spawnParams.spawnDelay = 1.0f;
		spawnParams.saveParams = false;
		spawnParams.vanilla.bStaticEntityId = false; // true - вылетает в редакторе и медленно работает O(n), false O(1)
		spawnParams.vanilla.bIgnoreLock = false; // spawn lock игнор

		auto pPlayer = TOS_GET_ACTOR_CHANNELID(params.playerChannelId);
		if (pPlayer)
			spawnParams.authorityPlayerName = pPlayer->GetEntity()->GetName();

		const string* const psClassName = &params.className;
		IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(psClassName->c_str());

		const string name = string("zeus_") + psClassName->c_str();
		spawnParams.vanilla.sName = name;
		spawnParams.vanilla.pClass = pClass;
		spawnParams.vanilla.vPosition = params.pos;
		spawnParams.vanilla.nFlags |= ENTITY_FLAG_NET_PRESENT | ENTITY_FLAG_CASTSHADOW;

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

	CryLog("<C++>[%s][%s][ClSpawnEntity]",
		tos::debug::GetEnv(), tos::debug::GetAct(3));
	 
	auto pZeusModule = g_pTOSGame->GetZeusModule();
	assert(pZeusModule != nullptr);

	pZeusModule->GetLocal().DeselectEntities();

	pZeusModule->GetLocal().m_dragging = true;
	pZeusModule->GetHUD().m_menuSpawnHandling = true;

	//TODO: не выделяется сущность после спавна
	pZeusModule->GetLocal().SelectEntity(params.spawnedId);
	pZeusModule->GetLocal().ClickEntity(params.spawnedId, params.spawnedPos);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestTransformEntity)
{
	CryLog("<C++>[%s][%s][SvRequestTransformEntity]",
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
	CryLog("<C++>[%s][%s][SvRequestTransformEntity]",
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
	CryLog("<C++>[%s][%s][SvRequestRemoveEntity]",
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
	CryLog("<C++>[%s][%s][SvRequestKillEntity]",
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
	CryLog("<C++>[%s][%s][SvRequestExecuteOrder]",
		tos::debug::GetEnv(), tos::debug::GetAct(3));

	IScriptSystem* pSS = gEnv->pScriptSystem;
	if (pSS->ExecuteFile("Scripts/AI/TOS/TOSHandleOrder.lua", true, true))
	{
		SmartScriptTable executorInfo;
		SmartScriptTable orderInfo;

		executorInfo.Create(pSS);
		orderInfo.Create(pSS);

		executorInfo->SetValue("entityId", params.id);
		executorInfo->SetValue("maxCount", params.maxCount); // макс. кол-во исполнителей
		executorInfo->SetValue("index", params.index); // текущий номер исполнителя
		
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
	CryLog("<C++>[%s][%s][SvRequestHideEntity]",
		tos::debug::GetEnv(), tos::debug::GetAct(3));

	auto pEntity = TOS_GET_ENTITY(params.id);
	if (pEntity)
		pEntity->Hide(params.bHide);

	RMISend(ClHideEntity(), params, eRMI_ToAllClients | eRMI_NoLocalCalls);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, ClHideEntity)
{
	CryLog("<C++>[%s][%s][ClHideEntity]",
		tos::debug::GetEnv(), tos::debug::GetAct(3));

	auto pEntity = TOS_GET_ENTITY(params.id);
	if (pEntity)
	{
		pEntity->Hide(params.bHide);
		pEntity->EnablePhysics(!params.bHide);
	}// TODO: физика не отключается при копировании и коллайдит с сузествующей сущностью

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestAIMakeHostile)
{
	CryLog("<C++>[%s][%s][SvRequestAIMakeHostile]",
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
		CryLog("<C++>[%s][%s][SvRequestCopyEntity]",
			tos::debug::GetEnv(), tos::debug::GetAct(3));

		auto pZeusModule = g_pTOSGame->GetZeusModule();
		assert(pZeusModule != nullptr);

		STOSEntityDelaySpawnParams spawnParams;
		spawnParams.pCallback = std::bind(
			&CTOSZeusModule::ClientServer::ServerOnEntityCopied,
			&pZeusModule->GetClientServer(),
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3);

		spawnParams.clientChannelId = params.playerChannelId;
		spawnParams.hide = true;
		spawnParams.spawnDelay = 1.0f;
		spawnParams.saveParams = false;
		spawnParams.vanilla.bStaticEntityId = false; // true - вылетает в редакторе и медленно работает O(n), false O(1)
		spawnParams.vanilla.bIgnoreLock = false; // spawn lock игнор

		//auto pPlayer = TOS_GET_ACTOR_CHANNELID(params.playerChannelId);
		//if (pPlayer)
		//	spawnParams.authorityPlayerName = pPlayer->GetEntity()->GetName();

		const string* const psClassName = &params.className;
		IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(psClassName->c_str());

		const string name = string("zeus_") + psClassName->c_str();
		spawnParams.vanilla.sName = name;
		spawnParams.vanilla.pClass = pClass;
		spawnParams.vanilla.vPosition = params.pos;

		const auto pArchetype = gEnv->pEntitySystem->LoadEntityArchetype(psClassName->c_str());
		if (pArchetype)
			spawnParams.vanilla.pArchetype = pArchetype;

		if (!pClass && !pArchetype)
		{
			CryLogError("[Zeus] not defined entity class '%s'", psClassName->c_str());
			return true;
		}

		bool bSpawned = tos::entity::SpawnDelay(spawnParams, true);
		if (!bSpawned)
		{
			CryLogError("[Zeus] entity with class '%s' copy failed!", psClassName->c_str());
			return true;
		}
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSZeusSynchronizer, SvRequestVehicleEnter)
{
	IVehicle* pVehicle = TOS_GET_VEHICLE(params.vehicleId);
	IActor* pActor = TOS_GET_ACTOR(params.actorId);
											
	if (!pVehicle || !pActor)
		return true;

	tos::vehicle::Enter(pActor, pVehicle, params.fast);

	return true;
}