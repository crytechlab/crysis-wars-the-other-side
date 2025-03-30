/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once

#include "Game.h"
#include "ITOSGameModule.h"

#include "TheOtherSideMP/Game/TOSGameEventRecorder.h"

// Akeeper 28.01.2024:
// Проблема: действия будут отпускаться 2 раза подряд. Может вызвать баги.
// Решение: если режим активации - отпустить, то вызывать другую функцию, которая будет вызывать уже эту
// с указанным временем нажатия.
#define ASSING_ACTION(pActor, actionId, activationMode, pressedDuration, checkActionId, func)\
if ( (actionId) == (checkActionId) )\
	func( (pActor), (actionId), (activationMode), value, (pressedDuration))\


class CTOSGenericModule : public ITOSGameModule // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	friend class CTOSMasterModule;

	CTOSGenericModule();
	~CTOSGenericModule();

	//ITOSGameModule
	bool        OnInputEvent(const SInputEvent& event)
	{
		return true;
	};
	bool        OnInputEventUI(const SInputEvent& event)
	{
		return false;
	};
	void        OnExtraGameplayEvent(IEntity* pEntity, const STOSGameEvent& event)
	{

	};
	void        GetMemoryStatistics(ICrySizer* s);
	const char* GetName()
	{
		return "GenericModule";
	};
	void        Init();
	void        Update(float frametime)
	{

	}
	void        Serialize(TSerialize ser)
	{

	}
	//bool		NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) ;

	int GetDebugLog()
	{
		return m_debugLogMode;
	}

	CScriptableBase* GetScriptBind()
	{
		return nullptr;
	};

	void InitScriptBinds()
	{
	};

	void InitCVars(IConsole* pConsole)
	{
	};
	void InitCCommands(IConsole* pConsole)
	{
	};
	void ReleaseCVars()
	{
	};
	void ReleaseCCommands()
	{
	};
	void ReleaseScriptBinds()
	{
	};
	//~ITOSGameModule

	virtual CTOSGenericSynchronizer* GetSynchronizer() const;

protected:
	/**
	 * \brief Регистрация синхронизатора для этого модуля.
		\n смотреть \a m_pSynchonizer
	 */
	void RegisterSynchronizer(CTOSGenericSynchronizer* pSynch);
	CTOSGenericSynchronizer* m_pSynchonizer;
	int m_debugLogMode; // режим отладки модуля (1 - вкл, 0 - выкл)

private:
};