/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once
#include <IGame.h>

class CScriptBind_ItemSystem
{
public:
	CScriptBind_ItemSystem(ISystem* pSystem, IGameFramework* pGameFramework);
	virtual ~CScriptBind_ItemSystem();

	void RegisterMethods();


	// Methods
	int GiveItem(IFunctionHandler* pH, const char* itemName);
	int GiveItemPack(IFunctionHandler* pH, ScriptHandle actorId, const char* packName);

private:
	ISystem* m_pSystem;
	IGameFramework* m_pGF;
	IScriptSystem* m_pSS;
};