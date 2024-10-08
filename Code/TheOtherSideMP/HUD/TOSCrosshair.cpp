/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#include "StdAfx.h"
#include "TOSCrosshair.h"

CTOSHUDCrosshair::CTOSHUDCrosshair(CHUD* pHUD)
	: CHUDCrosshair(pHUD)
{
	
}

void CTOSHUDCrosshair::Update(const float fDeltaTime)
{
	// Скорее всего нужно будет убрать, если худ правильно работать не будет
	CHUDCrosshair::Update(fDeltaTime);


}

void CTOSHUDCrosshair::UpdateCrosshair()
{
	CHUDCrosshair::UpdateCrosshair();
}

void CTOSHUDCrosshair::Show(bool show)
{
	m_animCrossHair.SetVisible(show);
}

void CTOSHUDCrosshair::ShowFriendCross(const bool show)
{
	if (g_pHUD && show)
		m_animFriendCross.SetVisible(true);
	else
		m_animFriendCross.SetVisible(false);
}
