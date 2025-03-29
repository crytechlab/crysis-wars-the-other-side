#include "StdAfx.h"
#include "TOSActor.h"
#include <TheOtherSideMP/Helpers/TOS_Entity.h>
#include <Fists.h>

IMPLEMENT_RMI(CTOSActor, SvRequestPlayAnimation)
{
	// Описываем здесь всё, что будет выполняться на сервере

	GetGameObject()->InvokeRMI(ClPlayAnimation(), params, eRMI_ToRemoteClients);

	return true;
}

IMPLEMENT_RMI(CTOSActor, ClPlayAnimation)
{
	// Описываем здесь всё, что будет выполняться на клиенте

	IAnimationGraphState* pGraphState = (GetAnimatedCharacter() ? GetAnimatedCharacter()->GetAnimationGraphState() : nullptr);
	string mode;

	if (pGraphState)
	{
		if (params.mode == AIANIM_SIGNAL)
		{
			mode = "Signal";
		}
		else if (params.mode == AIANIM_ACTION)
		{
			mode = "Action";
		}

		pGraphState->SetInput(mode.c_str(), params.animation.c_str());
	}

	//CryLogAlways("<C++>[%s][%s][%s] mode = %s, animation = %s", 
	//	TOS_Debug::GetEnv(), 
	//	TOS_Debug::GetAct(3), 
	//	__FUNCTION__, 
	//	mode.c_str(), params.animation.c_str());

	return true;
}

//IMPLEMENT_RMI(CTOSActor, SvRequestMarkMeAsMaster)
//{
//	// Описываем здесь всё, что будет выполняться на сервере
//
//	m_isMaster = params.value;
//	GetGameObject()->InvokeRMI(ClMarkMeAsMaster(), params, eRMI_ToAllClients);
//
//	GetGameObject()->ChangedNetworkState(TOS_NET::SERVER_ASPECT_STATIC);
//
//	CryLog("<C++>[%s][%s][%s][%s] mark as master = %i",
//		TOS_Debug::GetEnv(),
//		TOS_Debug::GetAct(3),
//		__FUNCTION__,
//		m_debugName,
//		params.value);
//
//
//	return true;
//}
//
//IMPLEMENT_RMI(CTOSActor, ClMarkMeAsMaster)
//{
//	// Описываем здесь всё, что будет выполняться на клиенте
//
//	m_isMaster = params.value;
//
//	CryLog("<C++>[%s][%s][%s][%s] mark as master = %i",
//		TOS_Debug::GetEnv(),
//		TOS_Debug::GetAct(3),
//		__FUNCTION__,
//		m_debugName,
//		params.value);
//
//	return true;
//}
//
//IMPLEMENT_RMI(CTOSActor, SvRequestMarkMeAsSlave)
//{
//	// Описываем здесь всё, что будет выполняться на сервере
//
//	m_isSlave = params.value;
//
//	GetGameObject()->ChangedNetworkState(TOS_NET::SERVER_ASPECT_STATIC);
//
//	CryLog("<C++>[%s][%s][%s][%s] mark as slave = %i",
//		TOS_Debug::GetEnv(),
//		TOS_Debug::GetAct(3),
//		__FUNCTION__,
//		m_debugName,
//		params.value);
//	
//
//	return true;
//}
//
//IMPLEMENT_RMI(CTOSActor, ClMarkMeAsSlave)
//{
//	// Описываем здесь всё, что будет выполняться на клиенте
//
//	m_isSlave = params.value;
//
//	CryLog("<C++>[%s][%s][%s][%s] mark as slave = %i",
//		TOS_Debug::GetEnv(),
//		TOS_Debug::GetAct(3),
//		__FUNCTION__,
//		m_debugName,
//		params.value);
//
//	return true;
//}

IMPLEMENT_RMI(CTOSActor, SvRequestHideMe)
{
	// Описываем здесь всё, что будет выполняться на сервере

	// 13.01.2024 Akeeper: Не уверен на счёт этих строк, но пусть они тут будут (519-521)
	const auto* pFists = static_cast<CFists*>(GetItemByClass(CItem::sFistsClass));
	if (pFists)
		g_pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(this, pFists->GetEntityId());

	GetGameObject()->SetAspectProfile(eEA_Physics, GetSpectatorMode() != 0 || params.hide ? eAP_Spectator : eAP_Alive);
	GetGameObject()->InvokeRMI(ClMarkHideMe(), params, eRMI_ToAllClients);

	return true;
}

