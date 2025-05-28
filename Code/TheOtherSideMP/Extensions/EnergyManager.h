#pragma once

#include "IGameObject.h"
#include "TheOtherSideMP/Game/Modules/GenericSynchronizer.h"
#define TOS_CHECK_CONSUMER_EXISTING(pActor)\
	CRY_ASSERT_MESSAGE(pActor, "Pointer to actor is nullptr");\
	CRY_ASSERT_MESSAGE((pActor)->GetEnergyManager(), "Pointer to Energy Consumer instance is nullptr")\

#define TOS_SAFE_ADD_ENERGY(pActor, energy)\
	assert(pActor);\
	assert((pActor)->GetEnergyManager());\
	if ((pActor) && (pActor)->GetEnergyManager())\
	{\
		(pActor)->GetEnergyManager()->AddEnergy(energy);\
	}\

#define TOS_SAFE_GET_ENERGY(pActor)\
	(pActor) && (pActor)->GetEnergyManager() ? (pActor)->GetEnergyManager()->GetEnergy() : 0.0f\

class CTOSEnergyManager : public CGameObjectExtensionHelper<CTOSEnergyManager, IGameObjectExtension>
{
public:
    CTOSEnergyManager();
    virtual ~CTOSEnergyManager();

    // IGameObjectExtension
	bool Init(IGameObject* pGameObject) override;
	void PostInit(IGameObject* pGameObject) override;
	void InitClient(int channelId) override;
	void PostInitClient(int channelId) override;
	void Release() override;
	void FullSerialize(TSerialize ser) override;
	bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) override;
	void PostSerialize() override {};
	void SerializeSpawnInfo(TSerialize ser) override {};
	ISerializableInfoPtr GetSpawnInfo() override { return 0;}
	void Update(SEntityUpdateContext& ctx, int updateSlot) override;
	void PostUpdate(float frameTime) override {} ;
	void PostRemoteSpawn() override {} ;
	void HandleEvent(const SGameObjectEvent&) override;
	void ProcessEvent(SEntityEvent&) override;
	void SetChannelId(uint16 id) override {};
	void SetAuthority(bool auth) override {};
	void GetMemoryStatistics(ICrySizer* s) override;
    // ~IGameObjectExtension

    bool AddEnergy(const float value);
    bool SetEnergy(float value, const bool initiated = false);
    bool SetEnergyForced(const float value);
    float GetEnergy() const;
    bool SetMaxEnergy(const float value);
    float GetMaxEnergy() const;
    bool SetDrainValue(const float value);
    float GetDrainValue() const;
    void EnableUpdate(const bool enable);
    bool IsUpdating() const;
    void Reset();
    bool SetRegenStartDelaySP(const float val);
    bool SetRegenStartDelayMP(const float val);
    bool SetRegenStartDelay20Boundary(const float val);
    float GetRegenStartDelay() const;
    void SetRechargeTimeSP(const float time);
    void SetRechargeTimeMP(const float time);

    struct NetEnergyParams
	{
		float energy;
		bool initiated;
		bool forced;
		NetEnergyParams()
			: energy(0),
			initiated(false),
			forced(false) {} ;

		explicit NetEnergyParams(float _energy, bool _initiated, bool _forced)
			: energy(_energy), initiated(_initiated), forced(_forced)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("energy", energy);
			ser.Value("initiated", initiated, 'bool');
			ser.Value("forced", forced, 'bool');
		}
	};

    DECLARE_SERVER_RMI_NOATTACH(SvRequestSetEnergy, NetEnergyParams, eNRT_ReliableOrdered);
    static const int DEFAULT_ENERGY = 200;
private:
    float m_energy;
    float m_maxEnergy;
    float m_regenStartDelay;
    float m_regenStartDelaySP;
    float m_regenStartDelayMP;
    float m_regenStartDelay20Boundary;
    float m_rechargeTimeSP;
    float m_rechargeTimeMP;
    float m_drainValue;
    bool m_enableUpdate;
}; 