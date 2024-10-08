/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once

#include "IActorSystem.h"
#include "IInput.h"
#include "TheOtherSideMP\Game\Events\ITOSGameEventListener.h"

struct STOSGameEvent;
class CTOSGenericSynchronizer;

struct ITOSGameModule : public ITOSGameEventListener // NOLINT(cppcoreguidelines-special-member-functions)
{
	//ITOSGameEventListener
	virtual void OnExtraGameplayEvent(IEntity* pEntity, const STOSGameEvent& event) {};
	//~ITOSGameEventListener

	virtual ~ITOSGameModule()
	{};
	//virtual void OnMainMenuEnter() = 0;
	//virtual void OnGameRulesReset() = 0;
	//virtual void OnActorDeath(IActor* pActor) = 0;
	//virtual void OnActorGrabbed(IActor* pActor, EntityId grabberId) = 0;
	//virtual void OnActorDropped(IActor* pActor, EntityId droppedId) = 0;
	//virtual void OnActorGrab(IActor* pActor, EntityId grabId) = 0;
	//virtual void OnActorDrop(IActor* pActor, EntityId dropId) = 0;
	//virtual void OnEnterVehicle(IActor* pActor, IVehicle* pVehicle) = 0;
	//virtual void OnExitVehicle(IActor* pActor) = 0;

	//virtual void OnStartControl(const IActor* pActor) { };
	//virtual void OnStopControl(const IActor* pActor) { };

	virtual bool OnInputEvent(const SInputEvent& event)
	{
		return true;
	};
	virtual bool OnInputEventUI(const SInputEvent& event)
	{
		return false;
	};

	virtual void GetMemoryStatistics(ICrySizer* s) = 0;
	virtual const char* GetName() = 0;
	virtual void Init() = 0;
	virtual void Update(float frametime) = 0;
	virtual void Serialize(TSerialize ser) = 0;
	//virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) = 0;
	virtual int GetDebugLog() = 0;

	virtual CScriptableBase* GetScriptBind() = 0;
	virtual void InitScriptBinds() = 0;
	virtual void InitCVars(IConsole* pConsole) = 0;
	virtual void InitCCommands(IConsole* pConsole) = 0;
	virtual void ReleaseCVars() = 0;
	virtual void ReleaseCCommands() = 0;
	virtual void ReleaseScriptBinds() = 0;

	bool operator == (ITOSGameModule* pModule)
	{
		if (!pModule)
			return false;

		return strcmp(pModule->GetName(), this->GetName()) == 0;
	}

	bool operator != (ITOSGameModule* pModule)
	{
		if (!pModule)
			return false;

		return strcmp(pModule->GetName(), this->GetName()) != 0;
	}
};