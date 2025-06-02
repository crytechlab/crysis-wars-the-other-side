/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once

#include "IActorSystem.h"
#include "IInput.h"
#include "TheOtherSideMP\Game\Events\ITOSGameEventListener.h"

struct STOSGameEvent;

struct ITOSGameModule : public ITOSGameEventListener // NOLINT(cppcoreguidelines-special-member-functions)
{
	//ITOSGameEventListener
	virtual void OnExtraGameplayEvent(IEntity* pEntity, const STOSGameEvent& event) = 0;
	//~ITOSGameEventListener

	virtual ~ITOSGameModule() = default;
	virtual bool OnInputEvent(const SInputEvent& event) = 0;
	virtual bool OnInputEventUI(const SInputEvent& event) = 0;
	virtual void GetMemoryStatistics(ICrySizer* s) = 0;
	virtual const char* GetName() const = 0;
	virtual void Init() = 0;
	virtual void Update(float frametime) = 0;
	virtual void Serialize(TSerialize ser) = 0;
	//virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) = 0;
	virtual int GetDebugLog() const = 0;

	virtual CScriptableBase* GetScriptBind() = 0;
	virtual void InitScriptBinds() = 0;
	virtual void InitCVars(IConsole* pConsole) = 0;
	virtual void InitCCommands(IConsole* pConsole) = 0;
	virtual void ReleaseCVars() = 0;
	virtual void ReleaseCCommands() = 0;
	virtual void ReleaseScriptBinds() = 0;
	virtual void DumpModuleInfo() = 0;

	bool operator == (const ITOSGameModule& other) const
	{
		return strcmp(other.GetName(), this->GetName()) == 0;
	}

	bool operator != (const ITOSGameModule& other) const
	{
		return strcmp(other.GetName(), this->GetName()) != 0;	
	}
};