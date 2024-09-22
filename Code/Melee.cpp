/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 23:3:2006   13:05 : Created by Mбrcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Melee.h"
#include "Game.h"
#include "Item.h"
#include "Weapon.h"
#include "GameRules.h"
#include "Player.h"
#include "BulletTime.h"
#include <IEntitySystem.h>
#include "IMaterialEffects.h"
#include "GameCVars.h"


#include "IRenderer.h"
#include "IRenderAuxGeom.h"

//TheOtherSide
#include "TheOtherSideMP/Actors/player/TOSPlayer.h"
//~TheOtherSide

//std::vector<Vec3> g_points;

//------------------------------------------------------------------------
CMelee::CMelee()
{
	m_noImpulse = false;
}

//------------------------------------------------------------------------
CMelee::~CMelee()
{
}

const float STRENGTH_MULT = 0.018f;

void CMelee::Init(IWeapon *pWeapon, const struct IItemParamsNode *params)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);

	if (params)
		ResetParams(params);

	m_attacking = false;
	m_attacked = false;
	m_delayTimer=0.0f;
	m_durationTimer=0.0f;
	m_ignoredEntity = 0;
	m_meleeScale = 1.0f;
}

//------------------------------------------------------------------------
void CMelee::Update(float frameTime, uint frameId)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	bool requireUpdate = false;
	
	if (m_attacking)
	{
		requireUpdate = true;
		if (m_delayTimer>0.0f)
		{
			m_delayTimer-=frameTime;
			if (m_delayTimer<=0.0f)
				m_delayTimer=0.0f;
		}
		else
		{
			if (!m_attacked)
			{
				m_attacked = true;

				CActor *pActor = m_pWeapon->GetOwnerActor();
				if(!pActor)
					return;

				Vec3 pos(ZERO);
				Vec3 dir(ZERO);
				IMovementController * pMC = pActor->GetMovementController();
				if (!pMC)
					return;

				float strength = 1.0f;//pActor->GetActorStrength();
				if (pActor->GetActorClass() == CPlayer::GetActorClassType())
				{
					CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
					if (CNanoSuit *pSuit = pPlayer->GetNanoSuit())
					{
						ENanoMode curMode = pSuit->GetMode();
						if (curMode == NANOMODE_STRENGTH)
						{
							strength = pActor->GetActorStrength();
							strength = strength * (1.0f + 2.0f * pSuit->GetSlotValue(NANOSLOT_STRENGTH)* STRENGTH_MULT);
							if(!pPlayer->IsPlayer() && g_pGameCVars->g_difficultyLevel < 4)
								strength *= g_pGameCVars->g_AiSuitStrengthMeleeMult;
						}
					}
				}

				SMovementState info;
				pMC->GetMovementState(info);
				pos = info.eyePosition;
				dir = info.eyeDirection;
				if (!PerformRayTest(pos, dir, strength, false))
					if(!PerformCylinderTest(pos, dir, strength, false))
						ApplyCameraShake(false);

				m_ignoredEntity = 0;
				m_meleeScale = 1.0f;
				
				m_pWeapon->RequestMeleeAttack(m_pWeapon->GetMeleeFireMode()==this, pos, dir, m_pWeapon->GetShootSeqN());
			}
		}
	}

	if (requireUpdate)
		m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CMelee::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CMelee::ResetParams(const struct IItemParamsNode *params)
{
	const IItemParamsNode *melee = params?params->GetChild("melee"):0;
	const IItemParamsNode *actions = params?params->GetChild("actions"):0;

	m_meleeparams.Reset(melee);
	m_meleeactions.Reset(actions);
}

//------------------------------------------------------------------------
void CMelee::PatchParams(const struct IItemParamsNode *patch)
{
	const IItemParamsNode *melee = patch->GetChild("melee");
	const IItemParamsNode *actions = patch->GetChild("actions");

	m_meleeparams.Reset(melee, false);
	m_meleeactions.Reset(actions, false);
}

//------------------------------------------------------------------------
void CMelee::Activate(bool activate)
{
	m_attacking = m_noImpulse = false;
	m_delayTimer=0.0f;
	m_durationTimer=0.0f;
}

