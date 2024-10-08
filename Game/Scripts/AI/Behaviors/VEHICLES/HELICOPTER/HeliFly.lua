--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: VTOL Behaviour SCRIPT
--  
--------------------------------------------------------------------------
--  History:
--  - 16/04/2007   : the first implementation by Tetsuji
--
--------------------------------------------------------------------------
local Xaxis =0;
local Yaxis =1;
local Zaxis =2;
local minUpdateTime = 0.4;

AIBehaviour.HeliFly = {
	Name = "HeliFly",
	Base = "HeliBase",
	alertness = 0,

	-- SYSTEM EVENTS --
	--------------------------------------------------------------------------
	Constructor = function( self, entity, sender, data )
		
		--TheOtherSide
		entity.AI.previousBehaviour = entity.AI.currentBehaviour
		entity.AI.currentBehaviour = self.Name
		--~TheOtherSide	

	end,
	
	ACT_DUMMY = function( self, entity, sender, data )
		self:VTOLFLY_MAIN( entity, data );
	end,

	--------------------------------------------------------------------------
	Destructor = function ( self, entity, data )

		entity.vehicle:SetMovementMode(0);

		local vZero = { x=0.0, y=0.0, z=0.0 };
		AI.SetForcedNavigation( entity.id, vZero );
		if ( entity.AI.heliTimer ~= nil ) then
			Script.KillTimer(entity.AI.heliTimer);
			entity.AI.heliTimer = nil;
		end

	end,

	--------------------------------------------------------------------------
	OnPathFound = function( self, entity, sender )
	end,	
	--------------------------------------------------------------------------
	OnNoTarget = function( self, entity )
		end,
	--------------------------------------------------------------------------
	OnSomethingSeen = function( self, entity )
	end,
	--------------------------------------------------------------------------
	OnPlayerSeen = function( self, entity, fDistance )
	end,
	--------------------------------------------------------------------------
	OnSeenByEnemy = function( self, entity, sender )
	end,
	--------------------------------------------------------------------------
	OnCloseContact= function( self, entity )
	end,
	--------------------------------------------------------------------------
	OnEnemyMemory = function( self, entity )
	end,
	--------------------------------------------------------------------------
	OnInterestingSoundHeard = function( self, entity )
	end,
	--------------------------------------------------------------------------
	OnThreateningSoundHeard = function( self, entity )
	end,
	--------------------------------------------------------------------------
	OnSoreDamage = function ( self, entity, sender, data )
	end,
	--------------------------------------------------------------------------
	OnEnemyDamage = function ( self, entity, sender, data )
	end,
	--------------------------------------------------------------------------
	OnDamage = function ( self, entity, sender, data )
	end,
	--------------------------------------------------------------------------
	OnGroupMemberDied = function( self, entity, sender )
	end,
	--------------------------------------------------------------------------
	OnObjectSeen = function( self, entity, fDistance, data )
	end,

	--------------------------------------------------------------------------
	VTOLFLY_MAIN = function( self, entity, data )
		--TheOtherSide
		if (entity.AI.ignoreSignals == true) then
			return;
		end
		--~TheOtherSide

		AI.CreateGoalPipe("VtolLookatRef");
		AI.PushGoal("VtolLookatRef","locate",0,"refpoint");
		AI.PushGoal("VtolLookatRef","lookat",0,0,0,true,1);

		AI.CreateGoalPipe("VtolFly");
		AI.PushGoal("VtolFly","waitsignal", 1, "VTOLFLY_END", nil, 1000.0 );
		AI.PushGoal("VtolFly","signal",1,1,"TO_HELI_IDLE",SIGNALFILTER_SENDER);
		entity:InsertSubpipe(0,"VtolFly",nil,data.iValue);

		entity.AI.bLock = 0;
		entity.AI.deltaTSystem = System.GetCurrTime();

		entity.AI.vDirectionRsv = {};

		if ( entity.AI.flyPathName ~=nil ) then
			entity.AI.heliTimer = nil;
			entity.AI.heliTimer = Script.SetTimerForFunction( minUpdateTime *1000, "AIBehaviour.HeliFly.VTOLFLY_SUB2", entity );

		else
			entity.AI.heliTimer = nil;
			entity.AI.heliTimer = Script.SetTimerForFunction( minUpdateTime *1000, "AIBehaviour.HeliFly.VTOLFLY_SUB", entity );
		end

	end,

	----------------------------------------------------------------------------------------------------------------------------------------------------
	VTOLFLY_SUB = function( entity )
		--TheOtherSide
		if (entity.AI.ignoreSignals == true) then
			return;
		end
		--~TheOtherSide

	--------------------------------------------------------------------------
		if ( entity.AI == nil or entity.AI.heliTimer == nil or entity:GetSpeed() == nil ) then
			local myEntity = System.GetEntity( entity.id );
			if ( myEntity ) then
				local vZero = {x=0.0,y=0.0,z=0.0};
				AI.SetForcedNavigation( entity.id, vZero );
			end
			return;
		end

		if ( entity.AI.bFinishLanding == true ) then
			local vZero = {x=0.0,y=0.0,z=0.0};
			AI.SetForcedNavigation( entity.id, vZero );
			return;
		end

		if ( entity.id and System.GetEntity( entity.id ) ) then
		else
			local vZero = {x=0.0,y=0.0,z=0.0};
			AI.SetForcedNavigation( entity.id, vZero );
			return;
		end

		if ( entity:IsActive() and AI.IsEnabled(entity.id) ) then
		else
			local vZero = {x=0.0,y=0.0,z=0.0};
			AI.SetForcedNavigation( entity.id, vZero );
			return;
		end

		local dt = System.GetCurrTime() - entity.AI.deltaTSystem;
		entity.AI.deltaTSystem = System.GetCurrTime();
			
		if ( dt < minUpdateTime*0.25 ) then
			return;
		end

		entity.AI.heliTimer = Script.SetTimerForFunction( minUpdateTime *1000, "AIBehaviour.HeliFly.VTOLFLY_SUB", entity );

	--------------------------------------------------------------------------

		if ( entity.AI.bLock == 0 ) then

			local vDir = {};
			CopyVector( vDir, entity:GetDirectionVector(1) );
			vDir.z = 0;
			NormalizeVector( vDir );

			local vTmp = {};
			SubVectors( vTmp, entity.AI.vReinforcementSetPosition, entity:GetPos() );
			vTmp.z = 0.0;
			NormalizeVector( vTmp );
			FastScaleVector( vTmp, vTmp, 3.0 );
			CopyVector( entity.AI.vDirectionRsv, vTmp );
			vTmp.z = 5.0;
			AI.SetForcedNavigation( entity.id, vTmp );
			if ( entity:GetSpeed() < 5.0 ) then
				NormalizeVector( entity.AI.vDirectionRsv );
				if ( dotproduct3d( vDir, entity.AI.vDirectionRsv ) > math.cos( 3.1416 * 2.0/180.0 ) ) then
					entity.AI.bLock = 1;
					entity.vehicle:SetMovementMode(1);
				end
			end
			return;

		end

		if ( entity.AI.vehicleIgnorantIssued ~= true ) then
			local target = AI.GetAttentionTargetEntity( entity.id );
			if ( target and AI.Hostile( entity.id, target.id ) ) then
				AI.Signal(SIGNALFILTER_SENDER, 1, "TO_HELI_ATTACK", entity.id);
			end
		end

		if ( entity.AI.bLock == 1 ) then

			local vTmp = {};
			local vMyPos = {};

			local curSpeed = entity:GetSpeed() + 10.0;
			if ( curSpeed > 50.0 ) then
				curSpeed = 50.0;
			end

			FastScaleVector( vTmp, entity.AI.vDirectionRsv, 50.0 );

			CopyVector( vMyPos, entity:GetPos() );
			if ( vMyPos.z < entity.AI.vReinforcementSetPosition.z ) then
				vTmp.z = 3.0 ;
			end
			AI.SetForcedNavigation( entity.id, vTmp );
	
			local vDir = {};
			SubVectors( vDir, entity.AI.vReinforcementSetPosition, entity:GetPos() );
			vDir.z = 0;
			NormalizeVector( vDir );
			
			if ( dotproduct3d( vDir, entity.AI.vDirectionRsv ) < 0 ) then
				entity.AI.bLock = 2;
				local vZero = {x=0.0,y=0.0,z=0.0};
				AI.SetForcedNavigation( entity.id, vZero );
				AI.Signal(SIGNALFILTER_SENDER, 1, "VTOLFLY_END", entity.id);
				entity.vehicle:SetMovementMode(0);
			end
			return;

		end

		if ( entity.AI.bLock == 2 ) then
		end

	end,

	----------------------------------------------------------------------------------------------------------------------------------------------------
	VTOLFLY_SUB2 = function( entity )
		--TheOtherSide
		if (entity.AI.ignoreSignals == true) then
			return;
		end
		--~TheOtherSide

	--------------------------------------------------------------------------
		if ( entity.AI == nil or entity.AI.heliTimer == nil or entity:GetSpeed() == nil ) then
			local myEntity = System.GetEntity( entity.id );
			if ( myEntity ) then
				local vZero = {x=0.0,y=0.0,z=0.0};
				AI.SetForcedNavigation( entity.id, vZero );
			end
			return;
		end

		if ( entity.AI.bFinishLanding == true ) then
			local vZero = {x=0.0,y=0.0,z=0.0};
			AI.SetForcedNavigation( entity.id, vZero );
			return;
		end

		if ( entity.id and System.GetEntity( entity.id ) ) then
		else
			local vZero = {x=0.0,y=0.0,z=0.0};
			AI.SetForcedNavigation( entity.id, vZero );
			return;
		end

		if ( entity:IsActive() and AI.IsEnabled(entity.id) ) then
		else
			local vZero = {x=0.0,y=0.0,z=0.0};
			AI.SetForcedNavigation( entity.id, vZero );
			return;
		end

		local dt = System.GetCurrTime() - entity.AI.deltaTSystem;
		entity.AI.deltaTSystem = System.GetCurrTime();
			
		if ( dt < minUpdateTime*0.25 ) then
			return;
		end

		entity.AI.heliTimer = Script.SetTimerForFunction( minUpdateTime *1000, "AIBehaviour.HeliFly.VTOLFLY_SUB2", entity );

	--------------------------------------------------------------------------
		if ( entity.AI.bLock == 0 ) then


			local mySegNo			= AI.GetPathSegNoOnPath( entity.id, entity.AI.flyPathName, entity:GetPos() );
	
			local pathLength = AI.GetTotalLengthOfPath( entity.id, entity.AI.flyPathName );
			local delta = 10.0 / pathLength;
			if ( delta >1.0 ) then 
				delta = 1.0;
			end
	
			delta = delta * 100.0;
	
			local vSrc = {};
			local vDst = {};
			local vUnit = {};
			local vMyPos = {};

			CopyVector( vMyPos, entity:GetPos() );
			CopyVector( vSrc, AI.GetPointOnPathBySegNo( entity.id, entity.AI.flyPathName, mySegNo ) );
			CopyVector( vDst, AI.GetPointOnPathBySegNo( entity.id, entity.AI.flyPathName, mySegNo + delta ) );

			local vFwd = {};
			
			CopyVector( vFwd, entity:GetDirectionVector(1) );
			vFwd.z = 0;
			NormalizeVector( vFwd );
			
			SubVectors( vUnit, vDst, vSrc );
			vUnit.z =0;
			NormalizeVector( vUnit );

			dot = dotproduct3d( vUnit, vFwd );
			FastScaleVector( vUnit, vUnit, 1.0 );
			if ( vMyPos.z < vDst.z ) then
				vUnit.z = vDst.z - vMyPos.z;
			end

			AI.SetForcedNavigation( entity.id, vUnit );


			if ( dot >math.cos( 3.1416 * 3.0/180.0 ) and math.abs( vUnit.z ) < 5.0 ) then
				entity.vehicle:SetMovementMode(1);
				entity.AI.bLock = 1;
				local vVec = {};
				entity:GetVelocity( vVec );
				FastScaleVector( vVec, vVec, 0.0 );
				AI.SetForcedNavigation( entity.id, vVec );
				return;
			end

		end

		if ( entity.AI.vehicleIgnorantIssued ~= true ) then
			local target = AI.GetAttentionTargetEntity( entity.id );
			if ( target and AI.Hostile( entity.id, target.id ) ) then
				AI.Signal(SIGNALFILTER_SENDER, 1, "TO_HELI_ATTACK", entity.id);
			end
		end

		if ( entity.AI.bLock == 1 ) then

			local vSrc = {};
			local vDst = {};
			local vDst2 = {};
			local vMid = {};
			local vUnit = {};

			local vMyPos = {};
			CopyVector( vMyPos, entity:GetPos() );

			local speedFactor = entity:GetSpeed();
			speedFactor =20.0;

			local mySegNo			= AI.GetPathSegNoOnPath( entity.id, entity.AI.flyPathName, entity:GetPos() );
	
			local pathLength = AI.GetTotalLengthOfPath( entity.id, entity.AI.flyPathName );
			local delta = speedFactor / pathLength;
			if ( delta >1.0 ) then 
				delta = 1.0;
			end
	
			delta = delta * 100.0;

			if ( mySegNo > (pathLength-250.0)*100.0/pathLength ) then
				entity.vehicle:SetMovementMode(0);
				entity.AI.bLock = 2;
				return;
			end

			CopyVector( vSrc, AI.GetPointOnPathBySegNo( entity.id, entity.AI.flyPathName, mySegNo ) );
			
			local delta1 = mySegNo + delta;
			local delta2 = mySegNo + delta + delta;
			
			if ( delta1 >100.0 ) then
				delta1 = 100.0;
			end
			if ( delta2 >100.0 ) then
				delta2 = 100.0;
			end

			local dcurSpeed = entity:GetSpeed() + 10.0;
			if ( dcurSpeed > 40.0 ) then
				dcurSpeed = 40.0;
			end

			-- makespline(2)			
			CopyVector( vDst, AI.GetPointOnPathBySegNo( entity.id, entity.AI.flyPathName, delta1 ) );
			CopyVector( vDst2, AI.GetPointOnPathBySegNo( entity.id, entity.AI.flyPathName, delta2 ) );
			FastSumVectors( vMid, vDst, vDst2 );
			vSrc.z = vMyPos.z;
			FastScaleVector( vMid, vMid, 0.5 );
			SubVectors( vUnit, vMid, vSrc );
		--	vUnit.z =0;
			NormalizeVector( vUnit );
			FastScaleVector( vUnit, vUnit, dcurSpeed );

			AI.SetForcedNavigation( entity.id, vUnit );

		end

		if ( entity.AI.bLock == 2 ) then

				local vVec = {};
				entity:GetVelocity( vVec );
				FastScaleVector( vVec, vVec, 0.5 );
				vVec.z = 1.0;
				AI.SetForcedNavigation( entity.id, vVec );

				entity:GetVelocity( vVec );
				vVec.z = 0;

				if ( LengthVector( vVec ) < 5.0 ) then
					FastScaleVector( vVec, vVec, 0.0 );
					AI.SetForcedNavigation( entity.id, vVec );
					entity.AI.bLock = 3
					AI.Signal(SIGNALFILTER_SENDER, 1, "VTOLFLY_END", entity.id);
				end

		end

		if ( entity.AI.bLock == 3 ) then

		end


	end,

}
