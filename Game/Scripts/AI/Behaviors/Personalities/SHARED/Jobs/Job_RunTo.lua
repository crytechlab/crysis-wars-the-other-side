
-- created by petar
--------------------------


AIBehaviour.Job_RunTo = {
	Name = "Job_RunTo",				
	JOB = 1,


	---------------------------------------------
	Constructor = function(self,entity )
		entity:InitAIRelaxed();
		
		--TheOtherSide
		entity.AI.previousBehaviour = entity.AI.currentBehaviour
		entity.AI.currentBehaviour = self.Name
		--~TheOtherSide	


		AI.CreateGoalPipe("run_to");
		AI.PushGoal("run_to","run",0,1);
		if (entity.cnt:GetCurrWeapon()) then
			AI.PushGoal("run_to","bodypos",0,BODYPOS_STAND);
		else
			if (entity.Properties.special == 1) then 
				AI.SetIgnorant(entity.id,1);
			end
			AI.PushGoal("run_to","bodypos",0,BODYPOS_RELAX);
		end
		
		AI.PushGoal("run_to","acqtarget",0,"");
		AI.PushGoal("run_to","approach",1,1);

		local run_target = entity:GetName().."_RUNTO";

		-- try to get tagpoint of the same name as yourself first
		local TagPoint = System.GetEntityByName(run_target);

		if (TagPoint) then 
			entity:SelectPipe(0,"run_to",run_target);	
		else
			AI.Warning( "[AI] Entity "..entity:GetName().." has run job assigned to it but no tag point name_RUNTO.");
		end

		entity:InsertSubpipe(0,"setup_idle");
	end,

	
}


