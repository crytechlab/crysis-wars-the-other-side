/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once

#include "Game.h"
#include "ITOSGameModule.h"
#include "GenericSynchronizer.h"
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
    void        GetMemoryStatistics(ICrySizer* s) override;
    void        Init() override;
    int         GetDebugLog() const override { return m_debugLogMode; }
    //~ITOSGameModule

    virtual CTOSGenericSynchronizer* GetSynchronizer() const;

protected:
    void RegisterSynchronizer(CTOSGenericSynchronizer* pSynch);
    CTOSGenericSynchronizer* m_pSynchonizer;
    int m_debugLogMode; // режим отладки модуля (1 - вкл, 0 - выкл)
};