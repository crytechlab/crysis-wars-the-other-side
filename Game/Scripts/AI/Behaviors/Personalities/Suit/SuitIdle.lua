--------------------------------------------------
-- SuitIdle
--------------------------
--   created: Kirill Bulatsev 26-10-2006
--
--	
--

AIBehaviour.SuitIdle = {
	Name = "SuitIdle",
	alertness = 0,
	
	-----------------------------------------------------
	Constructor = function (self, entity)
		entity:InitAIRelaxed();

		--TheOtherSide
		entity.AI.previousBehaviour = entity.AI.currentBehaviour
		entity.AI.currentBehaviour = self.Name
		--~TheOtherSide	

--		entity:DrawWeaponNow();
		
		entity:NanoSuitMode(BasicAI.SuitMode.SUIT_ARMOR);
--		entity:NanoSuitMode( BasicAI.SuitMode.SUIT_SPEED );
		
		---------------------------------------------
		AI.BeginGoalPipe("su_fast_bullet_reaction");
			AI.PushGoal("bodypos",1,BODYPOS_STAND,1);
			AI.PushGoal("strafe",0,100,100);
			AI.PushGoal("firecmd",0,FIREMODE_BURST_DRAWFIRE);
			AI.PushGoal("locate",0,"probtarget");
			AI.PushGoal("+seekcover", 1, COVER_HIDE, 3.0, 2, 1);
			AI.PushGoal("signal",1,1,"COVER_NORMALATTACK",0);
		AI.EndGoalPipe();

		---------------------------------------------
		AI.BeginGoalPipe("su_fast_threat_reaction");
			AI.PushGoal("firecmd",0,0);
			AI.PushGoal("run", 0, 0);
			AI.PushGoal("bodypos",1,BODYPOS_CROUCH);
			AI.PushGoal("strafe",0,0,0);
			AI.PushGoal("timeout",1,2,3);
			AI.PushGoal("strafe",0,100,100);
			AI.PushGoal("bodypos",1,BODYPOS_STAND,1);
			AI.PushGoal("firecmd",0,FIREMODE_AIM);
			AI.PushGoal("locate",0,"probtarget");
			AI.PushGoal("+seekcover", 1, COVER_HIDE, 3.0, 2, 1);
			AI.PushGoal("signal",1,1,"COVER_NORMALATTACK",0);
		AI.EndGoalPipe();

		entity:SelectPipe(0,"do_nothing");

		entity:NanoSuitMode(BasicAI.SuitMode.SUIT_ARMOR);

		AI.EnableWeaponAccessory(entity.id, AIWEPA_LASER, false);
		entity.actor:SelectItemByName("DSG1");

		entity.AI.lastBulletReactionTime = _time - 10.0;
	end,

	-----------------------------------------------------
	Destructor = function(self,entity)
	end,

	---------------------------------------------
	OnQueryUseObject = function ( self, entity, sender, extraData )
	end,

	---------------------------------------------
	OnPlayerSeen = function( self, entity, fDistance, data )
		entity:MakeAlerted();
		entity:TriggerEvent(AIEVENT_DROPBEACON);
		entity.AI.firstContact = true;
		AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT, 1, "ENEMYSEEN_FIRST_CONTACT",entity.id);
		AI.Signal(SIGNALFILTER_SENDER,1,"GO_TO_ATTACK",entity.id);
	end,

	---------------------------------------------
	COVER_NORMALATTACK = function (self, entity, sender)
		AI.Signal(SIGNALFILTER_SENDER, 1, "GO_TO_THREATENED",entity.id);
	end,

	---------------------------------------------
	OnNoTarget = function(self,entity,sender)
	end,

	---------------------------------------------
	OnTargetDead = function( self, entity )
		-- called when the attention target died
		entity:Readibility("target_down",1,1,0.3,0.5);
	end,
	
	--------------------------------------------------
	OnNoHidingPlace = function( self, entity, sender,data )
	end,	

	---------------------------------------------
	OnBackOffFailed = function(self,entity,sender)
	end,

	---------------------------------------------
	SEEK_KILLER = function(self, entity)
		AI.Signal(SIGNALFILTER_SENDER,1,"GO_TO_THREATENED",entity.id);
	end,
	
	---------------------------------------------
	OnEnemyMemory = function( self, entity )
		-- called when the enemy can no longer see its foe, but remembers where it saw it last
	end,
	
	---------------------------------------------
	OnSomethingSeen = function( self, entity )
		-- called when the enemy sees a foe which is not a living player
		entity:Readibility("idle_interest_see",1,1,0.6,1);
		AI.Signal(SIGNALFILTER_SENDER, 1, "GO_TO_THREATENED",entity.id);
	end,
	
	---------------------------------------------
	OnInterestingSoundHeard = function( self, entity )
		--TheOtherSide
		if (entity.AI.ignoreSignals == true) then
			return;
		end
		--~TheOtherSide

		-- check if we should check the sound or not.
		entity:Readibility("idle_interest_hear",1,1,0.6,1);
		AI.Signal(SIGNALFILTER_SENDER, 1, "GO_TO_THREATENED",entity.id);
	end,

	---------------------------------------------
	OnThreateningSoundHeard = function( self, entity, fDistance )
		--TheOtherSide
		if (entity.AI.ignoreSignals == true) then
			return;
		end
		--~TheOtherSide
		-- called when the enemy hears a scary sound
		entity:Readibility("idle_alert_threat_hear",1,1,0.6,1);
		entity:TriggerEvent(AIEVENT_DROPBEACON);
		AI.Signal(SIGNALFILTER_SENDER, 1, "GO_TO_THREATENED",entity.id);
	end,

	--------------------------------------------------
	INVESTIGATE_BEACON = function (self, entity, sender)
		entity:Readibility("ok_battle_state",1,1,0.6,1);
		AI.Signal(SIGNALFILTER_SENDER, 1, "GO_TO_THREATENED",entity.id);
	end,
		
	--------------------------------------------------
	OnCoverRequested = function ( self, entity, sender)
		-- called when the enemy is damaged
	end,

	---------------------------------------------
	OnDamage = function ( self, entity, sender)
		-- called when the enemy is damaged
		entity:Readibility("taking_fire",1,1,0.3,0.5);
		entity:GettingAlerted();
	end,

	---------------------------------------------
	OnEnemyDamage = function (self, entity, sender, data)
		-- called when the enemy is damaged
		entity:GettingAlerted();
		entity:Readibility("taking_fire",1,1,0.3,0.5);

		-- set the beacon to the enemy pos
		local shooter = System.GetEntity(data.id);
		if(shooter) then
			AI.SetBeaconPosition(entity.id, shooter:GetPos());
		else
			entity:TriggerEvent(AIEVENT_DROPBEACON);
		end

		AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT,1,"INCOMING_FIRE",entity.id);

		-- dummy call to this one, just to make sure that the initial position is checked correctly.
		AI_Utils:IsTargetOutsideStandbyRange(entity);

		entity.AI.lastBulletReactionTime = _time;
		entity:Readibility("bulletrain",1,0.1,0.4);
		entity:SelectPipe(0,"su_fast_bullet_reaction");