//------------------------------------------------------------------------
bool CMelee::CanFire(bool considerAmmo) const
{
	return !m_attacking;
}

//------------------------------------------------------------------------
struct CMelee::StopAttackingAction
{
	CMelee *_this;
	StopAttackingAction(CMelee *melee): _this(melee) {};
	void execute(CItem *pItem)
	{
		_this->m_attacking = false;

		_this->m_delayTimer = 0.0f;
		_this->m_durationTimer = 0.0f;
		pItem->SetBusy(false);
		// this allows us to blend into the idle animation (for swimming) -- johnn
		//pItem->ResetAnimation();
		//pItem->ReAttachAccessories();
		//pItem->PlayAction(ItemStrings::idle, 0, false, CItem::eIPAF_CleanBlending | CItem::eIPAF_NoBlend | CItem::eIPAF_Default);
		//pItem->PlayAction(pItem->GetDefaultIdleAnimation(0), 0, false, CItem::eIPAF_CleanBlending | CItem::eIPAF_Default);

	}
};

void CMelee::StartFire()
{
	if (!CanFire())
		return;

	//Prevent fists melee exploit 
	if ((m_pWeapon->GetEntity()->GetClass() == CItem::sFistsClass) && m_pWeapon->IsBusy())
		return;

	CTOSActor* pOwner = static_cast<CTOSActor*>(m_pWeapon->GetOwnerActor());

	if(pOwner)
	{
		if(pOwner->GetStance()==STANCE_PRONE)
			return;

		//TheOtherSide
		//Здесь нигде не определяется что pOwner это игрок.
		//Нужно это исправить.

		if (pOwner->GetActorClass() == CPlayer::GetActorClassType())
		{
			const SPlayerStats* pPlayerStats = static_cast<SPlayerStats*>(pOwner->GetActorStats());
			if (pPlayerStats)
			{
				if (pPlayerStats->bLookingAtFriendlyAI)
					return;
			}
		}

		const auto slaveStats = pOwner->GetSlaveStats();
		if (slaveStats.lookAtFriend)
			return;
		//~TheOtheSide
	}

	m_attacking = true;
	m_attacked = false;
	m_pWeapon->RequireUpdate(eIUS_FireMode);
	m_pWeapon->ExitZoom();

	const bool isClient = pOwner ? pOwner->IsClient():false;

	if (g_pGameCVars->bt_end_melee && isClient)
		g_pGame->GetBulletTime()->Activate(false);


	float speedOverride = -1.0f;

	//TheOtherSide
	//Проблема: вылет при ударе в рукопашкую
	const auto pPlayer = static_cast<CPlayer*>(pOwner);
	if(pPlayer)
	{
		if(CNanoSuit *pSuit = pPlayer->GetNanoSuit())
		{
			const ENanoMode curMode = pSuit->GetMode();
			if (curMode == NANOMODE_SPEED && pSuit->GetSuitEnergy() > NANOSUIT_ENERGY * 0.1f)
				speedOverride = 1.5f;
		}
		
		pPlayer->PlaySound(CPlayer::ESound_Melee);
	}

	//~TheOtherSide

	m_pWeapon->PlayAction(m_meleeactions.attack.c_str(), 0, false, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending, speedOverride);
	m_pWeapon->SetBusy(true);

	//TheOtherSide

	//m_beginPos = m_pWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, m_meleeparams.helper.c_str(), true);
	//m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<StopAttackingAction>::Create(this), true);

	uint time = m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson);
	m_durationTimer = m_meleeparams.duration;

	if (pOwner && pOwner->IsSlave())
		time = m_durationTimer * 1000;

	m_pWeapon->GetScheduler()->TimerAction(time, CSchedulerAction<StopAttackingAction>::Create(this), true);

	//~TheOtherSide
	

	m_delayTimer = m_meleeparams.delay;
	
	if (g_pGameCVars->dt_enable && m_delayTimer < g_pGameCVars->dt_time)
		m_delayTimer = g_pGameCVars->dt_time;


	m_pWeapon->OnMelee(m_pWeapon->GetOwnerId());

	m_pWeapon->RequestStartMeleeAttack(m_pWeapon->GetMeleeFireMode()==this);
}

