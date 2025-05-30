#include "StdAfx.h"
#include "ZeusModule.h"
#include "ZeusSynchronizer.h"
#include <TheOtherSideMP/Helpers/TOS_Entity.h>
#include <TheOtherSideMP/Helpers/TOS_Vehicle.h>
#include <TheOtherSideMP/Helpers/TOS_Console.h>
#include <TheOtherSideMP/Helpers/TOS_Inventory.h>
#include <TheOtherSideMP/Helpers/TOS_Script.h>
#include <GameActions.h>

static bool EntityIsSimilarToEntity(IEntity* pFirstEntity, IEntity* pSecondEntity)
{
	if (!pFirstEntity || !pSecondEntity)
		return false;

	if (pFirstEntity->GetClass() != pSecondEntity->GetClass())
		return false;

	const auto pFirstArchetype = pFirstEntity->GetArchetype();
	const auto pSecondArchetype = pSecondEntity->GetArchetype();
	if (pFirstArchetype && pSecondArchetype)
	{
		if (string(pFirstArchetype->GetName()) != pSecondArchetype->GetName())
			return false;
	}

	int firstSpecies = -1;
	int secondSpecies = -1;

	tos::script::GetEntityProperty(pFirstEntity, "species", firstSpecies);
	tos::script::GetEntityProperty(pSecondEntity, "species", secondSpecies);

	if (firstSpecies != secondSpecies)
		return false;

	auto pFirstActor = static_cast<CTOSActor*>(TOS_GET_ACTOR(pFirstEntity->GetId()));
	auto pSecondActor = static_cast<CTOSActor*>(TOS_GET_ACTOR(pSecondEntity->GetId()));
	if (pFirstActor && pSecondActor)
	{
		auto pFirstVehicle = pFirstActor->GetLinkedVehicle();
		auto pSecondVehicle = pSecondActor->GetLinkedVehicle();
		if (pFirstVehicle != pSecondVehicle)
			return false;
	}

	return true;
}


