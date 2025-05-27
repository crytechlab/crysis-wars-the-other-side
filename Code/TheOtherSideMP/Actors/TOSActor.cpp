/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

// ReSharper disable CppInconsistentNaming
#include "StdAfx.h"
#include "TOSActor.h"

#include "Actor.h"
//#include "OffHand.h"

//#include "Aliens/TOSTrooper.h"

#include "Fists.h"
#include "GameRules.h"
#include "NetInputChainDebug.h"

#include "TheOtherSideMP/Extensions/EnergyСonsumer.h"
#include "TheOtherSideMP/Game/TOSGameEventRecorder.h"
#include "TheOtherSideMP/Game/Modules/Master/MasterClient.h"
#include "TheOtherSideMP/Game/Modules/Master/MasterModule.h"
#include "TheOtherSideMP/Helpers/TOS_Inventory.h"
#include "TheOtherSideMP/Helpers/TOS_NET.h"

#include "HUD/HUD.h"
#include "HUD/HUDRadar.h"
#include "HUD/HUDTagNames.h"

#include "TheOtherSideMP/Control/ControlSystem.h"
#include <stdexcept>
#include <TheOtherSideMP/Helpers/TOS_AI.h>
#include <TheOtherSideMP/Helpers/TOS_Entity.h>
#include <TheOtherSideMP/Helpers/TOS_Script.h>

CTOSActor::CTOSActor()
	:
	//m_filteredDeltaMovement(ZERO),
	m_isEntityHidden(false),
	m_isSlave(false),
	m_isMaster(false),
	m_isZeus(false),
	m_chargingJump(false),
	m_lastShooterId(0),
	m_pEnergyConsumer(nullptr)
{
	
}

CTOSActor::~CTOSActor() {};

bool CTOSActor::Init(IGameObject* pGameObject)
{
	TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_ActorInit, "", true));

	if (!CActor::Init(pGameObject))
		return false;

	m_debugName = GetEntity()->GetName();

	auto pExtension = GetGameObject()->AcquireExtension("TOSEnergyConsumer");
	m_pEnergyConsumer = static_cast<CTOSEnergyConsumer*>(pExtension);
	m_pEnergyConsumer->Reset();

	return true;
}

void CTOSActor::PostInit(IGameObject* pGameObject)
{
	//CryLogAlways("<C++>[%s][%s][CTOSActor::PostInit] Actor: %s|%i",
	//	tos::debug::GetEnv(), tos::debug::GetAct(1), GetEntity()->GetName(), GetEntity()->GetId());

	TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_ActorPostInit, "", true));

	CActor::PostInit(pGameObject);

	//Crysis Co-op
	pGameObject->SetAIActivation(eGOAIAM_Always);
	//~Crysis Co-op

	m_netBodyInfo.Reset();
	m_slaveStats = STOSSlaveStats();
	m_debugName = GetEntity()->GetName();

	// Факт: если оружие выдаётся на сервере, оно выдаётся и на всех клиентах тоже.
	//ResetActorWeapons(1000);

	// 30.11.2023 Akeeper: Это я оставлю здесь на всякий случай.
	// Но к сожалению это не позволяет включить PrePhysicsUpdate в одиночной игре 
	// отравки запроса на движение в MasterClient. 
	GetGameObject()->EnablePrePhysicsUpdate(ePPU_Always);

	if (ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0))
		pCharacter->SetFlags(pCharacter->GetFlags() | CS_FLAG_UPDATE_ALWAYS);

	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));
	if (pRenderProxy)
		pRenderProxy->UpdateCharactersBeforePhysics(true);

	// сохранение и применение модели персонажа
	if (gEnv->bServer)
	{
		const char* model = 0;
		tos::script::GetEntityProperty(GetEntity(), "fileModel", model);
		m_modelFilename = model;
	}
	else
	{
		if (m_modelFilename.length() > 0)
		{
			// предполагаем, что при спавне движок уже загрузил m_modelFilename из SpawnInfo
			tos::script::SetEntityProperty(GetEntity(), "fileModel", m_modelFilename.c_str());
			CActor::Physicalize();  // пересоздать физику под новую модель
		}
	}
}

