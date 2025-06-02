/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#include "StdAfx.h"
#include "IFlashPlayer.h"
#include "HUD/HUD.h"
#include "HUD/HUDSilhouettes.h"
#include "Menus/FlashMenuObject.h"
#include "GameActions.h"
#include "ZeusModule.h"
#include "ZeusSynchronizer.h"
#include <TheOtherSideMP\HUD\TOSCrosshair.h>
#include <TheOtherSideMP\Helpers\TOS_AI.h>
#include <TheOtherSideMP\Helpers\TOS_Console.h>
#include <TheOtherSideMP\Helpers\TOS_NET.h>
#include <TheOtherSideMP\Helpers\TOS_Inventory.h>
#include <TheOtherSideMP\Helpers\TOS_Entity.h>
#include <TheOtherSideMP\Helpers\TOS_Vehicle.h>
#include <TheOtherSideMP\Helpers\TOS_STL.h>
#include <TheOtherSideMP\Helpers\TOS_Screen.h>
#include <Cry_Camera.h>

std::map<string, string> CTOSZeusModule::s_classToConsoleVar;

// Карта для сопоставления названий классов с консольными переменными
static void InitSelectionFilterClasses()
{
	CTOSZeusModule::s_classToConsoleVar["BasicEntity"] = "tos_sv_zeus_selection_ignore_basic_entity";
	CTOSZeusModule::s_classToConsoleVar["RigidBody"] = "tos_sv_zeus_selection_ignore_rigid_body";
	CTOSZeusModule::s_classToConsoleVar["RigidBodyEx"] = "tos_sv_zeus_selection_ignore_rigid_body";
	CTOSZeusModule::s_classToConsoleVar["DestroyableObject"] = "tos_sv_zeus_selection_ignore_destroyable_object";
	CTOSZeusModule::s_classToConsoleVar["BreakableObject"] = "tos_sv_zeus_selection_ignore_breakable_object";
	CTOSZeusModule::s_classToConsoleVar["AnimObject"] = "tos_sv_zeus_selection_ignore_anim_object";
	CTOSZeusModule::s_classToConsoleVar["PressurizedObject"] = "tos_sv_zeus_selection_ignore_pressurized_object";
	CTOSZeusModule::s_classToConsoleVar["Switch"] = "tos_sv_zeus_selection_ignore_switch";
	CTOSZeusModule::s_classToConsoleVar["SpawnGroup"] = "tos_sv_zeus_selection_ignore_spawn_group";
	CTOSZeusModule::s_classToConsoleVar["InteractiveEntity"] = "tos_sv_zeus_selection_ignore_interactive_entity";
	CTOSZeusModule::s_classToConsoleVar["VehiclePartDetached"] = "tos_sv_zeus_selection_ignore_vehicle_part_detached";
}

CTOSZeusModule::CTOSZeusModule()
	: m_pPersistantDebug(nullptr),
	  m_pZeusScriptBind(nullptr),

	  m_clientserver(this),
	  m_local(this),
	  m_hud(this)
{
}

CTOSZeusModule::~CTOSZeusModule()
{
	if (gEnv->pHardwareMouse)
		gEnv->pHardwareMouse->RemoveListener(this);
}

void CTOSZeusModule::Reset()
{
	m_local.Reset();
	m_hud.Reset();

	auto pPlayer = GetPlayer();
	if (pPlayer)
		pPlayer->SetMeZeus(false);
}

void CTOSZeusModule::Init()
{
	if (gEnv->pHardwareMouse)
		gEnv->pHardwareMouse->AddListener(this);

	InitSelectionFilterClasses();
	Reset();

	m_pPersistantDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
}

void CTOSZeusModule::InitScriptBinds()
{
	m_pZeusScriptBind = new CScriptBind_Zeus(gEnv->pSystem, g_pGame->GetIGameFramework());
}

void CTOSZeusModule::ReleaseScriptBinds()
{
	SAFE_DELETE(m_pZeusScriptBind);
}

