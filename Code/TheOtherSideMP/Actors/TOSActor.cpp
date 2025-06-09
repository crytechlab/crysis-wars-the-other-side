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

#include "TheOtherSideMP/Extensions/EnergyManager.h"
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
	m_pEnergyManager(nullptr)
{
	
}

CTOSActor::~CTOSActor() {};

bool CTOSActor::Init(IGameObject* pGameObject)
{
	TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_ActorInit, "", true));

	if (!CActor::Init(pGameObject))
		return false;

	m_debugName = GetEntity()->GetName();

	m_pEnergyManager = static_cast<CTOSEnergyManager*>(GetGameObject()->AcquireExtension("TOSEnergyManager"));
	m_pEnergyManager->Reset();

	return true;
}

void CTOSActor::PostInit(IGameObject* pGameObject)
{
	m_debugName = GetEntity()->GetName();
	TOS_RECORD_EVENT(GetEntityId(), STOSGameEvent(eEGE_ActorPostInit, m_debugName, true));

	CActor::PostInit(pGameObject);

	//Crysis Co-op
	pGameObject->SetAIActivation(eGOAIAM_Always);
	//~Crysis Co-op

	m_netBodyInfo.Reset();
	m_slaveStats = STOSSlaveStats();

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

	if (gEnv->bClient)
	{
		if (m_modelFilename.length() > 0)
		{
			// предполагаем, что при спавне движок уже загрузил m_modelFilename из SpawnInfo
			tos::script::SetEntityProperty(GetEntity(), "fileModel", m_modelFilename.c_str());
			CActor::Physicalize();  // пересоздать физику под новую модель
		}
		if (m_soundPack.length() > 0)
		{
			tos::script::SetEntityProperty(GetEntity(), "SoundPack", m_soundPack.c_str());
		}
		if (m_equipmentPack.length() > 0)
		{
			tos::script::SetEntityProperty(GetEntity(), "equip_EquipmentPack", m_equipmentPack.c_str());
		}
	}

	CreateGrabHanlder();
}

void CTOSActor::InitClient(const int channelId)
{
	//CryLogAlways("[%s][%s][CTOSActor::InitClient] Actor: %s|%i|ch:%i",
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
	GetGameObject()->RequestRemoteUpdate(
		EEntityAspects::eEA_GameServerDynamic |
		EEntityAspects::eEA_GameServerStatic |
		EEntityAspects::eEA_GameClientDynamic |
		EEntityAspects::eEA_GameClientStatic
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
			GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
		}

		break;
	}
	case ENTITY_EVENT_UNHIDE:
	{
		if (!IsPlayer() && gEnv->bServer)
		{
			GetEntity()->SetTimer(eMPTIMER_GIVEWEAPONDELAY, 1000);
			m_isEntityHidden = false;
			GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
		}

		break;
	}
	case ENTITY_EVENT_START_LEVEL:
	{
		if (!IsPlayer() && gEnv->bServer)
		{
			GetEntity()->SetTimer(eMPTIMER_GIVEWEAPONDELAY, 1000);
			m_isEntityHidden = false;
			GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
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
			
			// TheOtherSide: исправление случая когда между удаленным сервером и клиентом
			// нет синхронизации текущего оружия
			if (gEnv->bServer && !gEnv->bClient)
			{
				GetGameObject()->InvokeRMI(ClSelectItem(), NetSelectItemParams(GetInventory()->GetCurrentItem()), eRMI_ToAllClients);
			}
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

	if (aspect == EEntityAspects::eEA_GameServerStatic)
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
		if (aspect == EEntityAspects::eEA_GameServerStatic)
		{
			ser.Value("bHide", m_isEntityHidden, 'bool');

			if (ser.IsReading())
			{
				GetEntity()->Hide(m_isEntityHidden);
				HideMe(m_isEntityHidden);
			}

		}

		if (aspect == EEntityAspects::eEA_GameServerStatic ||
			aspect == EEntityAspects::eEA_GameClientStatic)
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
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameClientStatic);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	}
}

void CTOSActor::HolsterItem(const bool holster)
{
	CActor::HolsterItem(holster);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameClientStatic);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	}
}

void CTOSActor::SelectLastItem(const bool keepHistory, const bool forceNext /* = false */)
{
	CActor::SelectLastItem(keepHistory, forceNext);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameClientStatic);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	}

}

void CTOSActor::SelectItemByName(const char* name, const bool keepHistory)
{
	CActor::SelectItemByName(name, keepHistory);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameClientStatic);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	}

}