--		AI.Signal(SIGNALFILTER_SENDER, 1, "GO_TO_THREATENED",entity.id);
	end,

	--------------------------------------------------
	OnNearMiss = function(self, entity, sender)
		-- only react to hostile bullets.
		if(AI.Hostile(entity.id, sender.id)) then
			entity:GettingAlerted();
			if(AI.GetTargetType(entity.id)==AITARGET_NONE) then
				local	closestCover = AI.GetNearestHidespot(entity.id, 3, 15, sender:GetPos());
				if(closestCover~=nil) then
					AI.SetBeaconPosition(entity.id, closestCover);
				else
					AI.SetBeaconPosition(entity.id, sender:GetPos());
				end
			else
				entity:TriggerEvent(AIEVENT_DROPBEACON);
			end
			entity:Readibility("bulletrain",1,1,0.1,0.4);

			-- dummy call to this one, just to make sure that the initial position is checked correctly.
			AI_Utils:IsTargetOutsideStandbyRange(entity);

			AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT,1,"INCOMING_FIRE",entity.id);
--			AI.Signal(SIGNALFILTER_SENDER,1,"GO_TO_THREATENED",entity.id);

			entity.AI.lastBulletReactionTime = _time;
			entity:Readibility("bulletrain",1,0.1,0.4);
			entity:SelectPipe(0,"su_fast_bullet_reaction");

		end
	end,


	---------------------------------------------
	OnBulletRain = function(self, entity, sender, data)
		AIBehaviour.SuitHurricaneIdle:OnNearMiss(entity,sender);
	end,

	--------------------------------------------------
	OnCollision = function(self,entity,sender,data)
	end,	
	
	--------------------------------------------------
	OnCloseContact = function ( self, entity, sender,data)
