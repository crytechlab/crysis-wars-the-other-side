/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#include "StdAfx.h"
#include "EnergyManager.h"
#include "Game.h"
#include "GameCVars.h"

#include "TheOtherSideMP/Helpers/TOS_NET.h"

CTOSEnergyManager::CTOSEnergyManager()
	: m_energy(0),
	m_maxEnergy(0),
	m_regenStartDelay(0),
	m_regenStartDelaySP(0),
	m_regenStartDelayMP(0),
	m_regenStartDelay20Boundary(0),
	m_rechargeTimeSP(0),
	m_rechargeTimeMP(0),
	m_drainValue(0),
	m_enableUpdate(true) { }

CTOSEnergyManager::~CTOSEnergyManager() { }

bool CTOSEnergyManager::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);

	if (!GetGameObject()->BindToNetwork())
		return false;

	GetGameObject()->EnablePostUpdates(this);

	return true;
}

void CTOSEnergyManager::PostInit(IGameObject* pGameObject)
{
	Reset();

	pGameObject->EnableUpdateSlot(this, 0);
	pGameObject->SetUpdateSlotEnableCondition(this, 0, eUEC_Always);
	pGameObject->EnablePostUpdates(this);

	CryLog("[%s] Post init energy consumer with max energy %1.f",pGameObject->GetEntity()->GetName(), m_maxEnergy);
}

void CTOSEnergyManager::InitClient(int channelId) { }

void CTOSEnergyManager::PostInitClient(int channelId) { }

void CTOSEnergyManager::Release()
{
	delete this;
}

void CTOSEnergyManager::FullSerialize(TSerialize ser)
{
	ser.BeginGroup("TOSEnergyConsumer");
	ser.Value("energy", m_energy);
	ser.Value("maxEnergy", m_maxEnergy);
	ser.Value("drain", m_drainValue);
	ser.Value("regenStartDelay", m_regenStartDelay);
	ser.Value("regenStartDelayMP", m_regenStartDelayMP);
	ser.Value("regenStartDelaySP", m_regenStartDelaySP);
	ser.Value("enableUpdate", m_enableUpdate);
	ser.EndGroup();
}

bool CTOSEnergyManager::NetSerialize(TSerialize ser, const EEntityAspects aspect, uint8 profile, int flags)
{
	if (aspect == EEntityAspects::eEA_GameServerStatic)
	{
		ser.Value("energy", m_energy);
		ser.Value("maxEnergy", m_maxEnergy);
		ser.Value("drain", m_drainValue);
		ser.Value("regenStartDelay", m_regenStartDelay);
	}

	return true;
}

void CTOSEnergyManager::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	if (!m_enableUpdate)
		return;

	const float		rechargeTime = gEnv->bMultiplayer ? m_rechargeTimeMP : m_rechargeTimeSP;
	const float     frameTime    = ctx.fFrameTime;
	float           regenRate    = m_maxEnergy / max(0.01f, rechargeTime);

	if (m_drainValue > 0.0f)
		regenRate = min(regenRate - max(1.0f, m_drainValue), -max(1.0f, m_drainValue));

	if (gEnv->bServer)
	{
		if (regenRate < 0.0f || m_regenStartDelay <= 0.0f)
			SetEnergy(clamp(m_energy + regenRate * ctx.fFrameTime, 0.0f, m_maxEnergy));
	}

	if (m_regenStartDelay > 0.0f)
		m_regenStartDelay = max(0.0f, m_regenStartDelay - frameTime);
}

void CTOSEnergyManager::HandleEvent(const SGameObjectEvent&) { }

void CTOSEnergyManager::ProcessEvent(SEntityEvent&) { }

void CTOSEnergyManager::GetMemoryStatistics(ICrySizer* s)
{
	s->Add(*this);
}

bool CTOSEnergyManager::AddEnergy(const float value)
{
	if (gEnv->bServer)
	{
		SetEnergy(m_energy + value);
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	}
	else if (gEnv->bClient)
	{
		NetEnergyParams params;
		params.energy = m_energy + value;

		GetGameObject()->InvokeRMI(SvRequestSetEnergy(), params, eRMI_ToServer);
	}


	return true;
}

bool CTOSEnergyManager::SetEnergy(float value, const bool initiated)
{
	value = clamp(value, 0.0f, m_maxEnergy);

	if (gEnv->bServer)
	{
		if (value < m_energy)
		{
			m_regenStartDelay = gEnv->bMultiplayer ? m_regenStartDelayMP : m_regenStartDelaySP;

			if (!initiated)
			{
				// if we cross the 20% boundary we don't regenerate for 3secs
				if (gEnv->bMultiplayer && value / m_maxEnergy <= 0.2f && value < m_energy && g_pGameCVars->g_mpSpeedRechargeDelay)
					m_regenStartDelay = m_regenStartDelay20Boundary;
			}
		}

		m_energy = value;

		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	}
	else if (gEnv->bClient)
	{
		NetEnergyParams params;
		params.energy = value;
		params.initiated = initiated;
		params.forced = false;

		GetGameObject()->InvokeRMI(SvRequestSetEnergy(), params, eRMI_ToServer);
	}

	return true;
}

bool CTOSEnergyManager::SetEnergyForced(const float value)
{
	if (gEnv->bServer)
	{
		m_energy = value;
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	}
	else if (gEnv->bClient)
	{
		NetEnergyParams params;
		params.energy = value;
		params.initiated = false;
		params.forced = true;

		GetGameObject()->InvokeRMI(SvRequestSetEnergy(), params, eRMI_ToServer);
	}


	return true;
}

float CTOSEnergyManager::GetEnergy() const
{
	return m_energy;
}

bool CTOSEnergyManager::SetMaxEnergy(const float value)
{
	m_maxEnergy = value;
	return true;
}

float CTOSEnergyManager::GetMaxEnergy() const
{
	return m_maxEnergy;
}

bool CTOSEnergyManager::SetDrainValue(const float value)
{
	m_drainValue = value;
	return true;
}

float CTOSEnergyManager::GetDrainValue() const
{
	return m_drainValue;
}

void CTOSEnergyManager::EnableUpdate(const bool enable)
{
	m_enableUpdate = enable;
}

bool CTOSEnergyManager::IsUpdating() const
{
	return m_enableUpdate;
}

void CTOSEnergyManager::Reset()
{
	SetEnergy(DEFAULT_ENERGY);
	SetMaxEnergy(DEFAULT_ENERGY);

	m_regenStartDelayMP = 0.0f;
	m_regenStartDelaySP = 1.0f;
}

bool CTOSEnergyManager::SetRegenStartDelaySP(const float val)
{
	m_regenStartDelaySP = val;
	return true;
}

bool CTOSEnergyManager::SetRegenStartDelayMP(const float val)
{
	m_regenStartDelayMP = val;
	return true;
}

bool CTOSEnergyManager::SetRegenStartDelay20Boundary(const float val)
{
	m_regenStartDelay20Boundary = val;
	return true;
}

float CTOSEnergyManager::GetRegenStartDelay() const
{
	return m_regenStartDelay;
}

void CTOSEnergyManager::SetRechargeTimeSP(const float time)
{
	m_rechargeTimeSP = time;
}

void CTOSEnergyManager::SetRechargeTimeMP(const float time)
{
	m_rechargeTimeMP = time;
}

IMPLEMENT_RMI(CTOSEnergyManager, SvRequestSetEnergy)
{
	if (gEnv->bServer)
	{
		const float energy = params.energy;

		if (params.forced)
			SetEnergyForced(energy);
		else
			SetEnergy(energy, params.initiated);
	}

	return true;
}