//------------------------------------------------------------------------
void CMelee::StopFire()
{
}

//------------------------------------------------------------------------
void CMelee::NetStartFire()
{
	m_pWeapon->OnMelee(m_pWeapon->GetOwnerId());

	float speedOverride = -1.0f;

	//TheOtherSide

	const auto pPlayer = static_cast<CPlayer*>(m_pWeapon->GetOwnerActor());
	if(pPlayer)
	{
		if(const CNanoSuit *pSuit = pPlayer->GetNanoSuit())
		{
			const ENanoMode curMode = pSuit->GetMode();
			if (curMode == NANOMODE_SPEED)
				speedOverride = 1.5f;
		}
	}
	//~TheOtherSide

	m_pWeapon->PlayAction(m_meleeactions.attack.c_str(), 0, false, CItem::eIPAF_Default, speedOverride);
}

//------------------------------------------------------------------------
void CMelee::NetStopFire()
{
}

//------------------------------------------------------------------------
void CMelee::NetShoot(const Vec3 &hit, int ph)
{
}

//------------------------------------------------------------------------
void CMelee::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int ph)
{
	float strength=GetOwnerStrength();

	if (!PerformRayTest(pos, dir, strength, true))
		if(!PerformCylinderTest(pos, dir, strength, true))
			ApplyCameraShake(false);

	m_ignoredEntity = 0;
	m_meleeScale = 1.0f;
}

//------------------------------------------------------------------------
const char *CMelee::GetType() const
{
	return "Melee";
}

//------------------------------------------------------------------------
int CMelee::GetDamage(float distance) const
{
	if(distance > m_meleeparams.range * 1.5f)	// added a bit to compensate for network differences...
		return 0;

	// NOTE: in multiplayer m_meleeScale == 1.0,
	// however this is not a problem since we cannot pick up objects in that scenario

	if(!gEnv->bMultiplayer)
		return static_cast<int>(m_meleeparams.damage*GetOwnerStrength()*m_meleeScale);

	float	 defaultValue = m_meleeparams.damage*m_meleeScale;
	CActor *pActor = m_pWeapon->GetOwnerActor();
	if(!pActor)
		return defaultValue;

	if (pActor->GetActorClass() == CPlayer::GetActorClassType())
	{
		CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
		if (CNanoSuit *pSuit = pPlayer->GetNanoSuit())
		{
			ENanoMode curMode = pSuit->GetMode();
			if (curMode == NANOMODE_STRENGTH && pSuit->GetSuitEnergy()>=0 )
				return g_pGameCVars->g_meleeStrenghtDamage;
		}
	}
	return g_pGameCVars->g_meleeDamage;
}

//------------------------------------------------------------------------
const char *CMelee::GetDamageType() const
{
	return m_meleeparams.hit_type.c_str();
}


//------------------------------------------------------------------------
bool CMelee::PerformRayTest(const Vec3 &pos, const Vec3 &dir, float strength, bool remote)
{
	IEntity *pOwner = m_pWeapon->GetOwner();
	IPhysicalEntity *pIgnore = pOwner?pOwner->GetPhysics():0;

	ray_hit hit;
	int n =gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir.normalized()*m_meleeparams.range, ent_all|ent_water,
			rwi_stop_at_pierceable|rwi_ignore_back_faces,&hit, 1, &pIgnore, pIgnore?1:0);

	//===================OffHand melee (also in PerformCylincerTest)===================
	if(m_ignoredEntity && (n>0))
	{
		if(IEntity* pHeldObject = gEnv->pEntitySystem->GetEntity(m_ignoredEntity))
		{
			IPhysicalEntity *pHeldObjectPhysics = pHeldObject->GetPhysics();
			if(pHeldObjectPhysics==hit.pCollider)
				return false;
		}
	}
	//=================================================================================

	if (n>0)
	{
		Hit(&hit, dir, strength, remote);
		Impulse(hit.pt, dir, hit.n, hit.pCollider, hit.partid, hit.ipart, hit.surface_idx, strength);
	}

	return n>0;
}

