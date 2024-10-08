/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once
#include "IActorSystem.h"
#include "IVehicleSystem.h"
//#include "../Control System/ControlSystem.h"
//#include "../Conqueror/ConquerorSystem.h"

namespace TOS_Vehicle
{
	inline void BroadcastMovementEvent(IVehicle* pVehicle, IVehicleMovement::EVehicleMovementEvent event, const SVehicleMovementEventParams& params)
	{
		if (!pVehicle)
			return;

		auto pMovement = pVehicle->GetMovement();
		if (!pMovement)
			return;

		pMovement->OnEvent(event, params);
	}

	inline int RequestFreeSeatIndex(IVehicle* pVehicle)
	{
		auto freeSeatIndex = -1;

		if (pVehicle)
		{
			//HSCRIPTFUNCTION RequestSeatFunc = nullptr;
			//const auto      pTable = pVehicle->GetEntity()->GetScriptTable();

			//if (pTable && pTable->GetValue("TOSRequestSeat", RequestSeatFunc))
			//{
			//	Script::CallReturn(gEnv->pScriptSystem, RequestSeatFunc, pTable, freeSeatIndex);
			//	gEnv->pScriptSystem->ReleaseFunc(RequestSeatFunc);
			//}

			const int seatCount = pVehicle->GetSeatCount();
			for (int i = 0; i <= seatCount; i++)
			{
				auto pSeat = pVehicle->GetSeatById(i);
				if (pSeat && pSeat->GetPassenger() == 0)
				{
					freeSeatIndex = i;
					break;
				}
			}
		}

		//if (freeSeatIndex == -1)
		//CryLogAlways("%s[C++][WARNING][RequestFreeSeatIndex return -1]", TOS_COLOR_YELLOW);

		return freeSeatIndex;
	}

	inline int RequestGunnerSeatIndex(const IVehicle* pVehicle)
	{
		int gunnerSeatIndex = -1;

		if (pVehicle)
		{
			HSCRIPTFUNCTION RequestSeatFunc = nullptr;
			const auto      pTable = pVehicle->GetEntity()->GetScriptTable();

			if (pTable && pTable->GetValue("TOSRequestGunnerSeat", RequestSeatFunc))
			{
				Script::CallReturn(gEnv->pScriptSystem, RequestSeatFunc, pTable, gunnerSeatIndex);
				gEnv->pScriptSystem->ReleaseFunc(RequestSeatFunc);

				//CryLogAlways("GUNNER SEAT %i", gunnerSeatIndex);
			}
		}

		//if (gunnerSeatIndex == -1)
		//	CryLogAlways("%s[C++][WARNING][RequestGunnerSeatIndex return -1]", TOS_COLOR_YELLOW);

		return gunnerSeatIndex;
	}

	inline void Exit(const IActor* pActor, bool transitionEnabled, bool force, Vec3 exitPos = Vec3(0, 0, 0))
	{
		if (!pActor)
			return;

		const auto pVeh = pActor->GetLinkedVehicle();
		if (!pVeh)
			return;

		const auto pSeat = pVeh->GetSeatForPassenger(pActor->GetEntityId());
		if (!pSeat)
			return;

		pSeat->Exit(transitionEnabled, force, exitPos);
	}

	inline bool Enter(const IActor* pActor, IVehicle* pVehicle, bool fast)
	{
		if (!pActor || !pActor->GetEntity() || pActor && pActor->GetHealth() < 0)
			return false;

		if (!pVehicle || pVehicle && pVehicle->IsDestroyed())
			return false;

		const int seatIndex = RequestFreeSeatIndex(pVehicle);
		auto pSeat = pVehicle->GetSeatById(seatIndex);
		auto pActorAI = pActor->GetEntity()->GetAI();

		if (pActor->IsPlayer() || fast)
		{
			HSCRIPTFUNCTION EnterVehicle = nullptr;
			auto pTable = pVehicle->GetEntity()->GetScriptTable();
			if (pTable && pTable->GetValue("EnterVehicle", EnterVehicle))
			{
				if (pSeat)
				{
					pSeat->Enter(pActor->GetEntityId(), false);
					Script::CallMethod(pTable, EnterVehicle, ScriptHandle(pActor->GetEntityId()), seatIndex, true);
					gEnv->pScriptSystem->ReleaseFunc(EnterVehicle);
				}
			}
		}
		else if (gEnv->bServer && gEnv->pAISystem && pActorAI)
		{
			IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();

			pData->fValue = (float)seatIndex;
			pData->nID = pVehicle->GetEntityId();
			pData->iValue2 = false;
			pActorAI->CastToIAIActor()->SetSignal(10, "ACT_ENTERVEHICLE", pActor->GetEntity(), pData);
		}


		return true;
	}