void CTOSActor::InitClient(const int channelId)
{
	//CryLogAlways("<C++>[%s][%s][CTOSActor::InitClient] Actor: %s|%i|ch:%i",
	//	tos::debug::GetEnv(), tos::debug::GetAct(1), GetEntity()->GetName(), GetEntity()->GetId(), channelId);

	TOS_RECORD_EVENT(GetEntityId(), 
		STOSGameEvent(eEGE_InitClient, "", true, false, nullptr, 0.0f, channelId));

	CActor::InitClient(channelId);
}

void CTOSActor::PostInitClient(const int channelId)
{
	CActor::PostInitClient(channelId);

	//if (gEnv->bMultiplayer && !IsPlayer())
	//{
	//	GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_STATIC);
	//}

	// Для обновления состояния во время подключения клиента
	GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_DYNAMIC |
		tos::net::SERVER_ASPECT_STATIC |
		tos::net::CLIENT_ASPECT_DYNAMIC |
		tos::net::CLIENT_ASPECT_STATIC
	);
}

void CTOSActor::ProcessEvent(SEntityEvent& event)
{
	CActor::ProcessEvent(event);

	const auto pMC = g_pTOSGame->GetMasterModule()->GetMasterClient();
	if (pMC)
		pMC->OnEntityEvent(GetEntity(), event);

	if (gEnv->bEditor)
		return;

	switch (event.event)
	{
	case ENTITY_EVENT_HIDE:
	{
		if (!IsPlayer() && gEnv->bServer)
		{
			GetInventory()->Clear();
			m_isEntityHidden = true;
			GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
		}

		break;
	}
	case ENTITY_EVENT_UNHIDE:
	{
		if (!IsPlayer() && gEnv->bServer)
		{
			GetEntity()->SetTimer(eMPTIMER_GIVEWEAPONDELAY, 1000);
			m_isEntityHidden = false;
			GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
		}

		break;
	}
	case ENTITY_EVENT_START_LEVEL:
	{
		if (!IsPlayer() && gEnv->bServer)
		{
			GetEntity()->SetTimer(eMPTIMER_GIVEWEAPONDELAY, 1000);
			m_isEntityHidden = false;
			GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
		}

		break;
	}
	case ENTITY_EVENT_TIMER:
	{
		// Фикс бага #29
		//if (event.nParam[0] == eMPTIMER_REMOVEWEAPONSDELAY)
		//{
		//	const auto pInventory = GetInventory();
		//	if (pInventory)
		//	{
		//		pInventory->HolsterItem(true);
		//		pInventory->RemoveAllItems();
		//		pInventory->Clear();
		//	}
		//}
		if (event.nParam[0] == eMPTIMER_GIVEWEAPONDELAY)
		{
			IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
			SmartScriptTable props;
			if (pScriptTable->GetValue("Properties", props))
			{
				auto pEquipManager = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetIEquipmentManager();
				char* equip;
				if (pEquipManager && props->GetValue("equip_EquipmentPack", equip))			
				{
					pEquipManager->GiveEquipmentPack(this, equip, true, false);

					if (!IsPlayer() && gEnv->bServer)
					{
						GetEntity()->SetTimer(eMPTIMER_SELECTPRIMARY, 300);
					}
				}
			}
		}
		else if (event.nParam[0] == eMPTIMER_SELECTPRIMARY)
		{
			tos::inventory::SelectPrimary(this);
			tos::ai::SetStance(this->GetEntity()->GetAI(), EStance::STANCE_STAND);
		}
		//else if (event.nParam[0] == eMPTIMER_RAGDOLL)
		//{
		//	RagDollize(false);
		//	pe_action_impulse imp;
		//	imp.impulse = Vec3(1, 1, 1);

		//	GetEntity()->GetPhysics()->Action(&imp);
		//}
	}
	default: 
		break;
	}
}

