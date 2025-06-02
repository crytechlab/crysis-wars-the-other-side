/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#include <StdAfx.h>
#include <ISystem.h>
#include <IGameFramework.h>
#include <IItemSystem.h>
#include "TheOtherSideMP/Actors/TOSActor.h"
#include "TheOtherSideMP/Helpers/TOS_Script.h"
#include "TheOtherSideMP/Helpers/TOS_Entity.h"
#include "ScriptBind_ItemSystem.h"

CScriptBind_ItemSystem::CScriptBind_ItemSystem(ISystem* pSystem, IGameFramework* pGameFramework)
{
	m_pSS = pSystem->GetIScriptSystem();
	RegisterMethods();
}

CScriptBind_ItemSystem::~CScriptBind_ItemSystem()
{
}

void CScriptBind_ItemSystem::RegisterMethods()
{
# define REGISTER_TEMPLATE(sGlobalName, sFuncName, sFuncParams)\
	tos::script::RegisterTemplateFunction(sGlobalName, #sFuncName, sFuncParams, *this, &CScriptBind_ItemSystem::sFuncName)

	// REGISTER_TEMPLATE("System", TOSSpawnEntity, "params");
	REGISTER_TEMPLATE("ItemSystem", GiveItem, "itemName");
	REGISTER_TEMPLATE("ItemSystem", GiveItemPack, "actorId, packName");
}

//------------------------------------------------------------------------
int CScriptBind_ItemSystem::GiveItem(IFunctionHandler* pH, const char* itemName)
{
	EntityId actorId = 0;
	if (pH->GetParamType(2) == svtPointer)
	{
		ScriptHandle sh;
		pH->GetParam(2, sh);
		actorId = (EntityId)sh.n;
	}
	else if (pH->GetParamType(2) == svtString)
	{
		const char* name = 0;
		pH->GetParam(2, name);
		if (name)
		{
			IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(name);
			actorId = pEntity->GetId();
		}
	}

	bool sound = true;
	if (pH->GetParamCount() > 2 && (pH->GetParamType(3) == svtNumber || pH->GetParamType(3) == svtBool))
		pH->GetParam(3, sound);

	bool selectItem = true;
	if (pH->GetParamCount() > 3 && (pH->GetParamType(4) == svtBool))
	{
		pH->GetParam(4, selectItem);
	}

	auto pGameFramework = g_pGame->GetIGameFramework();

	CTOSActor* pActor = 0;
	if (actorId)
		pActor = static_cast<CTOSActor*>(pGameFramework->GetIActorSystem()->GetActor(actorId));
	else
		pActor = static_cast<CTOSActor*>(pGameFramework->GetClientActor());

	if (pActor)
	{
		// Зевс не должен получать оружие
		if (pActor->IsZeus())
			return pH->EndFunction();

		ScriptHandle result(pGameFramework->GetIItemSystem()->GiveItem(pActor, itemName, sound, selectItem, true));
		if (result.n)
			return pH->EndFunction(result);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_ItemSystem::GiveItemPack(IFunctionHandler* pH, ScriptHandle actorId, const char* packName)
{
	bool bKeepOld = false;
	bool bSelectPrimary = false;
	if (pH->GetParamCount() > 2 && (pH->GetParamType(3) == svtNumber || pH->GetParamType(3) == svtBool))
		pH->GetParam(3, bKeepOld);
	if (pH->GetParamCount() > 3 && (pH->GetParamType(4) == svtNumber || pH->GetParamType(4) == svtBool))
		pH->GetParam(4, bSelectPrimary);

	auto pGameFramework = g_pGame->GetIGameFramework();

	bool bOK = false;
	CTOSActor* pActor = static_cast<CTOSActor*>(pGameFramework->GetIActorSystem()->GetActor((EntityId)actorId.n));
	if (pActor)
	{
		// Зевс не должен получать оружие
		if (pActor->IsZeus())
			bOK = false;
		else
			bOK = pGameFramework->GetIItemSystem()->GetIEquipmentManager()->GiveEquipmentPack(pActor, packName, bKeepOld, bSelectPrimary);
	}

	return pH->EndFunction(bOK);
}