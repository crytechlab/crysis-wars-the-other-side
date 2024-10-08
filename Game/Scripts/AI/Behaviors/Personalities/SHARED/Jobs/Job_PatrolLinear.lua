-- linear patroling behaviour - 
--Each character will patrol a set of TagPoints in a linear way ie 1 2 3 4 5 4 3 2 1 etc

--The character will approach each TagPoint sequentialy, look in the direction of the current TagPoint, 
--make some idles and look around. After the last TagPoint he will return to his first point and start again.
-- created by sten: 		18-09-2002
-- last modified by petar
--------------------------

AIBehaviour.Job_PatrolLinear = {
	Name = "Job_PatrolLinear",
	JOB = 1,
	
	

	-- SYSTEM EVENTS			-----
	---------------------------------------------
	Constructor = function(self,entity )
		
		--TheOtherSide
		entity.AI.previousBehaviour = entity.AI.currentBehaviour
		entity.AI.currentBehaviour = self.Name
		--~TheOtherSide	

		entity.bGunReady=true;
		entity:InitAIRelaxed();
		entity.AI.PathStep = 0;
		entity.AI.SignIncrement = 1;

		entity:CheckWalkFollower();
		self:PatrolPath(entity);
		
	end,

	Destructor = function(self,entity )
--		entity:InitAICombat();
	end,

	OnJobContinue = function(self,entity )
		entity:InitAIRelaxed();
		self:PatrolPath(entity);
	end,
	---------------------------------------------		
	OnBored = function (self, entity)
		entity:MakeRandomConversation();
	end,
	----------------------------------------------------FUNCTIONS 
	PatrolPath = function (self, entity, sender)
		-- select next tagpoint for patrolling
		local name = entity:GetName();


		local tpname = name.."_P"..entity.AI.PathStep;

--		local TagPoint = Game:GetTagPoint(name.."_P"..entity.AI.PathStep);
		local TagPoint = System.GetEntityByName(name.."_P"..entity.AI.PathStep);
		if (TagPoint== nil) then 		

			if (entity.AI.PathStep == 0) then 
				AI.Warning(" Entity "..name.." has a path job but no specified path points.");
				do return end
			end

			entity.AI.SignIncrement = -entity.AI.SignIncrement;
			entity.AI.PathStep = entity.AI.PathStep + entity.AI.SignIncrement;
			tpname = name.."_P"..(entity.AI.PathStep + entity.AI.SignIncrement);
		end

		entity:SelectPipe(0,"patrol_approach_to",tpname);
		
		entity.AI.PathStep = entity.AI.PathStep + entity.AI.SignIncrement;
	end,
	
	------------------------------------------------------------------------
	-- GROUP SIGNALS
	------------------------------------------------------------------------
	BREAK_AND_IDLE = function (self, entity, sender)
	end,
	------------------------------------------------------------------------	
	
}

 