/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once

#include "Player.h"
class CTOSMasterClient;

class CTOSPlayer  : public CPlayer  // NOLINT(cppcoreguidelines-special-member-functions)
{
	friend class CPlayerMovement;
	friend class CPlayerRotation;
	friend class CPlayerInput;
	friend class CPlayerView;
	friend class CNetPlayerInput;
	friend class CTOSMasterClient;

public:
	CTOSPlayer();
	~CTOSPlayer() ;

	//CPlayer
	/**
	* Init Client
	* Вызывается на сервере при создании объекта на сервере.
	* Инициализирует текущий GameObjectExtension на клиенте по определенному каналу.
	*/
	bool Init(IGameObject* pGameObject) ;
	void InitClient(int channelId ) ;


	/**
	* Init Local Player
	* Вызывается на локальной машине игрока при подключении его к серверу
	*/
	void InitLocalPlayer() ;
	void SetSpectatorMode(uint8 mode, EntityId targetId) ;

	void PostInit( IGameObject * pGameObject ) ;
	void Update(SEntityUpdateContext& ctx, int updateSlot) ;
	bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) ;
	void Release() ;
	void UpdateView(SViewParams& viewParams) ;
	void PostUpdateView(SViewParams& viewParams) ;
	void Kill() ;
	void PostUpdate(float frameTime);

	IEntity *LinkToVehicle(EntityId vehicleId);
	IEntity *LinkToEntity(EntityId entityId, bool bKeepTransformOnDetach=true);
	void LinkToMountedWeapon(EntityId weaponId);

	//~CPlayer

	//CTOSActor
	Matrix33 GetViewMtx() ;
	Matrix33 GetBaseMtx() ;
	Matrix33 GetEyeMtx() ;
	bool	 ApplyActions(int actions);
	//~CTOSActor

	CTOSMasterClient* GetMasterClient() const;


protected:
	void ClearInterference();

private:

 /**
 * Master Client
 * Является Мастер-Клиентом для каждого игрока.
 * От сервера к клиенту Мастер-Клиент будет не доступен.
 * Будет доступен только внутри клиента.
 */
	CTOSMasterClient* m_pMasterClient;

	//Crysis co-op
public:

	struct SAwarenessParams
	{
		SAwarenessParams() {};
		SAwarenessParams(float awarenessFloat) :
			awarenessFloat(awarenessFloat)
		{};

		float awarenessFloat;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("awarenessFloat", awarenessFloat);
		}
	};

	void ForceMusicMood(float intensity, bool force)
	{
		m_bMusicForceMood = force;
		m_fMusicIntensity = intensity;
		m_fMusicDelay = 5.f;
	};

	DECLARE_CLIENT_RMI_PREATTACH(ClUpdateAwareness, SAwarenessParams, eNRT_ReliableUnordered);

private:
	void UpdateDetectionValue(float frameTime);
	void UpdateMusic(float frameTime);

public:

	// Summary:
	//	Gets the player's current detection value.
	float GetDetectionValue() {
		return m_fDetectionValue;
	}

private:
	struct ICVar* m_pSystemUpdateRate;
	float m_fDetectionTimer;
	float m_fDetectionValue;
	float m_fLastDetectionValue;

	float m_fMusicDelay;


	float m_fNetDetectionDelay;

	float m_fMusicIntensity;
	bool m_bMusicForceMood;
	//~Crysis co-op
};