void CTOSZeusModule::OnExtraGameplayEvent(IEntity *pEntity, const STOSGameEvent &event)
{
	auto pHUD = g_pGame->GetHUD();
	auto pLocalPlayer = GetPlayer();

	const bool bNoModalOrNoHUD = !pHUD || (pHUD && !pHUD->IsHaveModalHUD());
	const bool bZeusing = m_local.GetFlag(EFlag::Zeusing);

	TOS_INIT_EVENT_VALUES(pEntity, event);

	switch (event.event)
	{
	// После sv_restart зевс не зевс
	//case eEGE_ActorRevived:
	//{
	//	if (!bZeusing)
	//		return;

	//	if (pLocalPlayer && pLocalPlayer->GetEntityId() == entId)
	//	{
	//		m_local.Reset();

	//		if (bNoModalOrNoHUD)
	//			m_local.ShowMouse(false);
	//		m_hud.ShowZeusMenu(false);
	//	}

	//	break;
	//}
	case eEGE_MasterClientOnStartControl:
	{
		if (!bZeusing)
			return;

		if (pLocalPlayer)
		{
			m_hud.ShowPlayerHUD(true);
			if (bNoModalOrNoHUD)
				m_local.ShowMouse(false);

			m_local.SetFlag(EFlag::Possessing, true);
		}
		break;
	}
	case eEGE_MasterClientOnStopControl:
	{
		if (!bZeusing)
			return;

		if (pLocalPlayer)
		{
			m_clientserver.DispatchMakeZeus(pLocalPlayer, true, nullptr);
			m_local.SetFlag(EFlag::Possessing, false);
		}
		break;
	}
	case eEGE_HUDInit:
	{
		m_hud.Init();
		break;
	}
	case eEGE_HUDInGamePostUpdate:
	{
		m_hud.InGamePostUpdate(event.value);
		break;
	}
	case eEGE_HUDUnloadSimpleAssets:
	{
		m_hud.UnloadSimpleAssets(event.int_value);
		break;
	}
	case eEGE_EditorGameExit:
	{
		if (!bZeusing)
			return;

		if (pLocalPlayer)
		{
			if (m_local.IsMouseDisplayed() == false)
				m_local.ShowMouse(true);

			auto it = m_local.m_orders.cbegin();
			auto end = m_local.m_orders.cend();
			for (; it != end; it++)
			{
				m_local.StopOrder(it->first);
			}
		}

		gEnv->pScriptSystem->ResetTimers();
		break;
	}
	case eEGE_ActorEnterVehicle:
	{
		if (!bZeusing)
			return;

		if (pLocalPlayer && m_local.IsSelectedEntity(pEntity->GetId()))
		{
			const auto pVehEntity = TOS_GET_ENTITY(event.int_value);
			if (pVehEntity)
			{
				if (!m_local.m_dragging)
					m_local.DeselectEntity(pEntity->GetId());

				m_local.SelectEntity(pVehEntity->GetId());
			}
		}

		break;
	}
	case eEGE_ActorDead:
	case eEGE_VehicleDestroyed:
	{
		if (!TOS_GET_CLIENT_ACTOR)
			return;

		if (!bZeusing)
			return;

		if (!pEntity)
			return;

		m_local.RemoveOrder(pEntity->GetId());

		auto orderIt = m_local.m_orders.begin();
		auto orderEnd = m_local.m_orders.end();
		for (; orderIt != orderEnd; orderIt++)
		{
			if (orderIt->second.targetId == pEntity->GetId())
			{
				m_local.StopOrder(orderIt->first);
			}
		}
		break;
	}
	case eEGE_OnSynchronizerCreated:
	{
		if (pGO)
		{
			RegisterSynchronizer(static_cast<CTOSZeusSynchronizer*>(pGO->AcquireExtension("TOSZeusSynchronizer")));
			assert(GetSynchronizer() != nullptr);
		}

		TOS_RECORD_EVENT(entId, STOSGameEvent(eEGE_OnSynchronizerRegistered, "For Zeus Module", true));

		break;
	}
	case eEGE_OnPlayerLeftZeus:
	{
		if (!pLocalPlayer)
			return;

		// Если игрок вышел из режима наблюдателя зевса, то мы должны выйти из режима зевса
		if (bZeusing)
			m_clientserver.ClientMakeZeus(false);
		break;
	}
	case eEGE_UpdateContextViewState:
	{
		if (!pLocalPlayer)
			return;
			
		// Если игра после sv_restart перешла в режим InGame, 
		// то мы должны сделать зевса если он был зевсом раньше
		EContextViewState state = static_cast<EContextViewState>(event.int_value);
		if (state == EContextViewState::eCVS_InGame)
		{
			m_clientserver.ClientMakeZeus(bZeusing, true);
		}
		break;
	}
	default:
		break;
	}
}

void CTOSZeusModule::GetMemoryStatistics(ICrySizer *s)
{
	s->Add(*this);
	// s->AddContainer(m_selectedEntities);
	// s->AddContainer(m_doubleClickLastSelectedEntities);
	// s->AddContainer(m_selectStartEntitiesPositions);
	// s->AddContainer(m_storedEntitiesPositions);
	// s->AddContainer(m_boxes);
}

const char *CTOSZeusModule::GetName() const
{
	return "ModuleZeus";
}