// ReSharper disable once CppParameterMayBeConst
bool CTOSActor::NetSerialize(TSerialize ser, const EEntityAspects aspect, const uint8 profile, const int flags)
{
	if (!CActor::NetSerialize(ser,aspect,profile,flags))
		return false;

	if (aspect == tos::net::SERVER_ASPECT_STATIC)
	{
		// Персонаж мастера всегда должен быть невидим
		ser.Value("is_master", m_isMaster, 'bool');
		ser.Value("is_slave", m_isSlave, 'bool');
		ser.Value("is_zeus", m_isZeus, 'bool');

		if (ser.IsReading())
		{
			HideMe(m_isMaster || m_isZeus);

			if (m_isZeus)
				RemoveAllItems();
		}
	}

	if (!IsPlayer())
	{
		if (aspect == tos::net::SERVER_ASPECT_STATIC)
		{
			ser.Value("bHide", m_isEntityHidden, 'bool');

			if (ser.IsReading())
			{
				GetEntity()->Hide(m_isEntityHidden);
				HideMe(m_isEntityHidden);
			}

		}

		if (aspect == tos::net::CLIENT_ASPECT_STATIC ||
			aspect == tos::net::SERVER_ASPECT_STATIC)
		{
			// Current Weapon Serialize
			const bool writing = ser.IsWriting();
			bool	   hasWeapon = false;

			if (writing)
				hasWeapon = NetGetCurrentItem() != 0;

			ser.Value("hasWeapon", hasWeapon, 'bool');
			ser.Value("currentItemId",
				static_cast<CActor*>(this),
				&CActor::NetGetCurrentItem,
				&CActor::NetSetCurrentItem,
				'eid');

			if (!writing && hasWeapon && NetGetCurrentItem() == 0)
			{
				ser.FlagPartialRead();
			}
		}

	}

	return true;
}

void CTOSActor::SelectNextItem(const int direction, const bool keepHistory, const char* category)
{
	CActor::SelectNextItem(direction, keepHistory, category);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_STATIC);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
	}
}

void CTOSActor::HolsterItem(const bool holster)
{
	CActor::HolsterItem(holster);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_STATIC);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
	}
}

void CTOSActor::SelectLastItem(const bool keepHistory, const bool forceNext /* = false */)
{
	CActor::SelectLastItem(keepHistory, forceNext);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_STATIC);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
	}

}

void CTOSActor::SelectItemByName(const char* name, const bool keepHistory)
{
	CActor::SelectItemByName(name, keepHistory);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_STATIC);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
	}

}

void CTOSActor::SelectItem(const EntityId itemId, const bool keepHistory)
{
	CActor::SelectItem(itemId, keepHistory);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_STATIC);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
	}
}


void CTOSActor::Update(SEntityUpdateContext& ctx, const int updateSlot)
{
	CActor::Update(ctx, updateSlot);

	//Отладка потребителя энергии в виде вывода инф. на экран
	if (gEnv->bClient && IsClient())
	{
		const char* debugName = CTOSEnergyConsumer::s_debugEntityName;
		const auto pDebugEntity = gEnv->pEntitySystem->FindEntityByName(debugName);
		if (pDebugEntity)
		{
			const auto pDebugActor = static_cast<CTOSActor*>(TOS_GET_ACTOR(pDebugEntity->GetId()));
			if (pDebugActor)
			{
				const auto pEnergyConsumer = pDebugActor->GetEnergyConsumer();
				const float energy    = pEnergyConsumer->GetEnergy();
				const float maxEnergy = pEnergyConsumer->GetMaxEnergy();
				const float drain	  = pEnergyConsumer->GetDrainValue();
				const bool  updating  = pEnergyConsumer->IsUpdating();

				DRAW_2D_TEXT(40, 200, 1.3f, "--- Energy Consumer (%s) ---", 
					pDebugEntity->GetName());
				DRAW_2D_TEXT(40, 215, 1.3f, "Updating:   %i", updating);
				DRAW_2D_TEXT(40, 230, 1.3f, "Energy:     %1.f", energy);
				DRAW_2D_TEXT(40, 245, 1.3f, "MaxEnergy:  %1.f", maxEnergy);
				DRAW_2D_TEXT(40, 260, 1.3f, "DrainValue: %1.f", drain);
			}
		}
	}

	NETINPUT_TRACE(GetEntityId(), m_isMaster);
	NETINPUT_TRACE(GetEntityId(), m_isSlave);
	NETINPUT_TRACE(GetEntityId(), m_isZeus);
}

