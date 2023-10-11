#pragma once

#include <IGameObject.h>

#include "MasterModule.h"

#include "../GenericSynchronizer.h"

struct NetMasterClientSavedParams
{
	EntityId masterId;
	Vec3 pos;
	Quat rot;
	float suitEnergy;
	uint suitMode;
	int species;

	NetMasterClientSavedParams()
		: masterId(0),
		suitEnergy(0),
		suitMode(0),
		species(0) {};

	void SerializeWith(TSerialize ser)	{
		ser.Value("masterId", masterId, 'eid');
		ser.Value("pos", pos, 'wrld');
		ser.Value("rot", rot, 'ori1');
		ser.Value("suitEnergy", suitEnergy, 'nNRG'); // от 0 до 200
		ser.Value("suitMode", suitMode, 'ui3');// от 0 до 7
		ser.Value("species", species, 'ui5'); // от 0 до 31
	}
};

typedef NetMasterClientSavedParams NetMCSaved;


struct NetMasterAddingParams
{
	EntityId entityId;
	string desiredSlaveClassName;

	NetMasterAddingParams() :
		entityId(0), desiredSlaveClassName(nullptr) {};

	explicit NetMasterAddingParams(const EntityId entId, const char* slaveClsName) :
		entityId(entId), desiredSlaveClassName(slaveClsName) {}

	void SerializeWith(TSerialize ser)
	{
		ser.Value("entityId", entityId, 'eid');
		ser.Value("desiredSlaveClassName", desiredSlaveClassName, 'stab');
	}
};

struct NetMasterStartControlParams
{
	EntityId slaveId;
	EntityId masterId;

	NetMasterStartControlParams()
		: slaveId(0), masterId(0) {}

	void SerializeWith(TSerialize ser)
	{
		ser.Value("slaveId", slaveId, 'eid');
		ser.Value("masterId", masterId, 'eid');
	}
};

struct NetMasterStopControlParams
{
	EntityId masterId;

	NetMasterStopControlParams()
		: masterId(0) {}

	explicit NetMasterStopControlParams(const EntityId entId)
		: masterId(entId) {}

	void SerializeWith(TSerialize ser)
	{
		ser.Value("masterId", masterId, 'eid');
	}
};

typedef NetMasterAddingParams NetDesiredSlaveClsParams;
typedef NetMasterStopControlParams NetMasterIdParams;

/**
 * \brief TOS Master Module сетевой синхронизатор
 * \note Обеспечивает вызов RMI'шек, необходимых для сетевой синхронизации работы модуля
 */
class CTOSMasterSynchronizer final : public CTOSGenericSynchronizer  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	CTOSMasterSynchronizer();
	~CTOSMasterSynchronizer() override;

	// IGameObjectExtension
	void PostInit(IGameObject* pGameObject) override;
	void Release() override;
	void FullSerialize(TSerialize ser) override;
	bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) override;
	void Update(SEntityUpdateContext& ctx, int updateSlot) override;
	void HandleEvent(const SGameObjectEvent&) override;
	void ProcessEvent(SEntityEvent&) override;
	void GetMemoryStatistics(ICrySizer* s) override;
	//~IGameObjectExtension

	const char* GetClassName() override {return "CTOSMasterSynchronizer";}

	//CLIENT - Направленные на клиент
	//SERVER - Направленные на сервер с клиента
	//NOATTACH - Без привязки к данным сериализации
	//Reliable - надёжная доставка пакета

	DECLARE_SERVER_RMI_NOATTACH(SvRequestMasterAdd, NetMasterAddingParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestSetDesiredSlaveCls, NetDesiredSlaveClsParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClMasterClientStartControl, NetMasterStartControlParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestMasterClientStartControl, NetMasterStartControlParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClMasterClientStopControl, NetGenericNoParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestMasterClientStopControl, NetMasterStopControlParams, eNRT_ReliableOrdered);


	//TODO: 10/11/2023, 09:25 Создать модуль для ИИ, когда наберется достаточно функций.
	DECLARE_SERVER_RMI_NOATTACH(SvRequestSaveMCParams, NetMasterIdParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestApplyMCSavedParams, NetMasterIdParams, eNRT_ReliableOrdered);


};