--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2007.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Scout
--  
--------------------------------------------------------------------------
--  History:
--  - 11/12/2004   : Created by Kirill Bulatsev
--  - 05/04/2005   : CXP Rush Clean up by Mikko Mononen
--	- 29/11/2005   : Revised for new attack patterns by Tetsuji Iwasaki
--	- 15/01/2007   : Separated as the Melee Scout by Tetsuji Iwasaki
--------------------------------------------------------------------------

--------------------------------------------------------------------------
AIBehaviour.ScoutMeleeIdle = {
	Name = "ScoutMeleeIdle",
	Base = "ScoutMeleeDefault",
	alertness = 0,

	--------------------------------------------------------------------------
	Constructor = function ( self, entity, data )
		
		--TheOtherSide
		entity.AI.previousBehaviour = entity.AI.currentBehaviour
		entity.AI.currentBehaviour = self.Name
		--~TheOtherSide	

		AIBehaviour.ScoutMOACIdle:Constructor( entity, data );
	end,
	--------------------------------------------------------------------------
	Destructor = function ( self, entity, data )
	end,
	--------------------------------------------------------------------------
	OnPlayerSeen = function( self, entity, fDistance )
		AI.Signal(SIGNALFILTER_SENDER, 1, "TO_SCOUTMELEE_PATROL", entity.id);
	end,
	--------------------------------------------------------------------------
	OnBulletRain = function ( self, entity, sender, data )
		self:OnEnemyDamage( entity );
	end,
	--------------------------------------------------------------------------
	OnSoreDamage = function ( self, entity, sender, data )
		self:OnEnemyDamage( entity );
	end,
	--------------------------------------------------------------------------
	OnEnemyDamage = function ( self, entity, sender, data )
		AI.Signal(SIGNALFILTER_SENDER, 1, "TO_SCOUTMELEE_PATROL", entity.id);
	end,

	-------------------------------------------------------
	-- debug
	CHECK_TROOPER_GROUP = function(self,entity,sender)
		AI.Warning(entity:GetName().. " IS IN SAME GROUP WITH TROOPER "..sender:GetName()..", groupid = "..AI.GetGroupOf(entity.id));
	end,

}