--		entity:InsertSubpipe(AIGOALPIPE_NOTDUPLICATE,"melee_close");
	end,

	--------------------------------------------------
	OnGroupMemberDied = function(self, entity, sender, data)
		--AI.LogEvent(entity:GetName().." OnGroupMemberDied!");
		entity:GettingAlerted();
		AI.Signal(SIGNALFILTER_SENDER,1,"GO_TO_THREATENED",entity.id);
	end,

	---------------------------------------------
	ENEMYSEEN_FIRST_CONTACT = function( self, entity )
		if(AI.GetTargetType(entity.id) ~= AITARGET_ENEMY) then
			entity:Readibility("idle_interest_see",1,1,0.6,1);
			AI.Signal(SIGNALFILTER_SENDER, 1, "GO_TO_THREATENED",entity.id);
		end
	end,

	--------------------------------------------------
	ENEMYSEEN_DURING_COMBAT = function (self, entity, sender)
		entity:GettingAlerted();
		if(AI.GetTargetType(entity.id) ~= AITARGET_ENEMY) then
			AI.Signal(SIGNALFILTER_SENDER,1,"GO_TO_ATTACK",entity.id);
		end
	end,

	---------------------------------------------
	OnShapeEnabled = function (self, entity, sender, data)
		--Log(entity:GetName().."OnShapeEnabled");
		if(data.iValue == AIAnchorTable.COMBAT_TERRITORY) then
			AI_Utils:SetupTerritory(entity, false);
		elseif(data.iValue == AIAnchorTable.ALERT_STANDBY_IN_RANGE) then
			AI_Utils:SetupStandby(entity);
		end
	end,

	---------------------------------------------
	OnShapeDisabled = function (self, entity, sender, data)
		--Log(entity:GetName().."OnShapeDisabled");
		if(data.iValue == 1) then
			-- refshape
			AI_Utils:SetupStandby(entity);
		elseif(data.iValue == 2) then
			-- territory
			AI_Utils:SetupTerritory(entity, false);
		elseif(data.iValue == 3) then
			-- refshape and territory
			AI_Utils:SetupTerritory(entity, false);
			AI_Utils:SetupStandby(entity);
		end
		
	end,

	---------------------------------------------
	SET_TERRITORY = function (self, entity, sender, data)

		-- If the current standby area is the same as territory, clear the standby.
		if(entity.AI.StandbyEqualsTerritory) then
			entity.AI.StandbyShape = nil;
		end

		entity.AI.TerritoryShape = data.ObjectName;
		newDist = AI.DistanceToGenericShape(entity:GetPos(), entity.AI.TerritoryShape, 0);

		local curDist = 10000000.0;
		if(entity.AI.StandbyShape) then
			curDist = AI.DistanceToGenericShape(entity:GetPos(), entity.AI.StandbyShape, 0);
		end