IMPLEMENT_RMI(CTOSActor, ClMarkHideMe)
{
	// Описываем здесь всё, что будет выполняться на клиенте

	HideMe(params.hide);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClTOSJump)
{
	CMovementRequest request;
	request.SetJump();
	GetMovementController()->RequestMovement(request);

	CryLog("[%s] Received jump", m_debugName);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, SvRequestTOSJump)
{
	auto channelId = g_pGame->GetIGameFramework()->GetGameChannelId(pNetChannel);
	GetGameObject()->InvokeRMI(ClTOSJump(), params, eRMI_ToOtherClients | eRMI_NoLocalCalls, channelId);
	GetGameObject()->Pulse('bang');

	if ((IsSlave() && !IsLocalSlave()) || IsClient())
	{
		CMovementRequest request;
		request.SetJump();
		GetMovementController()->RequestMovement(request);

		CryLog("[%s] Requesting jump to channel %i", m_debugName, channelId);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClAttachChild)
{
	IEntity* pChild = TOS_GET_ENTITY(params.id);
	if (pChild)
	{
		GetEntity()->AttachChild(pChild, params.flags);
		CryLog("[%s] Received attach child '%s'", m_debugName, pChild->GetName());
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, SvRequestAttachChild)
{
	IEntity* pChild = TOS_GET_ENTITY(params.id);
	if (pChild)
	{
		CryLog("[%s] Requesting attach child '%s'", m_debugName, pChild->GetName());
		GetEntity()->AttachChild(pChild, params.flags);
	}

	GetGameObject()->InvokeRMI(ClAttachChild(), params, eRMI_ToRemoteClients);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClClearInventory)
{
	CryLog("<C++>[%s][%s][ClClearInventory]",
		TOS_Debug::GetEnv(), TOS_Debug::GetAct(3));

	RemoveAllItems();
	return true;
}

// Crysis Co-op
//------------------------------------------------------------------------

IMPLEMENT_RMI(CTOSActor, ClAISelectItem)
{
	if (params.select)
		SelectItem(params.itemId, false);
	HolsterItem(params.isHolstered);
	return true;
}

IMPLEMENT_RMI(CTOSActor, ClLooseHelmet)
{
	Vec3 hitDir = params.hitPos;
	Vec3 hitPos = params.hitDir;

	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
	if (!pCharacter)
		return true;
	IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
	//get helmet attachment
	bool hasProtection = true;

	IAttachment* pAttachment = pAttachmentManager->GetInterfaceByName("helmet");
	if (!pAttachment)
	{
		hasProtection = false;
		pAttachment = pAttachmentManager->GetInterfaceByName("hat");
	}

	if (pAttachment)
	{
		IAttachmentObject* pAttachmentObj = pAttachment->GetIAttachmentObject();
		if (pAttachmentObj)
		{
			IEntityClassRegistry* pClassRegistry = gEnv->pEntitySystem->GetClassRegistry();
			pClassRegistry->IteratorMoveFirst();
			IEntityClass* pEntityClass = pClassRegistry->FindClass("Default");
			if (!pEntityClass)
				return true;

			//spawn new helmet entity
			string helmetName(GetEntity()->GetName());
			helmetName.append("_helmet");
			SEntitySpawnParams params;
			params.sName = helmetName.c_str();
			params.nFlags = ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_MODIFIED_BY_PHYSICS | ENTITY_FLAG_SPAWNED;
			params.pClass = pEntityClass;

			IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(params, true);
			if (!pEntity)
				return false;

			IAttachmentObject::EType type = pAttachmentObj->GetAttachmentType();
			if (type != IAttachmentObject::eAttachment_StatObj)
			{
				gEnv->pEntitySystem->RemoveEntity(pEntity->GetId());
				return false;
			}
			IStatObj* pStatObj = pAttachmentObj->GetIStatObj();

			//set helmet geometry to new entity
			pEntity->SetStatObj(pStatObj, 0, true, 5);
			IMaterial* pUsedMaterial = pAttachmentObj->GetMaterial();
			if (pUsedMaterial)
			{
				pEntity->SetMaterial(pUsedMaterial);
				m_lostHelmetMaterial = pUsedMaterial->GetName();
			}

			Vec3 pos(GetEntity()->GetWorldPos() + GetLocalEyePos(BONE_EYE_R));
			pos.z += 0.2f;
			pEntity->SetPos(pos);

			SEntityPhysicalizeParams pparams;
			pparams.type = PE_RIGID;
			pparams.nSlot = -1;
			pparams.mass = 5;
			pparams.density = 1.0f;
			pEntity->Physicalize(pparams);

			IPhysicalEntity* pPE = pEntity->GetPhysics();
			if (!pPE)
			{
				gEnv->pEntitySystem->RemoveEntity(pEntity->GetId());
				return true;
			}

			//some hit-impulse for the helmet
			if (hitDir.len())
			{
				hitDir.Normalize();
				pe_action_impulse imp;
				hitDir += Vec3(0, 0, 0.2f);
				float r = cry_frand();
				imp.impulse = hitDir * (30.0f * max(0.1f, r));
				imp.angImpulse = (r <= 0.8f && r > 0.3f) ? Vec3(0, -1, 0) : Vec3(0, 1, 0);
				pPE->Action(&imp);
			}

			//remove old helmet
			pAttachment->ClearBinding();
			m_lostHelmet = pEntity->GetId();
			m_lostHelmetObj = pStatObj->GetFilePath();
			m_lostHelmetPos = hasProtection ? "helmet" : "hat";

			//add hair if necessary
			IAttachment* pHairAttachment = pAttachmentManager->GetInterfaceByName("hair");
			if (pHairAttachment)
			{
				if (pHairAttachment->IsAttachmentHidden())
					pHairAttachment->HideAttachment(0);
			}
		}
	}

	return true;    // Always return true - false will drop connection
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClPlayNetworkedSoundEvent)
{
	IEntity* pEntity = GetEntity();

	// Get or Create sound proxy if necessary.
	IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)pEntity->GetProxy(ENTITY_PROXY_SOUND);
	if (!pSoundProxy)
		if (GetEntity()->CreateProxy(ENTITY_PROXY_SOUND))
			pSoundProxy = (IEntitySoundProxy*)GetEntity()->GetProxy(ENTITY_PROXY_SOUND);

	if (pSoundProxy)
	{
		tSoundID ID = INVALID_SOUNDID;
		EntityId pSkipEnts[1];
		int nSkipEnts = 0;

		if (params.nSoundFlags & FLAG_SOUND_OBSTRUCTION)
		{
			pSkipEnts[0] = pEntity->GetId();
			nSkipEnts = 1;
		}

		if (pSoundProxy)
			ID = pSoundProxy->PlaySound(params.sSoundOrEventName, params.vOffset, params.vDirection, params.nSoundFlags, (ESoundSemantic)params.nSemantic, pSkipEnts, nSkipEnts);
	}
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClSetNetworkedAttachmentEffect)
{
	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(params.characterSlot);

	if (!pCharacter)
		return true;

	IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(params.attachmentName);

	if (!pIAttachment)
		return true;

	CEffectAttachment* pEffectAttachment = new CEffectAttachment(params.effectName, params.offset, params.dir, params.scale);

	pIAttachment->AddBinding(pEffectAttachment);
	pIAttachment->HideAttachment(0);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClPlayReadabilitySound)
{
	IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)GetEntity()->GetProxy(ENTITY_PROXY_SOUND);
	if (!pSoundProxy)
		if (GetEntity()->CreateProxy(ENTITY_PROXY_SOUND))
			pSoundProxy = (IEntitySoundProxy*)GetEntity()->GetProxy(ENTITY_PROXY_SOUND);

	if (pSoundProxy)
	{
		int sFlags = FLAG_SOUND_DEFAULT_3D | FLAG_SOUND_START_PAUSED | FLAG_SOUND_VOICE;
		int soundID = pSoundProxy->PlaySoundEx(params.sSoundEventName, Vec3(ZERO), FORWARD_DIRECTION, sFlags, 1.0f, 2.0f, 5.0f, eSoundSemantic_Dialog);

		if (soundID == INVALID_SOUNDID)
		{
			if (CCoopSystem::GetInstance()->GetDebugLog() > 0)
				CryLogAlways("[Coop] Sound:PlaySound - Can't play sound");
		}

		ISound* pSound = gEnv->pSoundSystem->GetSound(soundID);
		if (pSound)
		{
			pSound->SetPaused(false);
		}
	}
	return true;
}

IMPLEMENT_RMI(CTOSActor, ClPlayNetworkedAnimation)
{
	if (IAnimationGraphState* pGraphState = (this->GetAnimatedCharacter() ? this->GetAnimatedCharacter()->GetAnimationGraphState() : 0))
	{
		if (CCoopSystem::GetInstance()->GetDebugLog() > 1)
			CryLogAlways("[CTOSActor] Actor %s received animation mode %d for animation %s.", this->GetEntity()->GetName(), params.nMode, params.sAnimation.c_str());
		if (params.nMode == EAnimationMode::AIANIM_SIGNAL)
		{
			pGraphState->SetInput("Signal", params.sAnimation);
		}
		else if (params.nMode == EAnimationMode::AIANIM_ACTION)
		{
			pGraphState->SetInput("Action", params.sAnimation);
		}
	}
	//this->GetAnimationGraphState()->SetInput()

	return true;
}
// ~Crysis Co-op