void CTOSActor::Release()
{
	//CryLogAlways("<C++>[%s][%s][CTOSActor::Release] Actor: %s|%i",
	//	tos::debug::GetEnv(), tos::debug::GetAct(1), GetEntity()->GetName(), GetEntity()->GetId());

	TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_ActorRelease, m_debugName, true));

	CActor::Release();
}

void CTOSActor::Revive(const bool fromInit)
{
	CActor::Revive(fromInit);

	if (m_isMaster)
	{
		// Скрыть актера, если он мастер

		if (gEnv->bClient)
		{
			// GetGameObject()->InvokeRMI(SvRequestHideMe(), NetHideMeParams(true), eRMI_ToServer);
		}
		else if (gEnv->bServer)
		{
			const auto* pFists = static_cast<CFists*>(GetItemByClass(CItem::sFistsClass));
			if (pFists)
				g_pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(this, pFists->GetEntityId());

			// GetGameObject()->InvokeRMI(ClMarkHideMe(), NetHideMeParams(true), eRMI_ToAllClients);
		}

		if (IsClient())
		{
			SetAngles(Ang3(m_lastSpawnPointRotation));
		}

		// Не уверен что это на что-то влияет
		SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();
		params.flags &= ~eACF_EnableMovementProcessing;
		params.flags |= eACF_NoLMErrorCorrection;

		m_pAnimatedCharacter->SetParams(params);

		// Фиксит "физичность" в режиме зрителя после контроля раба
		if (ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0))
			pCharacter->GetISkeletonPose()->DestroyCharacterPhysics(1);

		m_pAnimatedCharacter->ForceRefreshPhysicalColliderMode();
		m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Spectator, eColliderModeLayer_Game, "Actor::SetAspectProfile");
	}
	else
	{
		if (IsPlayer())
			tos::ai::SendEvent(GetEntity()->GetAI(), AIEVENT_ENABLE);
	}

	SelectLastItem(true, true); 

	TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_ActorRevived, "", true));
}

void CTOSActor::Kill()
{
	CActor::Kill();

	if (IsPlayer())
		tos::ai::SendEvent(GetEntity()->GetAI(), AIEVENT_DISABLE);

	// Вызывается только на сервере
	TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_ActorDead, "", true));
}

void CTOSActor::AnimationEvent(ICharacterInstance* pCharacter, const AnimEventInstance& event)
{
	if (!pCharacter)
		return;

	const auto pMC = g_pTOSGame->GetMasterModule()->GetMasterClient();
	if (pMC)
		pMC->AnimationEvent(GetEntity(), pCharacter, event);

	CActor::AnimationEvent(pCharacter, event);
}

