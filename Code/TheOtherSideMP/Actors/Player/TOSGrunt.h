/*************************************************************************
Crysis Co-op Source File.
Copyright (C), Crysis Co-op
**************************************************************************/

// Created by Crysis Co-op Developers
// Adapted for TOS by Akeeper

#pragma once
#include "TOSPlayer.h"


class CTOSGrunt 
	: public CTOSPlayer
{
public:
	CTOSGrunt();
	virtual ~CTOSGrunt();

	//CTOSPlayer
	virtual bool Init( IGameObject * pGameObject );
	virtual void PostInit( IGameObject * pGameObject );
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot);
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void ProcessEvent(SEntityEvent& event);
	virtual IActorMovementController * CreateMovementController();
	//~CTOSPlayer

	//ITOSMasterControllable
	void UpdateMasterView(SViewParams& viewParams, Vec3& offsetX, Vec3& offsetY, Vec3& offsetZ, Vec3& target, Vec3& current, float& currentFov);
	void ApplyMasterMovement(const Vec3& delta);
	//~ITOSMasterControllable


	enum CoopTimers
	{
		eTIMER_WEAPONDELAY	= 0x110
	};

    struct SSuitParams
    {
		SSuitParams() {};
		SSuitParams(int suit): 
			suitmode(suit) 
		{};
        int suitmode;
        void SerializeWith(TSerialize ser)
        {
            ser.Value("suitmode", suitmode);
        }
    };

	int GetAlertnessState() const { return m_nAlertness; }

	DECLARE_CLIENT_RMI_NOATTACH(ClChangeSuitMode, SSuitParams, eNRT_ReliableOrdered);


protected:
	static const EEntityAspects ASPECT_ALIVE	= eEA_GameServerDynamic;
	static const EEntityAspects ASPECT_HIDE		= eEA_GameServerStatic;
	static const EEntityAspects ASPECT_STANCE	= eEA_GameServerStatic;

	void UpdateMovementState();
	void DrawDebugInfo();

protected:

	// Used to serialize special movement requests (e.g. SmartObject actortarget usage and such)
	struct SSpecialMovementRequestParams
	{
		SSpecialMovementRequestParams() {};
		SSpecialMovementRequestParams(uint32 reqFlags, const SActorTargetParams& actorTarget, const string& animation) 
			: flags(reqFlags)
			, targetParams(actorTarget)
		{
			// HACK: The structure copying is retarded and this needs to be done...
			//targetAnimation = animation;
		};

		uint32 flags;
		SActorTargetParams targetParams;
		//string targetAnimation; // why isnt it serializing?

		void SerializeWith(TSerialize ser)
		{

			ser.Value("flags", flags);

			//if ((flags & CMovementRequest::eMRF_ActorTarget))
			{

				ser.Value("location", targetParams.location);
				ser.Value("direction", targetParams.direction);
				ser.Value("vehicleName", targetParams.vehicleName);
				ser.Value("vehicleSeat", targetParams.vehicleSeat);
				ser.Value("speed", targetParams.speed);
				ser.Value("directionRadius", targetParams.directionRadius);
				ser.Value("locationRadius", targetParams.locationRadius);
				ser.Value("startRadius", targetParams.startRadius);
				ser.Value("signalAnimation", targetParams.signalAnimation);
				ser.Value("projectEnd", targetParams.projectEnd);
				ser.Value("navSO", targetParams.navSO);
				ser.Value("animation", targetParams.animation);
				ser.Value("stance", (int&)targetParams.stance);
				ser.Value("triggerUser", (int&)targetParams.triggerUser);
			}
			
		}
	};
	DECLARE_CLIENT_RMI_NOATTACH(ClSpecialMovementRequest, SSpecialMovementRequestParams, eNRT_ReliableOrdered);

public:

	void SendSpecialMovementRequest(uint32 reqFlags, const SActorTargetParams& targetParams);

private:
	Vec3 m_vMoveTarget;
	Vec3 m_vAimTarget;
	Vec3 m_vLookTarget;
	Vec3 m_vBodyTarget;
	Vec3 m_vFireTarget;

	float m_fPseudoSpeed;
	float m_fDesiredSpeed;

	int m_nAlertness;
	int m_nStance;
	int m_nSuitMode;

	enum EAIMovementNetFlags
	{
		eHasAimTarget = (1 << 0),
		eHasLookTarget = (1 << 1),
		eHasBodyTarget = (1 << 2),
		eHasFireTarget = (1 << 3),
		eHasMoveTarget = (1 << 4),
		eAllowStrafing = (1 << 5),
	};

	enum EAIWeaponNetFlags
	{
		eLaserActive = (1 << 0),
		eFlashlightActive = (1 << 1),
	};

	bool HasMovementFlag(EAIMovementNetFlags flag) { return (m_nMovementNetworkFlags & flag) == flag; }
	bool HasWeaponFlag(EAIWeaponNetFlags flag) { return (m_nWeaponNetworkFlags & flag) == flag; }

	uint8 m_nMovementNetworkFlags;
	uint8 m_nWeaponNetworkFlags;

	bool m_bHidden;
};
