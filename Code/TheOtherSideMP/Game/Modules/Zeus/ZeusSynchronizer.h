/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once

#include <IGameObject.h>
#include "TheOtherSideMP/Game/Modules/GenericSynchronizer.h"

/**
 * \brief TOS Zeus Module сетевой синхронизатор
 * \note Обеспечивает вызов RMI'шек, необходимых для сетевой синхронизации работы модуля
 */
class CTOSZeusSynchronizer : public CTOSGenericSynchronizer  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	struct NetSpawnParams
	{
		int playerChannelId;
		string spawnedName;
		string className;
		string archetypeName;
		Vec3 pos;
		Vec3 dir;

		NetSpawnParams()
			:
			playerChannelId(0),
			pos(ZERO),
			dir(ZERO)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("playerChannelId", playerChannelId, 'i8');
			ser.Value("spawnedName", spawnedName, 'stab');
			ser.Value("className", className, 'stab');
			ser.Value("archetypeName", archetypeName, 'stab');
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir0');
		}
	};

	struct NetCopyParams
	{
		int playerChannelId;
		EntityId copiedId;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("playerChannelId", playerChannelId, 'i8');
			ser.Value("copiedId", copiedId, 'eid');
		}
	};
	
	struct NetSpawnedInfo
	{
		EntityId spawnedId;
		Vec3 spawnedPos;

		NetSpawnedInfo()
			:
			spawnedId(0),
			spawnedPos(ZERO)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("spawnedId", spawnedId, 'eid');
			ser.Value("spawnedPos", spawnedPos, 'wrld');
		}
	};

	struct NetCopiedInfo
	{
		EntityId originalId;
		EntityId copiedId;
		Vec3 copiedPos;

		NetCopiedInfo()
			:
			originalId(0),
			copiedId(0),
			copiedPos(ZERO)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("originalId", originalId, 'eid');
			ser.Value("copiedId", copiedId, 'eid');
			ser.Value("copiedPos", copiedPos, 'wrld');
		}
	};

	struct NetMakeParams
	{
		int playerChannelId;
		bool bMake;
		string desiredTeam;

		NetMakeParams() :
			playerChannelId(0),
			bMake(false)
		{};			
		
		NetMakeParams(int _playerChannelId, bool _bmake) :
			playerChannelId(_playerChannelId),
			bMake(_bmake)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("playerChannelId", playerChannelId, 'i8');
			ser.Value("bMake", bMake, 'bool');
			ser.Value("desiredTeam", desiredTeam, 'stab');
		}
	};

	struct NetTransformParams
	{
		EntityId id;
		Vec3 pos;
		Vec3 dir;

		NetTransformParams()
			:
			id(ZERO),
			pos(ZERO),
			dir(ZERO)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'eid');
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir0');
		}
	};	
	
	struct NetRemoveParams
	{
		EntityId id;

		NetRemoveParams()
			:
			id(ZERO)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'eid');
		}
	};		
	
	struct NetKillParams
	{
		EntityId targetId;
		EntityId shooterId;

		NetKillParams()
			:
			targetId(ZERO),
			shooterId(ZERO)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("targetId", targetId, 'eid');
			ser.Value("shooterId", shooterId, 'eid');
		}
	};	
	
	struct NetExecuteOrderParams
	{
		EntityId id;
		EntityId targetId;
		Vec3 pos;
		int index;
		int maxCount;
		int goalPipeId;

		NetExecuteOrderParams()
			:
			id(ZERO),
			targetId(ZERO),
			pos(ZERO),
			index(ZERO),
			maxCount(ZERO),
			goalPipeId(ZERO)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'eid');
			ser.Value("targetId", targetId, 'eid');
			ser.Value("pos", pos, 'wrld');
			ser.Value("index", index, 'i8');
			ser.Value("maxCount", maxCount, 'i8');
			ser.Value("goalPipeId", goalPipeId, 'i8');
		}
	};	
	
	struct NetHideParams
	{
		EntityId id;
		bool bHide;

		NetHideParams()
			:
			id(ZERO),
			bHide(false)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'eid');
			ser.Value("bHide", bHide, 'bool');
		}
	};	
	
	struct NetMakeHostileParams
	{
		EntityId id;
		bool bHostile;

		NetMakeHostileParams()
			:
			id(ZERO),
			bHostile(false)
		{};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'eid');
			ser.Value("bHostile", bHostile, 'bool');
		}
	};

	struct NetServerEnterVehicleParams
	{
		EntityId actorId;
		EntityId vehicleId;
		bool fast;
		

		NetServerEnterVehicleParams()
			:
			actorId(ZERO),
			vehicleId(ZERO),
			fast(false)
		{
		};

		NetServerEnterVehicleParams(EntityId _actorId, EntityId _vehId, bool _fast)
			:
			actorId(_actorId),
			vehicleId(_vehId),
			fast(_fast)
		{
		};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("actorId", actorId, 'eid');
			ser.Value("vehicleId", vehicleId, 'eid');
			ser.Value("fast", fast, 'bool');
		}
	};	
	
	struct NetClientEnterVehicleParams
	{
		EntityId actorId;
		EntityId vehicleId;
		int seatId;
		

		NetClientEnterVehicleParams()
			:
			actorId(ZERO),
			vehicleId(ZERO),
			seatId(ZERO)
		{
		};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("actorId", actorId, 'eid');
			ser.Value("vehicleId", vehicleId, 'eid');
			ser.Value("seatId", seatId);
		}
	};	
	
	const char* GetNameOfClass() { return "CTOSZeusSynchronizer"; }

	//CLIENT - Направленные на клиент
	//SERVER - Направленные на сервер с клиента
	//NOATTACH - Без привязки к данным сериализации
	//Reliable - надёжная доставка пакета

	DECLARE_SERVER_RMI_POSTATTACH(SvRequestAIMakeHostile, NetMakeHostileParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_POSTATTACH(SvRequestExecuteOrder, NetExecuteOrderParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_POSTATTACH(SvRequestKillEntity, NetKillParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_POSTATTACH(SvRequestRemoveEntity, NetRemoveParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_POSTATTACH(SvRequestHideEntity, NetHideParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_POSTATTACH(ClHideEntity, NetHideParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_POSTATTACH_FAST(SvRequestTransformEntity, NetTransformParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_POSTATTACH_FAST(ClTransformEntity, NetTransformParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestCopyEntity, NetCopyParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestSpawnEntity, NetSpawnParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClCopyEntity, NetCopiedInfo, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSpawnEntity, NetSpawnedInfo, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_PREATTACH(SvRequestMakeZeus, NetMakeParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_PREATTACH(ClMakeZeus, NetMakeParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_POSTATTACH(SvRequestVehicleEnter, NetServerEnterVehicleParams, eNRT_ReliableUnordered);
	//DECLARE_CLIENT_RMI_POSTATTACH(ClVehicleEnter, NetClientEnterVehicleParams, eNRT_ReliableUnordered);

};