void CTOSActor::NetKill(EntityId shooterId, uint16 weaponClassId, int damage, int material, int hit_type, int killerHealthOnKill)
{
	static char weaponClassName[129] = { 0 };
	m_pGameFramework->GetNetworkSafeClassName(weaponClassName, 128, weaponClassId);

	g_pGame->GetGameRules()->OnKill(this, shooterId, weaponClassName, damage, material, hit_type);

	m_netLastSelectablePickedUp = 0;

	if (GetHealth() > 0)
		SetHealth(0);

	Kill();

	g_pGame->GetGameRules()->OnKillMessage(GetEntityId(), shooterId, weaponClassName, damage, material, hit_type);

	CHUD* pHUD = g_pGame->GetHUD();
	if (!pHUD)
		return;

	bool ranked = pHUD->GetPlayerRank(shooterId) != 0 || pHUD->GetPlayerRank(GetEntityId()) != 0;

	if (IsClient() && gEnv->bMultiplayer && shooterId != GetEntityId() && g_pGameCVars->g_deathCam != 0)
	{
		// use the spectator target to store who killed us (used for the MP death cam - not quite spectator mode but similar...).
		if (g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId))
		{
			SetSpectatorTarget(shooterId);
			SetSpectatorHealth(killerHealthOnKill);

			// Also display the name of the enemy who shot you...
			if (g_pGame->GetGameRules()->GetTeam(shooterId) != g_pGame->GetGameRules()->GetTeam(GetEntityId()) || g_pGame->GetGameRules()->GetTeamCount() <= 1)
				SAFE_HUD_FUNC(GetTagNames()->AddEnemyTagName(shooterId));

			// ensure full body is displayed (otherwise player is headless)
			if (!IsThirdPerson())
				ToggleThirdPerson();
		}
	}
}

void CTOSActor::NetReviveAt(const Vec3& pos, const Quat& rot, int teamId)
{
	if (IVehicle* pVehicle = GetLinkedVehicle())
	{
		if (IVehicleSeat* pSeat = pVehicle->GetSeatForPassenger(GetEntityId()))
			pSeat->Exit(false);
	}

	// stop using any mounted weapons before reviving
	CItem* pItem = static_cast<CItem*>(GetCurrentItem());
	if (pItem)
	{
		if (pItem->IsMounted())
		{
			pItem->StopUse(GetEntityId());
			pItem = 0;
		}
	}

	SetHealth(GetMaxHealth());

	m_teamId = teamId;
	g_pGame->GetGameRules()->OnRevive(this, pos, rot, m_teamId);

	Revive();

	// Мастер привязан координатно к рабу, поэтому менять позицию ему не требуется
	if (!m_isMaster)
	{
		GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1, 1, 1), rot, pos));
	}
	// Фиксит положение камеры игрока после воскрешения после смерти
	GetEntity()->SetRotation(rot);

	// This will cover the case when the ClPickup RMI comes in before we're revived
	{
		if (m_netLastSelectablePickedUp)
			pItem = static_cast<CItem*>(m_pItemSystem->GetItem(m_netLastSelectablePickedUp));
		
		m_netLastSelectablePickedUp = 0;

		if (pItem)
		{
			bool soundEnabled = pItem->IsSoundEnabled();
			pItem->EnableSound(false);
			pItem->Select(false);
			pItem->EnableSound(soundEnabled);

			m_pItemSystem->SetActorItem(this, (EntityId)0);
			SelectItem(pItem->GetEntityId(), true);
		}
	}

	//TheOtherSide IsLocalSlave
	if (IsClient() || IsLocalSlave())
	{
		SupressViewBlending(); // no view blending when respawning // CActor::Revive resets it.
		if (g_pGame->GetHUD())
			g_pGame->GetHUD()->GetRadar()->Reset();
	}

	UpdateLastMPSpawnPointRotation(rot);
	//~TheOtherSide
}

