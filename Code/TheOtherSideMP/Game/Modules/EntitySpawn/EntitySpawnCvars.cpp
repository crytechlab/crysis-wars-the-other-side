/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#include "StdAfx.h"
#include "IEntity.h"
#include "IEntitySystem.h"
#include "EntitySpawnModule.h"

#include "TheOtherSideMP/Game/TOSGameCvars.h"

void CTOSEntitySpawnModule::InitCVars(IConsole* pConsole)
{
	CTOSGenericModule::InitCVars(pConsole);
}

void CTOSEntitySpawnModule::ReleaseCVars()
{
	CTOSGenericModule::ReleaseCVars();
}

void CTOSEntitySpawnModule::InitCCommands(IConsole* pConsole)
{
	CTOSGenericModule::InitCCommands(pConsole);

	pConsole->AddCommand("spawnentity", CmdSpawnEntity);
	pConsole->AddCommand("removeentity", CmdRemoveEntity);
	pConsole->AddCommand("removeentityforced", CmdRemoveEntityForced);
	pConsole->AddCommand("dumpspawned", CmdDumpSpawned);
	pConsole->AddCommand("dumpentityrotation", CmdDumpEntityRotation);
}

void CTOSEntitySpawnModule::ReleaseCCommands()
{
	CTOSGenericModule::ReleaseCCommands();

	const auto pConsole = gEnv->pConsole;

	pConsole->RemoveCommand("spawnentity");
	pConsole->RemoveCommand("removeentity");
	pConsole->RemoveCommand("removeentityforced");
	pConsole->RemoveCommand("dumpspawned");
	pConsole->RemoveCommand("dumpentityrotation");
}

void CTOSEntitySpawnModule::CmdSpawnEntity(IConsoleCmdArgs* pArgs)
{
	ONLY_SERVER_CMD;

	string arg1 = pArgs->GetArg(1);
	if (arg1.empty())
	{
		CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, "<CTOSEntitySpawnModule> Spawn failed: empty class of entity");
		return;
	}

	const auto pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(arg1);
	assert(pClass);

	string newEntName = pArgs->GetArg(2);
	if (newEntName.empty())
	{
		newEntName = "spawned";
	}

	const string plName = pArgs->GetArg(3);

	const auto pPlayerEntity = gEnv->pEntitySystem->FindEntityByName(plName.c_str());
	assert(pPlayerEntity);
	if (!pPlayerEntity)
	{
		CryLogAlways("<c++> Spawn failed: player entity %(s) not found", plName.c_str());
		return;
	}

	STOSEntitySpawnParams params;
	params.authorityPlayerName = plName;
	params.vanilla.bStaticEntityId = true;
	params.vanilla.pClass = pClass;
	params.savedName = newEntName.c_str();
	params.vanilla.vPosition = pPlayerEntity->GetWorldPos();
	params.vanilla.qRotation = pPlayerEntity->GetWorldRotation();
	params.tosFlags |= ENTITY_MUST_RECREATED;

	CTOSEntitySpawnModule::SpawnEntity(params);
}

void CTOSEntitySpawnModule::CmdRemoveEntity(IConsoleCmdArgs* pArgs)
{
	ONLY_SERVER_CMD;
	GET_ENTITY_FROM_FIRST_ARG;


	gEnv->pEntitySystem->RemoveEntity(pEntity->GetId());
}

void CTOSEntitySpawnModule::CmdRemoveEntityForced(IConsoleCmdArgs* pArgs)
{
	ONLY_SERVER_CMD;
	GET_ENTITY_FROM_FIRST_ARG;


	RemoveEntityForced(pEntity->GetId());
}

void CTOSEntitySpawnModule::CmdDumpSpawned(IConsoleCmdArgs* pArgs)
{
	ONLY_SERVER_CMD;

	const auto pModule = g_pTOSGame->GetEntitySpawnModule();
	assert(pModule);

	CryLogAlways("Result: ");
	CryLogAlways("	saved: ");

	TMapTOSParams::const_iterator it = pModule->m_savedSpawnParams.begin();
	TMapTOSParams::const_iterator end = pModule->m_savedSpawnParams.end();
	for (; it != end; ++it)
	{
		const string& savedName = it->second->savedName;
		const string& savedAuthPlayer = it->second->authorityPlayerName;
		const bool savedWillBeControlled = it->second->forceStartControl;

		CryLogAlways("		--- name: %s, authName: %s, willBeControlled: %i",
			savedName.c_str(),
			savedAuthPlayer.c_str(),
			savedWillBeControlled);
	}

	CryLogAlways("	marked for recreation: ");
	std::vector<EntityId>::const_iterator markedIt = CTOSEntitySpawnModule::s_markedForRecreation.begin();
	std::vector<EntityId>::const_iterator markedEnd = CTOSEntitySpawnModule::s_markedForRecreation.end();
	for (; markedIt != markedEnd; ++markedIt)
	{
		const EntityId markedId = *markedIt;
		const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(markedId);
		if (!pEntity)
			continue;

		const char* markedName = pEntity->GetName();

		CryLogAlways("		--- name: %s, id: %i", markedName, markedId);
	}
}