void CTOSZeusModule::Update(float frametime)
{
	if (tos_sv_zeus_update == 0)
		return;

	CTOSActor *pLocalZeus = static_cast<CTOSActor *>(GetPlayer());
	if (!pLocalZeus || !m_local.GetFlag(EFlag::Zeusing))
		return;

	// Если мышка не отображается, но свободная камера не включена и нет модального окна, то отображаем мышку
	if (g_pGame->GetHUD()->GetCursorVisibilityCounter() == 0 || m_local.IsMouseDisplayed() == false)
	{
		if (m_local.m_spaceFreeCam == false && g_pGame->GetHUD()->IsHaveModalHUD() == false)
			m_local.ShowMouse(true);
	}

	// Если свободная камера включена и открыто модальное окно зевса или его вообще нет, то отключаем фильтр блокировки манипуляций с мышкой
	if (m_local.m_spaceFreeCam == true && m_hud.IsModalZeusMenuOnlyOrNot() == true)
	{
		if (g_pGameActions->FilterNoMouse()->Enabled())
			g_pGameActions->FilterNoMouse()->Enable(false);
	}

	auto pMouse = gEnv->pHardwareMouse;
	if (pMouse)
	{
		// Привязка мыши к позиции, когда крутится камера
		///////////////////////////////////////////////////////////////////////
		if (m_local.GetFlag(EFlag::CanRotateCamera) 
			&& m_local.GetFlag(EFlag::Possessing) == false
			&& m_hud.IsModalZeusMenuOnlyOrNot() == true
			&& g_pGame->GetMenu()->IsActive() == false)
		{
			if (m_local.m_anchoredMousePos == Vec2(0, 0))
			{
				Vec2 mousePos;
				pMouse->GetHardwareMousePosition(&mousePos.x, &mousePos.y);

				m_local.m_anchoredMousePos = mousePos;
			}

			pMouse->SetHardwareMousePosition(m_local.m_anchoredMousePos.x, m_local.m_anchoredMousePos.y);
		}
		else
		{
			if (m_local.m_anchoredMousePos != Vec2(0, 0))
				m_local.m_anchoredMousePos.zero();
		}
	}

	if (m_local.m_select)
		m_local.m_mouseDownDurationSec += frametime;
	else
		m_local.m_mouseDownDurationSec = 0.0f;

	m_local.MouseProjectToWorld(m_local.m_mouseRay, m_local.m_worldMousePos, m_local.m_mouseRayEntityFlags, true);
	m_local.m_worldProjectedMousePos = m_local.m_mouseRay.pt;

	// Обработка таймера задержки перемещения выделенных сущностей
	///////////////////////////////////////////////////////////////////////
	if (m_local.m_dragging && m_local.m_draggingMoveStartTimer > 0.0f)
		m_local.m_draggingMoveStartTimer -= frametime;

	if (m_local.m_draggingMoveStartTimer <= 0.0f)
		m_local.m_draggingMoveStartTimer = 0.0f;

	pe_status_dynamics zeus_dyn;
	const auto pZeusPhys = GetPlayer()->GetEntity()->GetPhysics();
	if (pZeusPhys)
		pZeusPhys->GetStatus(&zeus_dyn);

	const bool zeusMoving = zeus_dyn.v.len() > 0.1f;

	// Перемещение боксов выделенных сущностей
	///////////////////////////////////////////////////////////////////////
	if (m_local.m_dragging && !zeusMoving && m_local.m_draggingMoveStartTimer == 0.0f)
	{
		const bool autoEntitiesHeight = tos::console::GetSafeIntVar("tos_sv_zeus_dragging_entities_auto_height", 0) == 1; // Расчет высоты для каждой сущности отдельно
		const auto pClickedEntity = TOS_GET_ENTITY(m_local.m_curClickedEntityId);

		m_local.m_mouseRayEntityFlags = ZEUS_DRAGGING_MOUSE_ENT_FLAGS;
		m_local.m_draggingDelta = m_local.m_worldProjectedMousePos - m_local.m_clickedSelectStartPos;

		for (auto it = m_local.m_selectedEntities.cbegin(); it != m_local.m_selectedEntities.cend(); it++)
		{
			const auto id = *it;

			if (!m_local.UpdateDraggedEntity(id, pClickedEntity, pZeusPhys, m_local.m_boxes, autoEntitiesHeight))
				continue;
		}

		m_local.m_dragTargetId = m_local.GetMouseEntityId();
		if (m_local.m_dragTargetId == m_local.m_curClickedEntityId)
			m_local.m_dragTargetId = 0;
	}
	else if (!m_local.m_dragging)
	{

		for (auto it = m_local.m_selectedEntities.cbegin(); it != m_local.m_selectedEntities.cend(); it++)
		{
			const EntityId id = *it;
			const IEntity *pEntity = TOS_GET_ENTITY(id);
			if (pEntity)
			{
				m_local.m_boxes[id]->wPos = pEntity->GetWorldPos();
				m_local.m_boxes[id]->obb.m33 = Matrix33(pEntity->GetRotation());
			}
		}
	}
	else
	{
		m_local.m_mouseRayEntityFlags = ZEUS_DEFAULT_MOUSE_ENT_FLAGS;
	}

	// Отрисовка границ выделения
	///////////////////////////////////////////////////////////////////////
	if (m_local.m_select && m_local.CanSelectMultiplyWithBox() && !m_local.m_dragging)
	{
		if (IRenderAuxGeom *pGeom = gEnv->pRenderer->GetIRenderAuxGeom())
		{
			// calculate the four selection boundary points
			Vec3 vTopLeft(m_local.m_selectStartPos.x, m_local.m_selectStartPos.y, 0.0f);
			Vec3 vTopRight(m_local.m_mouseIPos.x, m_local.m_selectStartPos.y, 0.0f);
			Vec3 vBottomLeft(m_local.m_selectStartPos.x, m_local.m_mouseIPos.y, 0.0f);
			Vec3 vBottomRight(m_local.m_mouseIPos.x, m_local.m_mouseIPos.y, 0.0f);

			gEnv->pRenderer->Set2DMode(true, gEnv->pRenderer->GetWidth(), gEnv->pRenderer->GetHeight());

			// set boundary color: white
			ColorB col(255, 255, 255, 255);

			pGeom->DrawLine(vTopLeft, col, vTopRight, col);
			pGeom->DrawLine(vTopRight, col, vBottomRight, col);
			pGeom->DrawLine(vTopLeft, col, vBottomLeft, col);
			pGeom->DrawLine(vBottomLeft, col, vBottomRight, col);

			gEnv->pRenderer->Set2DMode(false, 0, 0);
		}
	}

	// Отрисовка флэш иконок под сущностями
	///////////////////////////////////////////////////////////////////////
	m_local.UpdateUnitIcons(pLocalZeus);
	m_local.UpdateOrderIcons();

	// Отрисовка квадрата выделенных сущностей
	///////////////////////////////////////////////////////////////////////
	const auto &color = ColorB(255, 255, 255, 255);
	const auto mode = eBBD_Faceted;
	const auto solid = false;

	for (auto it = m_local.m_boxes.cbegin(); it != m_local.m_boxes.cend(); it++)
	{
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawOBB(it->second->obb, it->second->wPos, solid, color, mode);
	}

	// Отрисовка отладки
	///////////////////////////////////////////////////////////////////////
	m_local.UpdateDebug(zeusMoving, zeus_dyn.v);
}