void CTOSActor::SelectItem(const EntityId itemId, const bool keepHistory)
{
	CActor::SelectItem(itemId, keepHistory);

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameClientStatic);
	}
	else
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	}
}


void CTOSActor::Update(SEntityUpdateContext& ctx, const int updateSlot)
{
	CActor::Update(ctx, updateSlot);

	NETINPUT_TRACE(GetEntityId(), m_isMaster);
	NETINPUT_TRACE(GetEntityId(), m_isSlave);
	NETINPUT_TRACE(GetEntityId(), m_isZeus);

	bool haveRightHandAttachment = false;  
	string rightHandAttachedName;

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	if (pCharacter)
	{
		IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
		IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName("right_item_attachment");
		haveRightHandAttachment = pAttachment != nullptr;
		if (pAttachment)
		{
			auto pEntityAttachment = static_cast<CEntityAttachment*>(pAttachment->GetIAttachmentObject());
			if (pEntityAttachment)
			{
				auto pEntity = TOS_GET_ENTITY(pEntityAttachment->GetEntityId());
				rightHandAttachedName = pEntity ? string(pEntity->GetName()) : "NULL";
			}
		}
	}

	const IEntity* pNetItem = TOS_GET_ENTITY(CActor::NetGetCurrentItem());
	const string netItemName = pNetItem ? pNetItem->GetName() : "";
	const EntityId netItemId = pNetItem ? pNetItem->GetId() : 0;

	NETINPUT_TRACE(GetEntityId(), netItemId);
	NETINPUT_TRACE(GetEntityId(), netItemName.c_str());
	NETINPUT_TRACE(GetEntityId(), haveRightHandAttachment);
	NETINPUT_TRACE(GetEntityId(), rightHandAttachedName.c_str()); 
}

void CTOSActor::Release()
{
	//CryLogAlways("[%s][%s][CTOSActor::Release] Actor: %s|%i",
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
	string soundPack;
	string equipmentPack;

	ser.Value("modelFilename", model, 'stab');
	ser.Value("soundPack", soundPack, 'stab');
	ser.Value("equipmentPack", equipmentPack, 'stab');

	m_modelFilename = model;
	m_soundPack = soundPack;
	m_equipmentPack = equipmentPack;

	// Клиент: таблица lua здесь ещё не создана
}

ISerializableInfoPtr CTOSActor::GetSpawnInfo()
{
	struct SInfo : public ISerializableInfo
	{
		int teamId;
		string modelFilename;
		string soundPack;
		string equipmentPack;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("teamId", teamId, 'team');
			ser.Value("modelFilename", modelFilename, 'stab');
			ser.Value("soundPack", soundPack, 'stab');
			ser.Value("equipmentPack", equipmentPack, 'stab');
		}
	};

	SInfo* p = new SInfo();

	CGameRules* pGameRules = g_pGame->GetGameRules();
	p->teamId = pGameRules ? pGameRules->GetTeam(GetEntityId()) : 0;

	const char* model = 0;
	const char* soundPack = 0;
	const char* equipmentPack = 0;

	tos::script::GetEntityProperty(GetEntity(), "fileModel", model);
	tos::script::GetEntityProperty(GetEntity(), "SoundPack", soundPack);
	tos::script::GetEntityProperty(GetEntity(), "equip_EquipmentPack", equipmentPack);

	p->modelFilename = m_modelFilename = model;
	p->soundPack = soundPack;
	p->equipmentPack = equipmentPack;

	return p;
}

bool CTOSActor::CanPickUpObject(IEntity *obj, float &heavyness, float &volume)
{
	// Зевс не может поднимать объекты
	if (IsZeus())
	{
		CryLogWarning("[CTOSActor::CanPickUpObject] Zeus cannot pick up objects");
		return false;
	}

	return CActor::CanPickUpObject(obj, heavyness, volume);
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
		// pInv->HolsterItem(true); 
		CRY_FIXME(9,6,2025,"возможно это причина того, почему у зевса видно пистолет в кобуре");
		pInv->RemoveAllItems();
	}

	if (gEnv->bClient)
	{
		GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameClientStatic);
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

CTOSEnergyManager* CTOSActor::GetEnergyManager() const
{
	assert(m_pEnergyManager);
	return m_pEnergyManager;
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
	GetGameObject()->ChangedNetworkState(EEntityAspects::eEA_GameServerStatic);
	return true;
}