bool CTOSZeusModule::OnInputEvent(const SInputEvent& event)
{
	if (!gEnv->bClient)
		return false;

	if (!GetPlayer() || !m_local.GetFlag(EFlag::Zeusing))
		return false;

	if (gEnv->pConsole->IsOpened())
		return false;

	if (event.deviceId == EDeviceId::eDI_Keyboard)
	{
		if (event.keyId == EKeyId::eKI_LCtrl)
		{
			if (event.state == eIS_Pressed)
				m_local.m_ctrlModifier = true;
			else if (event.state == eIS_Released)
				m_local.m_ctrlModifier = false;
		}
		else if (event.keyId == EKeyId::eKI_C)
		{
			if (event.state == eIS_Pressed)
			{
				if (m_local.m_ctrlModifier && !m_local.m_copying)
				{
					m_local.m_copying = true;
					m_local.ExecuteCommand(ECommand::CopySelected);
				}
			}
		}
		else if (event.keyId == EKeyId::eKI_LAlt)
		{
			if (event.state == eIS_Pressed)
			{
				m_local.m_altModifier = true;
				m_local.m_select = false;
			}
			else if (event.state == eIS_Released)
			{
				if (m_local.m_altModifier)
				{
					for (auto it = m_local.m_selectedEntities.cbegin();
						it != m_local.m_selectedEntities.cend();
						it++)
					{
						const bool movedOnHeight = m_local.m_draggingDelta.len() > 1;

						auto pVehicle = TOS_GET_VEHICLE(*it);
						if (pVehicle && tos::vehicle::IsAir(pVehicle) && movedOnHeight)
						{
							SVehicleMovementEventParams params;
							params.fValue = pVehicle->GetEntity()->GetWorldPos().z; // ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜

							tos::vehicle::BroadcastMovementEvent(pVehicle, IVehicleMovement::eVME_WarmUpEngine, params);
						}
					}
				}

				m_local.m_altModifier = false;
			}
		}
		else if (event.keyId == EKeyId::eKI_Z)
		{
			if (event.state == eIS_Pressed)
				m_local.m_debugZModifier = true;
			else if (event.state == eIS_Released)
			{
				if (m_local.m_debugZModifier)
				{
					m_local.m_debugZModifier = false;

					for (auto it = m_local.m_selectedEntities.cbegin();
						it != m_local.m_selectedEntities.cend();
						it++)
					{
						if (!m_local.SelectionFilter(*it))
							it = m_local.DeselectEntity(*it);
					}
				}
			}
		}
		else if (event.keyId == EKeyId::eKI_LShift)
		{
			if (event.state == eIS_Pressed)
				m_local.m_shiftModifier = true;
			else if (event.state == eIS_Released)
				m_local.m_shiftModifier = false;
		}
		else if (event.keyId == EKeyId::eKI_End)
		{
			m_local.ExecuteCommand(ECommand::KillSelected);
		}
		else if (event.keyId == EKeyId::eKI_Delete)
		{
			m_local.ExecuteCommand(ECommand::RemoveSelected);
		}
		else if (event.keyId == EKeyId::eKI_P)
		{
			if (event.state == eAAM_OnPress)
			{
				m_hud.ShowZeusMenu(!m_hud.m_menuShow);
			}
		}
		else if (event.keyId == EKeyId::eKI_Space)
		{
			if (event.state == eAAM_OnPress)
			{
				if (m_hud.IsModalZeusMenuOnlyOrNot() == true)
				{
					m_local.m_spaceFreeCam = !m_local.m_spaceFreeCam;
					m_local.SetFlag(EFlag::CanRotateCamera, m_local.m_spaceFreeCam);
					m_local.ShowMouse(!m_local.m_spaceFreeCam);
				}
			}
		}
		else if (event.keyId == EKeyId::eKI_X)
		{
			if (event.state == EInputState::eIS_Pressed)
			{
				auto it = m_local.m_selectedEntities.cbegin();
				auto end = m_local.m_selectedEntities.cend();
				for (; it != end; it++)
					m_local.StopOrder(*it);
			}
		}
	}
	else if (event.deviceId == EDeviceId::eDI_Mouse)
	{
		if (m_local.GetFlag(EFlag::CanUseMouse) == false)
			return false;

		if (event.keyId == EKeyId::eKI_Mouse3) // ˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜
		{
			if (m_local.m_spaceFreeCam == false && m_hud.IsModalZeusMenuOnlyOrNot() == true)
			{
				const bool down = event.state == eIS_Down;
				g_pGameActions->FilterNoMouse()->Enable(down == false);
				m_local.SetFlag(EFlag::CanRotateCamera, down);
			}
		}
		else if (event.keyId == EKeyId::eKI_Mouse2) // ˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜
		{
			if (event.state == EInputState::eIS_Pressed)
			{
				m_local.ExecuteCommand(ECommand::OrderSelected);
			}
		}
	}

	return true;
}

bool CTOSZeusModule::OnInputEventUI(const SInputEvent& event)
{
	return false;
}