void CTOSActor::NetReviveInVehicle(EntityId vehicleId, int seatId, int teamId)
{
	// stop using any mounted weapons before reviving
	CItem* pItem = static_cast<CItem*>(GetCurrentItem());
	if (pItem)
	{
		if (pItem->IsMounted())
		{
			pItem->StopUse(GetEntityId());
			pItem = 0;
		}
	}

	SetHealth(GetMaxHealth());

	m_teamId = teamId;
	g_pGame->GetGameRules()->OnReviveInVehicle(this, vehicleId, seatId, m_teamId);

	Revive();

	// fix our physicalization, since it's need for some vehicle stuff, and it will be set correctly before the end of the frame
	// make sure we are alive, for when we transition from ragdoll to linked...
	if (!GetEntity()->GetPhysics() || GetEntity()->GetPhysics()->GetType() != PE_LIVING)
		Physicalize();

	IVehicle* pVehicle = m_pGameFramework->GetIVehicleSystem()->GetVehicle(vehicleId);
	assert(pVehicle);
	if (pVehicle)
	{
		IVehicleSeat* pSeat = pVehicle->GetSeatById(seatId);
		if (pSeat && (!pSeat->GetPassenger() || pSeat->GetPassenger() == GetEntityId()))
			pSeat->Enter(GetEntityId(), false);
	}

	// This will cover the case when the ClPickup RMI comes in before we're revived
	if (m_netLastSelectablePickedUp)
		pItem = static_cast<CItem*>(m_pItemSystem->GetItem(m_netLastSelectablePickedUp));
	m_netLastSelectablePickedUp = 0;

	if (pItem)
	{
		bool soundEnabled = pItem->IsSoundEnabled();
		pItem->EnableSound(false);
		pItem->Select(false);
		pItem->EnableSound(soundEnabled);

		m_pItemSystem->SetActorItem(this, (EntityId)0);
		SelectItem(pItem->GetEntityId(), true);
	}

	if (IsClient())
	{
		SupressViewBlending(); // no view bleding when respawning // CActor::Revive resets it.
		if (g_pGame->GetHUD())
			g_pGame->GetHUD()->GetRadar()->Reset();
	}
}

void CTOSActor::NetSimpleKill()
{
	if (GetHealth() > 0)
		SetHealth(0);

	Kill();
}

void CTOSActor::SerializeSpawnInfo(TSerialize ser)
{
	CActor::SerializeSpawnInfo(ser);

	string model;
	ser.Value("modelFilename", model, 'stab');
	m_modelFilename = model;

	// Клиент: таблица lua здесь ещё не создана
}

ISerializableInfoPtr CTOSActor::GetSpawnInfo()
{
	struct SInfo : public ISerializableInfo
	{
		int teamId;
		string modelFilename;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("teamId", teamId, 'team');
			ser.Value("modelFilename", modelFilename, 'stab');
		}
	};

	SInfo* p = new SInfo();

	CGameRules* pGameRules = g_pGame->GetGameRules();
	p->teamId = pGameRules ? pGameRules->GetTeam(GetEntityId()) : 0;

	const char* model = 0;
	tos::script::GetEntityProperty(GetEntity(), "fileModel", model);
	p->modelFilename = m_modelFilename = model;

	return p;
}

//bool CTOSActor::ResetActorWeapons(int delayMilliseconds)
//{
//	if (gEnv->bServer && gEnv->bMultiplayer && !IsPlayer())
//	{
//		GetEntity()->SetTimer(eMPTIMER_REMOVEWEAPONSDELAY, delayMilliseconds);
//		return true;
//	}
//
//	return false;
//}

bool CTOSActor::ShouldUsePhysicsMovement()
{
	//swimming use physics, for everyone
	//if (m_stats.inWaterTimer > 0.01f)
	//	return true;

	if (GetActorStats()->inAir > 0.01f || InZeroG())
		return true;

	//players
	if (IsPlayer() || IsSlave())
	{
		//the client will be use physics always but when in thirdperson
		if (IsClient())
		{
			if (!IsThirdPerson()/* || m_stats.inAir>0.01f*/)
				return true;
			else
				return false;
		}

		//other clients will use physics always
		return true;
	}


	//in demo playback - use physics for recorded entities
	if (IsDemoPlayback())
		return true;

	//AIs in normal conditions are supposed to use LM
	return false;
}

bool CTOSActor::ApplyActions(int actions)
{
	throw std::logic_error("функция должна быть переопределена в дочерних классах");
	return false;
}

void CTOSActor::RemoveAllItems()
{
	auto pInv = GetInventory();
	if (pInv)
	{
		g_pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(this, EntityId(0));
		// pInv->HolsterItem(true); //FIXME 04.12.2024 возможно это причина того, почему у зевса видно пистолет в кобуре
		pInv->RemoveAllItems();
	}

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_STATIC);
	}
}

