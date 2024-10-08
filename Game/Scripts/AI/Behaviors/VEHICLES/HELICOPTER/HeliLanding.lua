--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Helocipter Behaviour SCRIPT
--  
--------------------------------------------------------------------------
--  History:
--  - 12/06/2006   : the first implementation by Tetsuji
--
--------------------------------------------------------------------------

AIBehaviour.HeliLanding = {
	Name = "HeliLanding",
	Base = "HeliReinforcement",
	alertness = 0,

	-- SYSTEM EVENTS			-----
	---------------------------------------------
	Constructor = function( self, entity, sender, data )

		--TheOtherSide
		entity.AI.previousBehaviour = entity.AI.currentBehaviour
		entity.AI.currentBehaviour = self.Name
		--~TheOtherSide	

		AIBehaviour.HeliReinforcement:Constructor( entity );

	end,

}