void CTOSZeusModule::Serialize(TSerialize ser)
{
}

int CTOSZeusModule::GetDebugLog() const
{
	return m_debugLogMode;
}

CScriptableBase *CTOSZeusModule::GetScriptBind()
{
    return nullptr;
}

bool CTOSZeusModule::IsPhysicsAllowed(const IEntity *pEntity)
{
	if (!pEntity)
		return false;

	IPhysicalEntity *physEnt = pEntity->GetPhysics();
	if (!physEnt)
		return false;

	// допустимые сущности
	const auto type = physEnt->GetType();
	if (!(type == PE_LIVING || type == PE_RIGID || type == PE_STATIC || type == PE_WHEELEDVEHICLE || type == PE_ARTICULATED))
		return false;
}

CTOSPlayer *CTOSZeusModule::GetPlayer() const
{
	return static_cast<CTOSPlayer *>(TOS_GET_CLIENT_ACTOR);
}

CTOSZeusModule::ClientServer &CTOSZeusModule::GetClientServer()
{
	return m_clientserver;
}

CTOSZeusModule::Local &CTOSZeusModule::GetLocal()
{
	return m_local;
}

CTOSZeusModule::HUD &CTOSZeusModule::GetHUD()
{
	return m_hud;
}

//------------------------------------------------------------------------
void CTOSZeusModule::DumpModuleInfo()
{
	CryLog("Info for %s", GetName());
	CryLog("=====================================");
	CryLog("Flags:");
	{
		CryLog("  Zeusing: %i", (int)m_local.GetFlag(CTOSZeusModule::EFlag::Zeusing));
		CryLog("  Can use mouse: %i", (int)m_local.GetFlag(CTOSZeusModule::EFlag::CanUseMouse));
		CryLog("  Possessing: %i", (int)m_local.GetFlag(CTOSZeusModule::EFlag::Possessing));
		CryLog("  Can rotate camera: %i", (int)m_local.GetFlag(CTOSZeusModule::EFlag::CanRotateCamera));
	}

	CryLog("=====================================");
}