//------------------------------------------------------------------------
bool CMelee::PerformCylinderTest(const Vec3 &pos, const Vec3 &dir, float strength, bool remote)
{
	IEntity *pOwner = m_pWeapon->GetOwner();
	IPhysicalEntity *pIgnore = pOwner?pOwner->GetPhysics():0;
	IEntity *pHeldObject = NULL;

	if(m_ignoredEntity)
		pHeldObject = gEnv->pEntitySystem->GetEntity(m_ignoredEntity);
		
	primitives::cylinder cyl;
	cyl.r = 0.25f;
	cyl.axis = dir;
	cyl.hh = m_meleeparams.range/2.0f;
	cyl.center = pos + dir.normalized()*cyl.hh;
	
	float n = 0.0f;
	geom_contact *contacts;
	intersection_params params;
	params.bStopAtFirstTri = false;
	params.bNoBorder = true;
	params.bNoAreaContacts = true;
	n = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(primitives::cylinder::type, &cyl, Vec3(ZERO), 
		ent_rigid|ent_sleeping_rigid|ent_independent|ent_static|ent_terrain|ent_water, &contacts, 0,
		geom_colltype0|geom_colltype_foliage|geom_colltype_player, &params, 0, 0, &pIgnore, pIgnore?1:0);

	int ret = static_cast<int>(n);

	float closestdSq = 9999.0f;
	geom_contact *closestc = 0;
	geom_contact *currentc = contacts;

	for (int i=0; i<ret; i++)
	{
		geom_contact *contact = currentc;
		if (contact)
		{
			IPhysicalEntity *pCollider = gEnv->pPhysicalWorld->GetPhysicalEntityById(contact->iPrim[0]);
			if (pCollider)
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pCollider);
				if (pEntity)
				{
					if ((pEntity == pOwner)||(pHeldObject && (pEntity == pHeldObject)))
					{
						++currentc;
						continue;
					}
				}

				float distSq = (pos-currentc->pt).len2();
				if (distSq < closestdSq)
				{
					closestdSq = distSq;
					closestc = contact;
				}
			}
		}
		++currentc;
	}

	if (ret)
	{
		WriteLockCond lockColl(*params.plock, 0);
		lockColl.SetActive(1);
	}

  
	if (closestc)
	{
		IPhysicalEntity *pCollider = gEnv->pPhysicalWorld->GetPhysicalEntityById(closestc->iPrim[0]);

		Hit(closestc, dir, strength, remote);
		Impulse(closestc->pt, dir, closestc->n, pCollider, closestc->iPrim[1], 0, closestc->id[1], strength);
	}

	return closestc!=0;
}