	inline bool ActorIsPassenger(const IActor* pActor)
	{
		if (pActor)
		{
			const auto pVehicle = pActor->GetLinkedVehicle();
			if (pVehicle)
			{
				const auto pSeat = pVehicle->GetSeatForPassenger(pActor->GetEntityId());
				if (pSeat)
					return !pSeat->IsDriver() && !pSeat->IsGunner();
			}
		}

		return false;
	}

	inline bool ActorIsDriver(const IActor* pActor)
	{
		if (pActor)
		{
			const auto pVehicle = pActor->GetLinkedVehicle();
			if (pVehicle)
			{
				const auto pSeat = pVehicle->GetSeatForPassenger(pActor->GetEntityId());
				if (pSeat)
					return pSeat->IsDriver();
			}
		}

		return false;
	}

	inline bool ActorIsGunner(const IActor* pActor)
	{
		if (pActor)
		{
			const auto pVehicle = pActor->GetLinkedVehicle();
			if (pVehicle)
			{
				const auto pSeat = pVehicle->GetSeatForPassenger(pActor->GetEntityId());
				if (pSeat)
					return pSeat->IsGunner();
			}
		}

		return false;
	}

	inline bool ActorInVehicle(const IActor* pActor)
	{
		if (pActor)
		{
			const auto pVehicle = pActor->GetLinkedVehicle();
			if (pVehicle)
			{
				const auto pSeat = pVehicle->GetSeatForPassenger(pActor->GetEntityId());
				if (pSeat)
					return true;
			}
		}

		return false;
	}

	inline IVehicle* GetVehicle(const IActor* pActor)
	{
		if (pActor)
		{
			const auto pVehicle = pActor->GetLinkedVehicle();
			if (pVehicle)
			{
				const auto pSeat = pVehicle->GetSeatForPassenger(pActor->GetEntityId());
				if (pSeat)
					return pVehicle;
			}
		}

		return nullptr;
	}

	inline IVehicle* GetVehicle(const IEntity* pEntity)
	{
		if (pEntity)
			return g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId());