void CTOSZeusModule::OnHardwareMouseEvent(int iX, int iY, EHARDWAREMOUSEEVENT eHardwareMouseEvent)
{
	if (gEnv->pConsole->IsOpened())
		return;

	m_local.m_mouseIPos.x = iX;
	m_local.m_mouseIPos.y = iY;

	int mod_iY = gEnv->pRenderer->GetHeight() - iY;
	gEnv->pRenderer->UnProjectFromScreen(
		iX,
		mod_iY,
		0.0f,
		&m_local.m_worldMousePos.x,
		&m_local.m_worldMousePos.y,
		&m_local.m_worldMousePos.z);

	auto pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		auto eCursorState = SFlashCursorEvent::eCursorMoved;
		if (HARDWAREMOUSEEVENT_LBUTTONDOWN == eHardwareMouseEvent)
		{
			eCursorState = SFlashCursorEvent::eCursorPressed;
		}
		else if (HARDWAREMOUSEEVENT_LBUTTONUP == eHardwareMouseEvent)
		{
			eCursorState = SFlashCursorEvent::eCursorReleased;
		}

		if (m_hud.m_animZeusUnitIcons.IsLoaded())
		{
			int x(iX), y(iY);
			m_hud.m_animZeusUnitIcons.GetFlashPlayer()->ScreenToClient(x, y);
			m_hud.m_animZeusUnitIcons.GetFlashPlayer()->SendCursorEvent(SFlashCursorEvent(eCursorState, x, y));
		}

		if (m_hud.m_animZeusMenu.IsLoaded())
		{
			int x(iX), y(iY);
			m_hud.m_animZeusMenu.GetFlashPlayer()->ScreenToClient(x, y);
			m_hud.m_animZeusMenu.GetFlashPlayer()->SendCursorEvent(SFlashCursorEvent(eCursorState, x, y));
		}

		// Òîëüêî ñ îòêðûòûì ìåíþ çåâñà èëè áåç äðóãèõ ìîäàëüíûõ îêîí ìîæíî ìàíèïóëèðîâàòü ñóùíîñòÿìè
		if (m_hud.IsModalZeusMenuOnlyOrNot() == true && m_local.GetFlag(EFlag::CanUseMouse))
		{
			if (eHardwareMouseEvent == HARDWAREMOUSEEVENT_LBUTTONDOUBLECLICK)
			{
				m_local.m_doubleClick = true;
			}
			else if (eHardwareMouseEvent == HARDWAREMOUSEEVENT_LBUTTONDOWN)
			{
				// åñëè íå âêëþ÷åíà ñâîáîäíàÿ êàìåðà, òî èäåò âûäåëåíèå ñóùíîñòåé
				if (m_local.m_spaceFreeCam == false)
				{
					m_local.m_select = true;
					m_local.m_selectStartPos = Vec2i(iX, iY);

					if (m_hud.m_menuSpawnHandling == false)
					{
						m_local.HandleOnceSelection(m_local.GetMouseEntityId());
					}
				}
			}
			else if (eHardwareMouseEvent == HARDWAREMOUSEEVENT_LBUTTONUP)
			{
				m_local.m_select = false;
				m_local.m_selectStopPos = Vec2i(iX, iY);

				if (!m_local.m_dragging && m_local.CanSelectMultiplyWithBox())
					m_local.GetSelectedEntities();
				else
				{
					// ˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ c ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜
					if (m_local.m_ctrlModifier)
					{
						if (!m_local.m_dragging &&
							!m_local.m_doubleClick &&
							m_local.m_curClickedEntityId != 0)
						{
							if (m_local.m_selectedEntities.count(m_local.m_curClickedEntityId) > 0)
							{
								// if (delta <= 0.15f)
								m_local.DeselectEntity(m_local.m_curClickedEntityId);
							}
							else
							{
								if (m_local.SelectionFilter(m_local.m_curClickedEntityId))
									m_local.SelectEntity(m_local.m_curClickedEntityId);
							}
						}
					}
				}

				if (m_local.m_doubleClick)
				{
					const auto pClickedEntity = TOS_GET_ENTITY(m_local.m_curClickedEntityId);
					if (pClickedEntity)
					{
						SmartScriptTable props;
						int clickedSpecies = -1;
						auto pTable = pClickedEntity->GetScriptTable();
						if (pTable)
						{
							pTable->GetValue("Properties", props);
							props->GetValue("species", clickedSpecies);
						}

						const IActor* pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
						if (!pClientActor)
							return;

						const auto clickedIter = stl::binary_find(
							m_local.m_doubleClickLastSelectedEntities.cbegin(),
							m_local.m_doubleClickLastSelectedEntities.cend(),
							m_local.m_curClickedEntityId);

						const bool clickedSelected = clickedIter != m_local.m_doubleClickLastSelectedEntities.cend();
						if (clickedSelected)
						{
							// ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜
							for (auto it = m_local.m_doubleClickLastSelectedEntities.begin();
								it != m_local.m_doubleClickLastSelectedEntities.end();)
							{
								auto pEntity = TOS_GET_ENTITY(*it);

								if (EntityIsSimilarToEntity(pEntity, pClickedEntity) ||
									pEntity == pClickedEntity)
								{
									m_local.DeselectEntity(*it);
									it = m_local.m_doubleClickLastSelectedEntities.erase(it);
								}
								else
								{
									it++;
								}
							}
						}
						else
						{
							// ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜ ˜˜˜˜ ˜˜˜˜˜˜

							IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
							while (!pIt->IsEnd())
							{
								if (IEntity* pEntity = pIt->Next())
								{
									const auto id = pEntity->GetId();

									if (!m_local.SelectionFilter(id))
										continue;

									if (pEntity->IsHidden())
										continue;

									if (!IsPhysicsAllowed(pEntity))
										continue;

									// ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜˜˜˜
									if (id == pClientActor->GetEntityId())
										continue;

									AABB worldBounds;
									pEntity->GetWorldBounds(worldBounds);

									// skip further calculations if the entity is not visible at all...
									if (gEnv->pSystem->GetViewCamera().IsAABBVisible_F(worldBounds) == CULL_EXCLUSION)
										continue;

									if (!EntityIsSimilarToEntity(pEntity, pClickedEntity))
										continue;

									m_local.SelectEntity(id);
									m_local.m_doubleClickLastSelectedEntities.insert(id);
								}
							}
						}
					}

					m_local.m_doubleClick = false;
				}

				// ˜˜˜˜˜ ˜˜˜ ˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜..
				// ˜˜˜˜˜˜˜˜˜˜ ˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜
				if (m_local.m_copying)
				{
					for (auto it = m_local.m_selectedEntities.cbegin();
						it != m_local.m_selectedEntities.cend();
						it++)
					{
						auto pEntity = TOS_GET_ENTITY(*it);
						if (pEntity)
						{
							CTOSZeusSynchronizer::NetHideParams hideParams;
							CTOSZeusSynchronizer::NetMakeHostileParams makeHostileParams;

							hideParams.bHide = false;
							hideParams.id = pEntity->GetId();

							makeHostileParams.bHostile = true;
							makeHostileParams.id = pEntity->GetId();

							GetSynchronizer()->RMISend(
								CTOSZeusSynchronizer::SvRequestHideEntity(),
								hideParams,
								eRMI_ToServer);
							GetSynchronizer()->RMISend(
								CTOSZeusSynchronizer::SvRequestAIMakeHostile(),
								makeHostileParams,
								eRMI_ToServer);
						}
					}
				}
				m_local.m_copying = false;

				if (m_local.m_dragging)
				{
					for (auto it = m_local.m_selectedEntities.begin();
						it != m_local.m_selectedEntities.end();)
					{
						const EntityId selectedEntId = *it;
						// ˜˜˜˜˜˜˜˜, ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜
						auto pSelectedEntity = TOS_GET_ENTITY(selectedEntId);
						if (!pSelectedEntity)
						{
							it++;
							continue;
						}

						bool moveSelectedEnt = true;
						bool needDeselect = false;

						// ˜˜˜˜˜˜˜˜, ˜˜ ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜
						const auto pDragTarget = TOS_GET_ENTITY(m_local.m_dragTargetId);
						if (pDragTarget)
						{
							auto pSelectedActor = static_cast<CTOSActor*>(TOS_GET_ACTOR(selectedEntId));
							auto pSelectedItem = static_cast<CItem*>(TOS_GET_ITEM(selectedEntId));

							const EntityId dragTargetId = pDragTarget->GetId();

							if (pSelectedActor)
							{
								// Actor ˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜ Vehicle
								IVehicle* pDragVehicle = TOS_GET_VEHICLE(dragTargetId);
								if (pDragVehicle)
								{
									// tos::vehicle::Enter(pSelectedActor, pDragVehicle, true);
									bool enter = ClientServer::DispatchEnterVehicle(
										pSelectedActor,
										pDragVehicle,
										true);

									if (enter)
									{
										moveSelectedEnt = false;
										needDeselect = true;
									}
								}
							}
							else if (pSelectedItem)
							{
								// Item ˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜ Actor
								auto pDragActor = static_cast<CTOSActor*>(TOS_GET_ACTOR(dragTargetId));
								if (pDragActor && pDragActor->GetHealth() > 0)
								{
									if (pDragActor->PickUpItem(pSelectedItem->GetEntityId(), true))
									{
										tos::inventory::SelectItemByClass(pDragActor,
											pSelectedItem->GetEntity()->GetClass()->GetName());

										moveSelectedEnt = false;
										needDeselect = true;
									}
								}
							}
						}

						// ˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜ ˜˜˜˜ ˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜
						// auto pPhys = pSelectedEntity->GetPhysics();
						// if (pPhys)
						//{
						//	pe_action_awake awake;
						//	awake.bAwake = 1;
						//	pPhys->Action(&awake);
						//}

						if (moveSelectedEnt)
						{
							// ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜˜˜˜
							const auto& pBox = m_local.m_boxes[pSelectedEntity->GetId()];
							// pSelectedEntity->SetWorldTM(Matrix34::CreateTranslationMat(pBox->wPos));
							// pSelectedEntity->SetRotation(Quat(pBox->obb.m33));

							CTOSZeusSynchronizer::NetTransformParams params;
							params.pos = pBox->wPos;
							params.dir = pBox->obb.m33.GetColumn1();
							params.id = pSelectedEntity->GetId();

							GetSynchronizer()->RMISend(
								CTOSZeusSynchronizer::SvRequestTransformEntity(),
								params,
								eRMI_ToServer);
						}

						if (needDeselect)
						{
							it = m_local.DeselectEntity(pSelectedEntity->GetId());
							// deselectionSet.insert(pSelectedEntity->GetId());
						}
						else
						{
							it++;
						}
					}
				}

				m_local.m_dragTargetId = 0;
				m_local.m_dragging = false;

				if (m_hud.m_menuSpawnHandling)
				{
					if (m_local.m_curClickedEntityId != 0)
					{
						auto pEntity = TOS_GET_ENTITY(m_local.m_curClickedEntityId);
						if (pEntity)
						{
							CTOSZeusSynchronizer::NetHideParams params;
							params.bHide = false;
							params.id = pEntity->GetId();

							GetSynchronizer()->RMISend(
								CTOSZeusSynchronizer::SvRequestHideEntity(),
								params,
								eRMI_ToServer);
						}
						else
						{
							m_local.m_curClickedEntityId = 0;
						}
					}
				}
				m_hud.m_menuSpawnHandling = false;
			}
			else if (eHardwareMouseEvent == HARDWAREMOUSEEVENT_MOVE)
			{
				// ˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜ ˜˜˜˜˜˜˜˜
				const auto clickedIter = stl::binary_find(
					m_local.m_selectedEntities.begin(),
					m_local.m_selectedEntities.end(),
					m_local.m_curClickedEntityId);

				const bool clickedSelected = clickedIter != m_local.m_selectedEntities.end();

				// ˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜. True - ˜˜
				const bool clickedOveredByMouse = m_local.m_mouseOveredEntityId == m_local.m_curClickedEntityId;

				if ((m_local.m_select) &&
					m_local.m_curClickedEntityId != 0 &&
					clickedSelected &&
					clickedOveredByMouse)
				{
					// ˜˜˜˜˜ ˜˜˜ ˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜...
					if (m_local.m_dragging == false)
					{
						// ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜
						m_local.SaveEntitiesStartPositions();

						// ˜˜˜˜˜˜ ˜˜˜˜˜˜˜
						m_local.m_draggingMoveStartTimer = tos::console::GetSafeFloatVar("tos_sv_zeus_dragging_move_start_delay", 0.05f);
					}

					m_local.m_dragging = true;
				}
			}
		}
	}
}