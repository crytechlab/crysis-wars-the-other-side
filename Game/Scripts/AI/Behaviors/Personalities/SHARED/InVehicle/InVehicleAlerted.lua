--------------------------------------------------
--    Created By: Kirill
--		same as inVehicle, differs only by alertness level
--------------------------
--

AIBehaviour.InVehicleAlerted = {
	Name = "InVehicleAlerted",
	Base = "InVehicle",	
	alertness = 2,
	exclusive = 1,

	Constructor = function(self, entity)

		--TheOtherSide
		AIBehaviour.InVehicle:Constructor(entity);
		entity.AI.currentBehaviour = "InVehicleAlerted";
		--~TheOtherSide
	end,

	Destructor = function( self, entity )	
		entity:SelectPipe(0,"do_nothing");
		entity:InsertSubpipe(AIGOALPIPE_NOTDUPLICATE,"clear_all");
	end,

}