//void CTOSActor::QueueAnimationEvent(const SQueuedAnimEvent& sEvent)
//{
//	if (!gEnv->bServer || gEnv->bEditor)
//		return;
//
//	//if (CCoopSystem::GetInstance()->GetDebugLog() > 1)
//	CryLogAlways("[%s] Animation Event Queued %s", __FUNCTION__, sEvent.sAnimEventName.c_str());
//
//	m_AnimEventQueue.push_back(sEvent);
//}

//void CTOSActor::UpdateAnimEvents(const float fFrameTime)
//{
//	if (!gEnv->bServer)
//		return;
//
//	for (auto iterator = m_AnimEventQueue.begin(); iterator != m_AnimEventQueue.end(); ++iterator)
//	{
//		SQueuedAnimEvent& animEvent = (*iterator);
//
//		animEvent.fElapsed += fFrameTime;
//
//		if (animEvent.fElapsed > animEvent.fEventTime)
//		{
//			//this->CreateScriptEvent("animationevent", 0.f, animEvent.sAnimEventName);
//
//			AnimEventInstance sEvent;
//			sEvent.m_EventName = animEvent.sAnimEventName;
//
//			this->AnimationEvent(GetEntity()->GetCharacter(0), sEvent);
//
//			m_AnimEventQueue.erase(iterator);
//
//			//if (CCoopSystem::GetInstance()->GetDebugLog() > 1)
//			CryLogAlways("[%s] Animation Event Played %s", __FUNCTION__, animEvent.sAnimEventName.c_str());
//
//			break;
//		}
//	}
//}

bool CTOSActor::IsLocalSlave() const
{
	const auto pMC = g_pTOSGame->GetMasterModule()->GetMasterClient();
	if (!pMC)
		return false;

	return pMC->GetSlaveEntity() == GetEntity();
}

CTOSEnergyConsumer* CTOSActor::GetEnergyConsumer() const
{
	assert(m_pEnergyConsumer);
	return m_pEnergyConsumer;
}

bool CTOSActor::UpdateLastMPSpawnPointRotation(const Quat& rotation)
{
	m_lastSpawnPointRotation = rotation;

	return true;
}

bool CTOSActor::UpdateLastShooterId(const EntityId id)
{
	m_lastShooterId = id;

	return true;
}

void CTOSActor::GiveEquipmentPack()
{
	IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
	SmartScriptTable props;
	if (pScriptTable->GetValue("Properties", props))
	{
		char* equip;
		if (props->GetValue("equip_EquipmentPack", equip))
		{
			tos::inventory::GiveEquipmentPack(this, string(equip), false);
			CryLogAlways("[CTOSActor::GiveEquipmentPack] %s acquired equipment pack %s", GetEntity()->GetName(), equip);
		}
	}
}

//void CTOSActor::NetSetActorModel(const char* model)
//{
//	m_modelFilename = model;
//	if (gEnv->bClient)
//		GetGameObject()->ChangedNetworkState(tos::net::CLIENT_ASPECT_STATIC);
//	else if (gEnv->bServer)
//		GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
//}

bool CTOSActor::HideMe(bool value)
{
	SActorStats* pActorStats = GetActorStats();
	if (pActorStats)
	{
		pActorStats->isHidden = value;

		uint32 slotFlags = GetEntity()->GetSlotFlags(0);

		if (value)
			slotFlags &= ~ENTITY_SLOT_RENDER;
		else
			slotFlags |= ENTITY_SLOT_RENDER;

		GetEntity()->SetSlotFlags(0, slotFlags);
		return true;
	}

	return false;
}

bool CTOSActor::SetMeSlave(bool value)
{
	m_isSlave = value;
	return true;
}

bool CTOSActor::SetMeMaster(bool value)
{
	m_isMaster = value;
	return true;
}

bool CTOSActor::SetMeZeus(bool value)
{
	m_isZeus = value;
	GetGameObject()->ChangedNetworkState(tos::net::SERVER_ASPECT_STATIC);
	return true;
}