//------------------------------------------------------------------------
void CMelee::Hit(const Vec3 &pt, const Vec3 &dir, const Vec3 &normal, IPhysicalEntity *pCollider, int partId, int ipart, int surfaceIdx, float damageScale, bool remote)
{
	// generate the damage
	IEntity *pTarget = gEnv->pEntitySystem->GetEntityFromPhysics(pCollider);

	// Report punch to AI system.
	// The AI notification must come before the game rules are 
	// called so that the death handler in AIsystem understands that the hit
	// came from the player.
	CActor *pActor = m_pWeapon->GetOwnerActor();
	if (pActor && pActor->GetActorClass() == CPlayer::GetActorClassType())
	{
		auto pPlayer = static_cast<CPlayer*>(pActor);
		if (pPlayer && pPlayer->GetNanoSuit())
		{
			if (pPlayer->GetEntity() && pPlayer->GetEntity()->GetAI())
			{
				SAIEVENT aiEvent;
				aiEvent.targetId = pTarget ? pTarget->GetId() : 0;
				// pPlayer->GetNanoSuit()->GetMode() == NANOMODE_STRENGTH
				pPlayer->GetEntity()->GetAI()->Event(AIEVENT_PLAYER_STUNT_PUNCH, &aiEvent);
			}
		}
	}

	bool ok = true;
	if(pTarget)
	{
		if(!gEnv->bMultiplayer && pActor && pActor->IsPlayer())
		{
			IActor* pAITarget = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTarget->GetId());
			if(pAITarget && pTarget->GetAI() && !pTarget->GetAI()->IsHostile(pActor->GetEntity()->GetAI(),false))
			{
				ok = false;
				m_noImpulse = true;
			}
		}

		if(ok)
		{
			CGameRules *pGameRules = g_pGame->GetGameRules();
			//TheOtherSide
			// Закомментируем неиспользуемое

			//int damage = m_meleeparams.damage;
			//if(pActor && !pActor->IsPlayer())
			//	damage = m_meleeparams.damageAI;
			//~TheOtherSide

			HitInfo info(
				m_pWeapon->GetOwnerId(), 
				pTarget->GetId(), 
				m_pWeapon->GetEntityId(),
				m_pWeapon->GetFireModeIdx(GetName()), 
				0.0f, 
				pGameRules->GetHitMaterialIdFromSurfaceId(surfaceIdx), 
				partId,
				pGameRules->GetHitTypeId(m_meleeparams.hit_type.c_str()), 
				pt, 
				dir, 
				normal);

			info.remote = remote;
			if (!remote)
				info.seq=m_pWeapon->GenerateShootSeqN();
			info.damage = m_meleeparams.damage;

			if (m_pWeapon->GetForcedHitMaterial() != -1)
				info.material=pGameRules->GetHitMaterialIdFromSurfaceId(m_pWeapon->GetForcedHitMaterial());

			pGameRules->ClientHit(info);
		}
	}

	// play effects
	if(ok)
	{
		IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();

		TMFXEffectId effectId = pMaterialEffects->GetEffectId("melee", surfaceIdx);
		if (effectId != InvalidEffectId)
		{
			SMFXRunTimeEffectParams params;
			params.pos = pt;
			params.playflags = MFX_PLAY_ALL | MFX_DISABLE_DELAY;
			params.soundSemantic = eSoundSemantic_Player_Foley;
			pMaterialEffects->ExecuteEffect(effectId, params);
		}
	}

	ApplyCameraShake(true);

	m_pWeapon->PlayAction(m_meleeactions.hit.c_str());
}