--		Log(" - curdist:"..tostring(curDist));
--		Log(" - newdist:"..tostring(newDist));

		if(newDist < curDist) then
			if(entity.AI.TerritoryShape) then
				entity.AI.StandbyShape = entity.AI.TerritoryShape;
			end
			entity.AI.StandbyEqualsTerritory = true;
		end

		if(entity.AI.StandbyShape) then
			entity.AI.StandbyValid = true;
			AI.SetRefShapeName(entity.id, entity.AI.StandbyShape);
		else
			entity.AI.StandbyValid = false;
			AI.SetRefShapeName(entity.id, "");
		end

		if(entity.AI.TerritoryShape) then
			AI.SetTerritoryShapeName(entity.id, entity.AI.TerritoryShape);
		else
			AI.SetTerritoryShapeName(entity.id, "");
		end

	end,

	---------------------------------------------
	CLEAR_TERRITORY = function (self, entity, sender, data)
		entity.AI.StandbyEqualsTerritory = false;
		entity.AI.StandbyShape = nil;
		entity.AI.TerritoryShape = nil;

		AI.SetRefShapeName(entity.id, "");
		AI.SetTerritoryShapeName(entity.id, "");
	end,

	--------------------------------------------------
	OnCallReinforcements = function (self, entity, sender, data)
	end,

	--------------------------------------------------
	OnGroupChanged = function (self, entity)
		AI.BeginGoalPipe("temp_goto_beacon");
			AI.PushGoal("locate",0,"beacon");
			AI.PushGoal("approach",1,4,AILASTOPRES_USE);
		AI.EndGoalPipe();
		entity:SelectPipe(0,"temp_goto_beacon");
	end,

	--------------------------------------------------
	OnExposedToFlashBang = function (self, entity, sender, data)
--		entity:SelectPipe(0,"su_fast_threat_reaction");
--
--		AI.NotifyGroupTacticState(entity.id, 0, GN_NOTIFY_UNAVAIL);
--
		if (data.iValue == 1) then
			-- near
			entity:SelectPipe(0,"sn_flashbang_reaction_flinch");
		else
			-- visible
			entity:SelectPipe(0,"sn_flashbang_reaction");
		end
	end,

	--------------------------------------------------
	FLASHBANG_GONE = function (self, entity)
		AI.Signal(SIGNALFILTER_SENDER, 1, "COVER_NORMALATTACK",entity.id);
--		entity:SelectPipe(0,"do_nothing");
		-- Choose proper action after being interrupted.
--		AI_Utils:CommonContinueAfterReaction(entity);
	end,

	--------------------------------------------------
	OnExposedToSmoke = function (self, entity)
	end,

	---------------------------------------------
	OnExposedToExplosion = function(self, entity, data)
		entity:SelectPipe(0,"su_fast_threat_reaction");
	end,

	---------------------------------------------
	OnGroupMemberMutilated = function(self, entity)
--		System.Log(">>"..entity:GetName().." OnGroupMemberMutilated");
--		AI.Signal(SIGNALFILTER_SENDER,1,"GO_TO_PANIC",entity.id);
	end,

	---------------------------------------------
	OnTargetCloaked = function(self, entity)
--		AI.NotifyGroupTacticState(entity.id, 0, GN_NOTIFY_UNAVAIL);
--		entity:SelectPipe(0,"sn_target_cloak_reaction");
	end,

	--------------------------------------------------	
	OnOutOfAmmo = function (self,entity, sender)
		-- player would not have Reload implemented
		if (entity.Reload == nil) then
--			System.Log("  - no reload available");
			do return end
		end
		entity:Reload();
	end,

	---------------------------------------------
	OnGrenadeDanger = function(self, entity, sender)
		entity:SelectPipe(0,"su_fast_threat_reaction");
	end,

	---------------------------------------------
	OnCloseCollision = function(self, entity, data)
		entity:SelectPipe(0,"su_fast_threat_reaction");
	end,
}