		return nullptr;
	}

	inline void ChangeSeat(const IActor* pActor, int seatIndex, bool isAnimationEnabled)
	{
		if (pActor && pActor->GetLinkedVehicle())
		{
			HSCRIPTFUNCTION ActorChangeSeat = nullptr;
			const auto      pTable = pActor->GetEntity()->GetScriptTable();

			if (pTable && pTable->GetValue("ActorChangeSeat", ActorChangeSeat))
				Script::CallMethod(pTable, ActorChangeSeat, seatIndex, isAnimationEnabled);
		}
	}

	inline void Destroy(IVehicle* pVehicle)
	{
		if (pVehicle)
		{
			const auto  id = pVehicle->GetEntityId();
			const auto& pos = pVehicle->GetEntity()->GetWorldPos();

			pVehicle->OnHit(id, id, 18000, pos, 1, "normal", false);
		}
	}

	inline IAIObject* GetAI(const IVehicle* pVehicle)
	{
		if (pVehicle)
			return pVehicle->GetEntity()->GetAI();

		return nullptr;
	}

	inline bool IsHavePassengers(IVehicle* pVehicle)
	{
		if (!pVehicle)
			return false;

		auto       passengerCount = 0;
		const auto seats = pVehicle->GetSeatCount();

		for (auto i = 0; i < seats; i++)
		{
			const auto pSeat = pVehicle->GetSeatById(i);
			if (!pSeat)
				continue;

			if (pSeat->IsDriver())
				continue;

			if (pSeat->IsGunner())
				continue;

			passengerCount++;
		}

		return passengerCount != 0;
	}

	inline bool DriverSelectWeapon(const IVehicle* pVehicle, int index)
	{
		if (!pVehicle)
			return false;

		HSCRIPTFUNCTION DriverSelectWeaponFunc = nullptr;
		const auto      pTable = pVehicle->GetEntity()->GetScriptTable();

		if (pTable && pTable->GetValue("DriverSelectWeaponByIndex", DriverSelectWeaponFunc))
			return Script::CallMethod(pTable, DriverSelectWeaponFunc, index);

		return false;
	}

	inline int GetSeatWeaponCount(const IVehicle* pVehicle, const TVehicleSeatId seatId)
	{
		if (!pVehicle)
			return 0;

		int count = 0;

		HSCRIPTFUNCTION GetSeatWeaponCountFunc = nullptr;
		const auto      pTable = pVehicle->GetEntity()->GetScriptTable();

		if (pTable && pTable->GetValue("GetSeatWeaponCount", GetSeatWeaponCountFunc))
		{
			Script::CallReturn(gEnv->pScriptSystem, GetSeatWeaponCountFunc, pTable, seatId, count);
			gEnv->pScriptSystem->ReleaseFunc(GetSeatWeaponCountFunc);
		}

		return count;
	}

	inline int GetSeatWeaponCount(const IActor* pActor)
	{
		if (!pActor)
			return 0;

		const auto pVehicle = pActor->GetLinkedVehicle();
		if (!pVehicle)
			return 0;

		const auto pSeat = pVehicle->GetSeatForPassenger(pActor->GetEntityId());
		if (!pSeat)
			return 0;

		const int count = GetSeatWeaponCount(pVehicle, pSeat->GetSeatId());

		return count;
	}

	inline bool IsAir(IVehicle* pVehicle)
	{
		if (!pVehicle)
			return false;

		return pVehicle->GetMovement()->GetMovementType() == IVehicleMovement::eVMT_Air;
	}

	inline bool IsLand(IVehicle* pVehicle)
	{
		if (!pVehicle)
			return false;

		return pVehicle->GetMovement()->GetMovementType() == IVehicleMovement::eVMT_Land;
	}

	//inline bool IsPLV(IVehicle* pVehicle)
	//{
	//	const auto pConqueror = g_pControlSystem->GetConquerorSystem();
	//	if (!pConqueror)
	//		return false;

	//	if (!pVehicle)
	//		return false;

	//	const auto classType = pConqueror->GetVehicleClassType(pVehicle);

	//	return classType == eVCT_PLV;
	//}

	//inline bool IsCar(const IVehicle* pVehicle)
	//{
	//	const auto pConqueror = g_pControlSystem->GetConquerorSystem();
	//	if (!pConqueror)
	//		return false;

	//	if (!pVehicle)
	//		return false;

	//	const auto classType = pConqueror->GetVehicleClassType(pVehicle);

	//	return classType == eVCT_Cars;
	//}

	//inline bool IsTank(IVehicle* pVehicle)
	//{
	//	const auto pConqueror = g_pControlSystem->GetConquerorSystem();
	//	if (!pConqueror)
	//		return false;

	//	if (!pVehicle)
	//		return false;

	//	const auto classType = pConqueror->GetVehicleClassType(pVehicle);

	//	return classType == eVCT_Tanks;
	//}

	inline bool IsSea(IVehicle* pVehicle)
	{
		if (!pVehicle)
			return false;

		return pVehicle->GetMovement()->GetMovementType() == IVehicleMovement::eVMT_Sea;
	}

	inline bool IsHaveFreeSeats(IVehicle* pVehicle)
	{
		if (!pVehicle)
			return false;

		const unsigned seatsCount = pVehicle->GetSeatCount();
		const int passengerCount = pVehicle->GetStatus().passengerCount;

		return passengerCount < seatsCount;
	}

	inline float GetEnterRadius(IVehicle* pVehicle)
	{
		if (!pVehicle)
			return 0.0f;

		AABB bounds;
		pVehicle->GetEntity()->GetWorldBounds(bounds);

		// rad < 4 --> rad = 4 or rad > 10 --> rad = 10

		float maximum = 10.0f;
		if (IsAir(pVehicle))
			if (!pVehicle->GetMovement()->IsEngineDisabled())
				maximum += 5.0f;

		return min(max(4.0f, bounds.GetRadius()), maximum);
	}
}