void CTOSEntitySpawnModule::CmdDumpEntityRotation(IConsoleCmdArgs* pArgs)
{
	GET_ENTITY_FROM_FIRST_ARG;


	CryLogAlways("Result: ");

	const auto angles = pEntity->GetWorldAngles();
	const auto rot = pEntity->GetWorldRotation();
	const auto inverted_rot = rot.GetInverted();
	const auto norm_rot = rot.GetNormalized();

	CryLogAlways("	pEntity->GetWorldAngles(Ang3)   = (%1.f, %1.f, %1.f)", 
		angles.x, angles.y, angles.z);

	CryLogAlways("	rot->GetWorldRotation(Quat): ");
	CryLogAlways("		GetColumn0 = (%1.f, %1.f, %1.f)", 
		rot.GetColumn0().x, rot.GetColumn0().y, rot.GetColumn0().z);
	CryLogAlways("		GetColumn1 = (%1.f, %1.f, %1.f)",
		rot.GetColumn1().x, rot.GetColumn1().y, rot.GetColumn1().z);
	CryLogAlways("		GetColumn2 = (%1.f, %1.f, %1.f)",
		rot.GetColumn2().x, rot.GetColumn2().y, rot.GetColumn2().z);
	CryLogAlways("		GetRow0 = (%1.f, %1.f, %1.f)",
		rot.GetRow0().x, rot.GetRow0().y, rot.GetRow0().z);
	CryLogAlways("		GetRow1 = (%1.f, %1.f, %1.f)",
		rot.GetRow1().x, rot.GetRow1().y, rot.GetRow1().z);
	CryLogAlways("		GetRow2 = (%1.f, %1.f, %1.f)",
		rot.GetRow2().x, rot.GetRow2().y, rot.GetRow2().z);
	CryLogAlways("		GetFwd = (%1.f, %1.f, %1.f)",
		rot.GetFwdX(), rot.GetFwdY(), rot.GetFwdZ());

	CryLogAlways(" ");

	CryLogAlways("	inverted_rot->GetWorldRotation(Quat): ");
	CryLogAlways("		GetColumn0 = (%1.f, %1.f, %1.f)",
		inverted_rot.GetColumn0().x, inverted_rot.GetColumn0().y, inverted_rot.GetColumn0().z);
	CryLogAlways("		GetColumn1 = (%1.f, %1.f, %1.f)",
		inverted_rot.GetColumn1().x, inverted_rot.GetColumn1().y, inverted_rot.GetColumn1().z);
	CryLogAlways("		GetColumn2 = (%1.f, %1.f, %1.f)",
		inverted_rot.GetColumn2().x, inverted_rot.GetColumn2().y, inverted_rot.GetColumn2().z);
	CryLogAlways("		GetRow0 = (%1.f, %1.f, %1.f)",
		inverted_rot.GetRow0().x, inverted_rot.GetRow0().y, inverted_rot.GetRow0().z);
	CryLogAlways("		GetRow1 = (%1.f, %1.f, %1.f)",
		inverted_rot.GetRow1().x, inverted_rot.GetRow1().y, inverted_rot.GetRow1().z);
	CryLogAlways("		GetRow2 = (%1.f, %1.f, %1.f)",
		inverted_rot.GetRow2().x, inverted_rot.GetRow2().y, inverted_rot.GetRow2().z);
	CryLogAlways("		GetFwd = (%1.f, %1.f, %1.f)",
		inverted_rot.GetFwdX(),   inverted_rot.GetFwdY(), inverted_rot.GetFwdZ());

	CryLogAlways(" ");

	CryLogAlways("	norm_rot->GetWorldRotation(Quat): ");
	CryLogAlways("		GetColumn0 = (%1.f, %1.f, %1.f)",
		norm_rot.GetColumn0().x, norm_rot.GetColumn0().y, norm_rot.GetColumn0().z);
	CryLogAlways("		GetColumn1 = (%1.f, %1.f, %1.f)",
		norm_rot.GetColumn1().x, norm_rot.GetColumn1().y, norm_rot.GetColumn1().z);
	CryLogAlways("		GetColumn2 = (%1.f, %1.f, %1.f)",
		norm_rot.GetColumn2().x, norm_rot.GetColumn2().y, norm_rot.GetColumn2().z);
	CryLogAlways("		GetRow0 = (%1.f, %1.f, %1.f)",
		norm_rot.GetRow0().x, norm_rot.GetRow0().y, norm_rot.GetRow0().z);
	CryLogAlways("		GetRow1 = (%1.f, %1.f, %1.f)",
		norm_rot.GetRow1().x, norm_rot.GetRow1().y, norm_rot.GetRow1().z);
	CryLogAlways("		GetRow2 = (%1.f, %1.f, %1.f)",
		norm_rot.GetRow2().x, norm_rot.GetRow2().y, norm_rot.GetRow2().z);
	CryLogAlways("		GetFwd = (%1.f, %1.f, %1.f)",
		norm_rot.GetFwdX(), norm_rot.GetFwdY(), norm_rot.GetFwdZ());
}
