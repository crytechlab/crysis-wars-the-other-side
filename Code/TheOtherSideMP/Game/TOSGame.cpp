#include "StdAfx.h"

#include "TOSGameCvars.h"
#include "TOSGameEventRecorder.h"

#include "Modules/ITOSGameModule.h"
#include "Modules/MasterSystem/MasterModule.h"

#include "TOSGame.h"
#include "Game.h"

CTOSGame::CTOSGame()
{

}

CTOSGame::~CTOSGame()
{
	SAFE_DELETE(m_pEventRecorder);
	SAFE_DELETE(m_pMasterModule);

	delete this;
}

void CTOSGame::Init()
{
	g_pGame->GetIGameFramework()->GetIGameplayRecorder()->RegisterListener(this);

	m_pEventRecorder = new CTOSGameEventRecorder();

	//Modules
	m_pMasterModule = new CTOSMasterModule();

	//~Modules

	for (ITOSGameModule* pModule : m_modules)
	{
		if (pModule)
		{
			pModule->Init();
		}
	}
}

void CTOSGame::Shutdown()
{
	g_pGame->GetIGameFramework()->GetIGameplayRecorder()->UnregisterListener(this);

	this->~CTOSGame();
}

void CTOSGame::Update(float frameTime, int frameId)
{
	for (ITOSGameModule* pModules : m_modules)
	{
		if (pModules)
		{
			pModules->Update(frameTime);
		}
	}
}

CTOSGameEventRecorder* CTOSGame::GetEventRecorder() const
{
	return m_pEventRecorder;
}

CTOSMasterModule* CTOSGame::GetMasterModule() const
{
	return m_pMasterModule;
}

bool CTOSGame::ModuleAdd(ITOSGameModule* pModule, bool flowGraph)
{
	auto& modules = flowGraph == true ? m_flowgraphModules : m_modules;

	return stl::push_back_unique(modules, pModule);
}

bool CTOSGame::ModuleRemove(ITOSGameModule* pModule, bool flowGraph)
{
	auto& modules = flowGraph == true ? m_flowgraphModules : m_modules;

	return stl::find_and_erase(modules, pModule);
}