//------------------------------------------------------------------------
// Функция Impulse добавляет импульс объекту в физической симуляции
void CMelee::Impulse(const Vec3& pt, const Vec3& dir, const Vec3& normal, IPhysicalEntity* pCollider, int partId, int ipart, int surfaceIdx, float impulseScale)
{
	// Если импульс не должен быть применен, выходим из функции
	if (m_noImpulse)
	{
		m_noImpulse = false;
		return;
	}

	// Применяем импульс, если коллайдер существует и параметр импульса больше заданного порога
	if (pCollider && m_meleeparams.impulse > 0.001f)
	{
		bool strengthMode = false; // Режим силы для нанокостюма

		// Получаем актера, владеющего оружием, и проверяем, включен ли режим силы
		auto pPlayer = static_cast<CTOSPlayer*>(m_pWeapon->GetOwnerActor());
		if (pPlayer)
		{
			if (CNanoSuit* pSuit = pPlayer->GetNanoSuit())
			{
				ENanoMode curMode = pSuit->GetMode();
				if (curMode == NANOMODE_STRENGTH)
					strengthMode = true;
			}
		}

		// Получаем динамические параметры коллайдера
		pe_status_dynamics dyn;
		float auxScale = 1.0f; // Вспомогательный масштаб для импульса

		// Если не удается получить статус динамики, увеличиваем масштаб импульса в режиме силы
		if (!pCollider->GetStatus(&dyn))
		{
			if (strengthMode)
				impulseScale *= 3.0f;
		}
		else
		{
			// Если удается получить статус, корректируем масштаб импульса в зависимости от массы
			auto pIEntity = static_cast<IEntity*>(pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY));
			IPhysicalEntity* pMainPhys;
			float mass0 = dyn.mass;
			if (pIEntity && (pMainPhys = pIEntity->GetPhysics()) && pMainPhys != pCollider && pMainPhys->GetStatus(&dyn))
				auxScale = dyn.mass ? mass0 / dyn.mass : 0;
			impulseScale *= clamp((dyn.mass * 0.01f), 1.0f, 15.0f);
		}

		// Добавляем импульс к физическому прокси, чтобы убедиться, что он применяется к цилиндру, а не только к скелету
		auto pEntity = static_cast<IEntity*>(pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY));
		if (gEnv->bMultiplayer && pEntity)
		{
			if (g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()) == NULL)
				impulseScale *= 0.33f;
		}

		// Применяем импульс, если объект не является "crap doll"
		if (pEntity)
		{
			bool crapDollFilter = false;
#ifdef CRAPDOLLS
			static IEntityClass* pDeadBodyClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DeadBody");
			if (pEntity->GetClass() == pDeadBodyClass)
				crapDollFilter = true;
#endif //CRAPDOLLS

			if (!crapDollFilter)
			{
				IEntityPhysicalProxy* pPhysicsProxy = static_cast<IEntityPhysicalProxy*>(pEntity->GetProxy(ENTITY_PROXY_PHYSICS));
				CActor* pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()));

				// Если актер находится в состоянии "ragdoll", корректируем масштаб импульса
				if (pActor)
				{
					SActorStats* pAS = pActor->GetActorStats();
					if (pAS && pAS->isRagDoll)
					{
						impulseScale = 1.0f; // Импульс был уменьшен, убедитесь, что он "едва двигается"
#ifdef CRAPDOLLS
						crapDollFilter = true;
#endif //CRAPDOLLS
					}
				}

				// Увеличиваем импульс для игрока и ограничиваем его по оси Z
				if (!crapDollFilter)
				{
					Vec3 impulse = dir * m_meleeparams.impulse * impulseScale * m_meleeScale;
					impulse.z = clamp(impulse.z, -40.0f, 40.0f);
					pPhysicsProxy->AddImpulse(partId, pt, impulse, true, auxScale);
				}
			}
		}
		else
		{
			// Если нет pEntity, применяем импульс старым способом
			pe_action_impulse ai;
			ai.partid = partId;
			ai.ipart = ipart;
			ai.point = pt;
			ai.iApplyTime = 0;
			ai.impulse = dir * (m_meleeparams.impulse * impulseScale * m_meleeScale);
			pCollider->Action(&ai);
		}

		// Создаем физическое столкновение для разрушения деревьев
		ISurfaceTypeManager* pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
		int invId = pSurfaceTypeManager->GetSurfaceTypeByName("mat_invulnerable")->GetId();

		pe_action_register_coll_event collision;
		collision.collMass = 0.005f; // Это значение фактически игнорируется
		collision.partid[1] = partId;
		collision.partid[0] = -2; // Столкновения с partId<-1 игнорируются при расчете урона в игре
		collision.idmat[1] = surfaceIdx;
		collision.idmat[0] = invId;
		collision.n = normal;
		collision.pt = pt;

		// Рассчитываем скорость для создания эффекта столкновения, аналогичного пуле SCAR
		Vec3 v = dir;
		float speed = cry_sqrtf(4000.0f / (80.0f * 0.5f)); // 80.0f - масса игрока

		// Проверяем, является ли объект кистью, и корректируем скорость в зависимости от режима нанокостюма
		auto pBrush = static_cast<IRenderNode*>(pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC));
		if (pBrush)
		{
			CActor* pActor = m_pWeapon->GetOwnerActor();
			if (pActor && (pActor->GetActorClass() == CPlayer::GetActorClassType()))
			{
				if (pPlayer)
				{
					if (CNanoSuit* pSuit = pPlayer->GetNanoSuit())
					{
						ENanoMode curMode = pSuit->GetMode();
						if (curMode != NANOMODE_STRENGTH)
							speed = 0.003f;
					}
				}
			}
		}

		collision.vSelf = (v.normalized() * speed * m_meleeScale);
		collision.v = Vec3(0, 0, 0);
		collision.pCollider = pCollider;

		// Применяем столкновение к физике скелета, если она существует
		IEntity* pOwner = m_pWeapon->GetOwner();
		if (pOwner && pOwner->GetCharacter(0) && pOwner->GetCharacter(0)->GetISkeletonPose()->GetCharacterPhysics())
		{
			if (ISkeletonPose* pSkeletonPose = pOwner->GetCharacter(0)->GetISkeletonPose())
			{
				if (pSkeletonPose && pSkeletonPose->GetCharacterPhysics())
					pSkeletonPose->GetCharacterPhysics()->Action(&collision);
			}
		}
	}
}

