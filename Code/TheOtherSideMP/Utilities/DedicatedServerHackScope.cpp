/*************************************************************************
Crysis Co-op Source File.
Copyright (C), Crysis Co-op
**************************************************************************/
// Created by Crysis Co-op Developers

#include <StdAfx.h>
#include "DedicatedServerHackScope.h"
#include <Cry_Camera.h>


void CDedicatedServerHackScope::Enter()
{
	bool* pDedicatedFlagAddress = &reinterpret_cast<bool*>(&gEnv->pSystem->GetViewCamera())[sizeof(CCamera) + 13];
	gEnv->bClient = true;
	*pDedicatedFlagAddress = false;
}

void CDedicatedServerHackScope::Exit()
{
	bool* pDedicatedFlagAddress = &reinterpret_cast<bool*>(&gEnv->pSystem->GetViewCamera())[sizeof(CCamera) + 13];
	*pDedicatedFlagAddress = true;
	gEnv->bClient = false;
}