//------------------------------------------------------------------------
void CMelee::Hit(geom_contact *contact, const Vec3 &dir, float damageScale, bool remote)
{
	CActor *pOwner = m_pWeapon->GetOwnerActor();
	if (!pOwner)
		return;

	Vec3 view(0.0f, 1.0f, 0.0f);

	if (IMovementController *pMC = pOwner->GetMovementController())
	{
		SMovementState state;
		pMC->GetMovementState(state);
		view = state.eyeDirection;
	}

	// some corrections to make sure the impulse is always away from the camera, and is not a backface collision
	bool backface = dir.Dot(contact->n)>0;
	bool away = dir.Dot(view.normalized())>0; // away from cam?

	Vec3 normal=contact->n;
	Vec3 ndir=dir;

	if (backface)
	{
		if (away)
			normal = -normal;
		else
			ndir = -dir;
	}
	else
	{
		if (!away)
		{
			ndir = -dir;
			normal = -normal;
		}
	}

	IPhysicalEntity *pCollider = gEnv->pPhysicalWorld->GetPhysicalEntityById(contact->iPrim[0]);

	Hit(contact->pt, ndir, normal, pCollider, contact->iPrim[1], 0, contact->id[1], damageScale, remote);
}

//------------------------------------------------------------------------
void CMelee::Hit(ray_hit *hit, const Vec3 &dir, float damageScale, bool remote)
{
	Hit(hit->pt, dir, hit->n, hit->pCollider, hit->partid, hit->ipart, hit->surface_idx, damageScale, remote);
}

//-----------------------------------------------------------------------
void CMelee::ApplyCameraShake(bool hit)
{
	// Add some camera shake for client even if not hitting
	//TheOtherSide
	const auto pOwner = static_cast<CTOSActor*>(m_pWeapon->GetOwnerActor());
	const auto cond = pOwner && 
		(pOwner->IsClient() || pOwner->IsLocalSlave()) ;
	//~TheOtherSide

	if(cond)
	{
		if(CScreenEffects* pScreenEffects = m_pWeapon->GetOwnerActor()->GetScreenEffects())
		{
			float rotateTime;
			if(!hit)
			{
				rotateTime = g_pGameCVars->hr_rotateTime*1.25f;
				pScreenEffects->CamShake(Vec3(rotateTime*0.5f,rotateTime*0.5f,rotateTime*0.25f), Vec3(0, 0.3f * g_pGameCVars->hr_rotateFactor,0), rotateTime, rotateTime);
			}
			else
			{
				rotateTime = g_pGameCVars->hr_rotateTime*2.0f;
				pScreenEffects->CamShake(Vec3(rotateTime,rotateTime,rotateTime*0.5f), Vec3(0, 0.5f * g_pGameCVars->hr_rotateFactor,0), rotateTime, rotateTime);
			}
		}
	}
}

float CMelee::GetOwnerStrength() const
{
	float strength = 1.0f;

	CActor *pActor = m_pWeapon->GetOwnerActor();
	if(!pActor)
		return strength;

	const IMovementController * pMC = pActor->GetMovementController();
	if (!pMC)
		return strength;

	//TheOtherSide
	const auto pPlayer = static_cast<CTOSPlayer*>(pActor);
	//~TheOtherSide

	if (pPlayer)
	{
		if (const CNanoSuit* pSuit = pPlayer->GetNanoSuit())
		{
			const ENanoMode curMode = pSuit->GetMode();
			if (curMode == NANOMODE_STRENGTH)
			{
				strength = pActor->GetActorStrength();
				strength = strength * (1.0f + 2.0f * pSuit->GetSlotValue(NANOSLOT_STRENGTH) * STRENGTH_MULT);
			}
		}
	}

	return strength;
}

void CMelee::GetMemoryStatistics( ICrySizer * s )
{
	s->Add(*this);
	s->Add(m_name);
	m_meleeparams.GetMemoryStatistics(s);
	m_meleeactions.GetMemoryStatistics(s);
}
