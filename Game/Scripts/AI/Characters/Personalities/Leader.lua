-- COVER CHARACTER SCRIPT

AICharacter.Leader = {

	Class = UNIT_CLASS_INFANTRY,
	TeamRole = GU_HUMAN_LEADER,
	
	Constructor = function(self,entity)
--		AI.Signal(SIGNALFILTER_SENDER,1,"ConstructorCover2",entity.id);
		entity.AI.target = {x=0, y=0, z=0};
		entity.AI.targetFound = 0;
		AI_Utils:SetupTerritory(entity);
		AI_Utils:SetupStandby(entity);
		AI.NotifyGroupTacticState(entity.id, 0, GN_INIT, 0);
	end,
	
	AnyBehavior = {
		--TheOtherSide
		GO_TO_TOSSHARED = "TOSSHARED",
		GO_TO_TOS_OBEY = "TOS_Obey",
		--~TheOtherSide
		ENTERING_VEHICLE =				"EnteringVehicle",
		RETURN_TO_FIRST =					"FIRST",
		USE_MOUNTED_WEAPON = 			"UseMounted",
		USE_MOUNTED_WEAPON_INIT = "UseMountedIdle",
		GO_TO_DUMB =							"Dumb",
		GO_TO_AVOIDEXPLOSIVES =   "Cover2AvoidExplosives",
		GO_TO_AVOIDVEHICLE =   		"Cover2AvoidVehicle",
		GO_TO_CHECKDEAD =					"CheckDead",
		OnFallAndPlay =						"HBaseTranquilized",

		GO_TO_ATTACK				= "Cover2Attack",
		GO_TO_ATTACK_GROUP	= "Cover2AttackGroup",
		GO_TO_RUSH_ATTACK	= "Cover2RushAttack",
		GO_TO_HIDE					= "Cover2Hide",
		GO_TO_AVOID_TANK		= "Cover2AvoidTank",
		GO_TO_RPG_ATTACK		= "Cover2RPGAttack",
		GO_TO_THREATENED		= "Cover2Threatened",
		GO_TO_THREATENED_STANDBY = "Cover2ThreatenedStandby",
		GO_TO_INTERESTED		= "Cover2Interested",
		GO_TO_SEEK					= "Cover2Seek",
		GO_TO_SEARCH				= "Cover2Search",
		GO_TO_RELOAD				= "Cover2Reload",
		GO_TO_CALL_REINFORCEMENTS	= "Cover2CallReinforcements",
		GO_TO_IDLE					= "Cover2Idle",
		GO_TO_PANIC					= "Cover2Panic",
		GO_TO_PREVIOUS				= "PREVIOUS",
		
		GO_TO_STATIC 			= "HBaseStaticShooter"
		
	},
	--TheOtherSide
	TOS_Obey = {
		-- Стандартные сигналы
		-- переопределяют те, что в AnyBehavior
		RESUME_FOLLOWING                 = "",
		ENTERING_VEHICLE                 = "",
		USE_MOUNTED_WEAPON               = "",
		OnPlayerSeen                     = "",
		OnTankSeen                       = "",
		OnHeliSeen                       = "",
		OnBulletRain                     = "",
		OnGrenadeSeen                    = "",
		OnInterestingSoundHeard          = "",
		OnThreateningSoundHeard          = "",
		exited_vehicle                   = "",
		exited_vehicle_investigate       = "",
		OnSomethingSeen                  = "",
		GO_TO_AVOIDEXPLOSIVES            = "",
		GO_TO_AVOIDVEHICLE               = "",
		GO_TO_CHECKDEAD                  = "",
		GO_TO_IDLE                       = "",
		GO_TO_ATTACK                     = "",
		GO_TO_ATTACK_GROUP               = "",
		GO_TO_RUSH_ATTACK                = "",
		GO_TO_HIDE                       = "",
		GO_TO_AVOID_TANK                 = "",
		GO_TO_RPG_ATTACK                 = "",
		GO_TO_THREATENED                 = "",
		GO_TO_THREATENED_STANDBY         = "",
		GO_TO_INTERESTED                 = "",
		GO_TO_SEEK                       = "",
		GO_TO_SEARCH                     = "",
		GO_TO_RELOAD                     = "",
		GO_TO_CALL_REINFORCEMENTS        = "",
		GO_TO_PANIC                      = "",
		GO_TO_STATIC              		 = "",
		ENEMYSEEN_FIRST_CONTACT          = "",
		ENEMYSEEN_DURING_COMBAT          = "",
		OnFallAndPlayWakeUp              = "Cover2Idle",
		OnBackOffFailed 				 = "",

		-- Форсированные стандартные сигналы
		-- ИИ поведения, созданные крайтеками используют
		-- стандартные сигналы, а эти нужны для
		-- вызовов при завершении приказа или в аналогичных ситуациях,
		-- где нужен 100% переход в нужное поведение.
		GO_TO_ATTACK_FORCED              = "Cover2Attack",
		GO_TO_ATTACK_GROUP_FORCED        = "Cover2AttackGroup",
		GO_TO_RUSH_ATTACK_FORCED         = "Cover2RushAttack",
		GO_TO_HIDE_FORCED                = "Cover2Hide",
		GO_TO_AVOID_TANK_FORCED          = "Cover2AvoidTank",
		GO_TO_RPG_ATTACK_FORCED          = "Cover2RPGAttack",
		GO_TO_THREATENED_FORCED          = "Cover2Threatened",
		GO_TO_THREATENED_STANDBY_FORCED  = "Cover2ThreatenedStandby",
		GO_TO_INTERESTED_FORCED          = "Cover2Interested",
		GO_TO_SEEK_FORCED                = "Cover2Seek",
		GO_TO_SEARCH_FORCED              = "Cover2Search",
		GO_TO_RELOAD_FORCED              = "Cover2Reload",
		GO_TO_CALL_REINFORCEMENTS_FORCED = "Cover2CallReinforcements",
		GO_TO_IDLE_FORCED                = "Cover2Idle",
		GO_TO_PANIC_FORCED               = "Cover2Panic",
		GO_TO_PREVIOUS_FORCED            = "PREVIOUS",
		GO_TO_STATIC_FORCED              = "HBaseStaticShooter",
	},

	TOSSHARED = {

		-- Стандартные сигналы
		-- переопределяют те, что в AnyBehavior
		RESUME_FOLLOWING                 = "",
		ENTERING_VEHICLE                 = "",
		USE_MOUNTED_WEAPON               = "",
		OnPlayerSeen                     = "",
		OnTankSeen                       = "",
		OnHeliSeen                       = "",
		OnBulletRain                     = "",
		OnGrenadeSeen                    = "",
		OnInterestingSoundHeard          = "",
		OnThreateningSoundHeard          = "",
		entered_vehicle                  = "",
		exited_vehicle                   = "",
		exited_vehicle_investigate       = "",
		OnSomethingSeen                  = "",
		GO_TO_AVOIDEXPLOSIVES            = "",
		GO_TO_AVOIDVEHICLE               = "",
		GO_TO_CHECKDEAD                  = "",
		GO_TO_IDLE                       = "",
		GO_TO_ATTACK                     = "",
		GO_TO_ATTACK_GROUP               = "",
		GO_TO_RUSH_ATTACK                = "",
		GO_TO_HIDE                       = "",
		GO_TO_AVOID_TANK                 = "",
		GO_TO_RPG_ATTACK                 = "",
		GO_TO_THREATENED                 = "",
		GO_TO_THREATENED_STANDBY         = "",
		GO_TO_INTERESTED                 = "",
		GO_TO_SEEK                       = "",
		GO_TO_SEARCH                     = "",
		GO_TO_RELOAD                     = "",
		GO_TO_CALL_REINFORCEMENTS        = "",
		GO_TO_PANIC                      = "",
		GO_TO_STATIC              		 = "",
		ENEMYSEEN_FIRST_CONTACT          = "",
		ENEMYSEEN_DURING_COMBAT          = "",
		OnFallAndPlayWakeUp              = "Cover2Idle",
		OnBackOffFailed 				 = "",
	},
	--~TheOtherSide
	
	HBaseAlerted = {
			
	},
	
	HBaseTranquilized = {
		GO_TO_IDLE =							"Cover2Idle",
		RESUME_FOLLOWING =				"",
		ENTERING_VEHICLE =				"",
		USE_MOUNTED_WEAPON =			"",
		OnPlayerSeen =						"",
		OnTankSeen =							"",
		OnHeliSeen =							"",
		OnBulletRain =						"",
		OnGrenadeSeen =						"",
		OnInterestingSoundHeard =	"",
		OnThreateningSoundHeard =	"",
		entered_vehicle	=					"",
		exited_vehicle =					"",
		exited_vehicle_investigate = "",
		OnSomethingSeen =					"",
		GO_TO_SEEK =							"",
		GO_TO_SEARCH =						"",
		GO_TO_ATTACK =						"",
		GO_TO_AVOIDEXPLOSIVES =   "",
		GO_TO_AVOIDVEHICLE =   		"",
		GO_TO_ALERT =							"",
		GO_TO_CHECKDEAD =					"",
	},
	
	UseMounted = {
		ORDER_HIDE = "",
		ORDER_FIRE = "",
		USE_MOUNTED_WEAPON = "",
		ACT_GOTO = "Cover2Idle",
		ACT_FOLLOWPATH = "Cover2Idle",
	},

	UseMountedIdle = {
		USE_MOUNTED_WEAPON = "",
		--OnPlayerSeen = "UseMounted",
		--OnEnemyDamage = "UseMounted",
		--OnBulletRain = "UseMounted",
		TO_USE_MOUNTED = "UseMounted",
		TOO_FAR_FROM_WEAPON = "Cover2Idle",
		ACT_GOTO = "Cover2Idle",
		ACT_FOLLOWPATH = "Cover2Idle",
	},
	

	Cover2AvoidExplosives = {
		GO_TO_IDLE =							"Cover2Idle",
		RESUME_FOLLOWING =				"",
		ENTERING_VEHICLE =				"",
		USE_MOUNTED_WEAPON =			"",
		OnPlayerSeen =						"",
		OnBulletRain =						"",
		OnGrenadeSeen =						"",
		OnInterestingSoundHeard =	"",
		OnThreateningSoundHeard =	"",
		OnSomethingSeen =					"",
		OnNoTarget = 							"",
	},

	Cover2AvoidVehicles = {
		GO_TO_IDLE =							"Cover2Idle",
		RESUME_FOLLOWING =				"",
		ENTERING_VEHICLE =				"",
		USE_MOUNTED_WEAPON =			"",
	},


	LeaderIdle = {
	},

	Cover2Idle = {
	
		OnPlayerSeen    = "",
		OnInterestingSoundHeard = "",
		OnSomethingSeen	= "",
		OnThreateningSoundHeard = "",
		OnBulletRain		= "",
		OnNearMiss			= "",
		OnEnemyDamage		= "",
		OnGroupMemberDiedNearest 	= "",
		OnSomebodyDied	= "",
		ENEMYSEEN_FIRST_CONTACT	 		= "",
		ENEMYSEEN_DURING_COMBAT		= "",
		OnExplosionDanger		= "HBaseGrenadeRun",
		-----------------------------------
		-- Vehicles related
		entered_vehicle = "InVehicle",
		exited_vehicle 	= "PREVIOUS",
	},

	Cover2Panic = {
	},

	Cover2Interested = {
	},

	Cover2Threatened = {
	},
	
	Cover2Hide = {
	},
	
	Cover2AvoidTank = {
	},

	Cover2RPGAttack= {
	},
	
	Cover2Seek = {
	},

	Cover2Search = {
	},

	Cover2Attack = {
	},

	Cover2Reload = {
	},

	Cover2CallReinforcements = {
	},

	Cover2AttackGroup = {
	},
	
	CheckDead = {
	},
	
	-- Vehicles related signals
	-- there are some cases that you have to mask signals when you add in AnyBehavior.
	-- these charactors also should be supported for cover2/Sneaker/Camper 10/07/2006 tetsuji

	EnteringVehicle = {
		exited_vehicle = "FIRST",
		do_exit_vehicle= "FIRST",
		GO_TO_ATTACK				= "",
		GO_TO_ATTACK_GROUP	= "",
		GO_TO_HIDE					= "",
		GO_TO_THREATENED		= "",
		GO_TO_THREATENED_STANDBY = "",
		GO_TO_INTERESTED		= "",
		GO_TO_SEEK					= "",
		GO_TO_SEARCH				= "",
		GO_TO_PANIC				= "",
		entered_vehicle = "InVehicle",
		entered_vehicle_gunner = "InVehicleGunner",
		OnFallAndPlay    = "InVehicleTranquilized",
	},


	InVehicle = {

		exited_vehicle_investigate = "Job_Investigate",
		exited_vehicle = "FIRST",
		do_exit_vehicle= "FIRST",

		OnPlayerSeen =						"InVehicleAlerted",
		OnTankSeen =							"",
		OnHeliSeen =							"",
		OnInterestingSoundHeard =	"",
		OnSomethingSeen =					"",
		OnBulletRain =						"",
		OnEnemyDamage	=						"",
		OnDamage =								"",
		OnGroupMemberDied	=				"",
		INCOMING_FIRE =						"",
		ENEMYSEEN_FIRST_CONTACT	 		= "",
		ENEMYSEEN_DURING_COMBAT		= "",
		GO_TO_HIDE =							"",

		GO_TO_ATTACK				= "",
		GO_TO_ATTACK_GROUP	= "",
		GO_TO_HIDE					= "",
		GO_TO_THREATENED		= "",
		GO_TO_THREATENED_STANDBY = "",
		GO_TO_INTERESTED		= "",
		GO_TO_SEEK					= "",
		GO_TO_SEARCH				= "",
		GO_TO_AVOID_TANK		= "",
		GO_TO_PANIC				= "",

		controll_vehicle = "InVehicleControlled",

		OnFallAndPlay    = "InVehicleTranquilized",
	},
	
	InVehicleAlerted = {
		exited_vehicle_investigate = "Job_Investigate",
		exited_vehicle = "FIRST",
		do_exit_vehicle= "FIRST",

		OnPlayerSeen =						"",
		OnTankSeen =							"",
		OnHeliSeen =							"",
		OnInterestingSoundHeard =	"",
		OnSomethingSeen =					"",
		OnBulletRain =						"",
		OnEnemyDamage	=						"",
		OnDamage =								"",
		OnGroupMemberDied	=				"",
		INCOMING_FIRE =						"",
		ENEMYSEEN_FIRST_CONTACT	 		= "",
		ENEMYSEEN_DURING_COMBAT		= "",
		GO_TO_HIDE =							"",

		GO_TO_ATTACK				= "",
		GO_TO_ATTACK_GROUP	= "",
		GO_TO_HIDE					= "",
		GO_TO_THREATENED		= "",
		GO_TO_THREATENED_STANDBY = "",
		GO_TO_INTERESTED		= "",
		GO_TO_SEEK					= "",
		GO_TO_SEARCH				= "",
		GO_TO_AVOID_TANK		= "",
		GO_TO_PANIC				= "",

		controll_vehicle = "InVehicleControlled",
		OnFallAndPlay    = "InVehicleTranquilized",
	},

	InVehicleGunner = {

		exited_vehicle_investigate = "Job_Investigate",
		exited_vehicle = "FIRST",
		do_exit_vehicle= "FIRST",

		OnPlayerSeen =						"",
		OnTankSeen =							"",
		OnHeliSeen =							"",
		OnInterestingSoundHeard =	"",
		OnSomethingSeen =					"",
		OnBulletRain =						"",
		OnEnemyDamage	=						"",
		OnDamage =								"",
		OnGroupMemberDied	=				"",
		INCOMING_FIRE =						"",
		ENEMYSEEN_FIRST_CONTACT	 		= "",
		ENEMYSEEN_DURING_COMBAT		= "",
		GO_TO_HIDE =							"",

		GO_TO_ATTACK				= "",
		GO_TO_ATTACK_GROUP	= "",
		GO_TO_HIDE					= "",
		GO_TO_THREATENED		= "",
		GO_TO_THREATENED_STANDBY = "",
		GO_TO_INTERESTED		= "",
		GO_TO_SEEK					= "",
		GO_TO_SEARCH				= "",
		GO_TO_AVOID_TANK		= "",
		GO_TO_PANIC				= "",

		controll_vehicleGunner = "InVehicleControlledGunner",

		OnFallAndPlay    = "InVehicleTranquilized",
	},

	InVehicleControlledGunner = {	
		exited_vehicle_investigate = "Job_Investigate",
		exited_vehicle = "FIRST",
		do_exit_vehicle= "FIRST",

		OnPlayerSeen =						"",
		OnTankSeen =							"",
		OnHeliSeen =							"",
		OnInterestingSoundHeard =	"",
		OnSomethingSeen =					"",
		OnBulletRain =						"",
		OnEnemyDamage	=						"",
		OnDamage =								"",
		OnGroupMemberDied	=				"",
		INCOMING_FIRE =						"",
		ENEMYSEEN_FIRST_CONTACT	 		= "",
		ENEMYSEEN_DURING_COMBAT		= "",
		GO_TO_HIDE =							"",

		GO_TO_ATTACK				= "",
		GO_TO_ATTACK_GROUP	= "",
		GO_TO_HIDE					= "",
		GO_TO_THREATENED		= "",
		GO_TO_THREATENED_STANDBY = "",
		GO_TO_INTERESTED		= "",
		GO_TO_SEEK					= "",
		GO_TO_SEARCH				= "",
		GO_TO_AVOID_TANK		= "",
		GO_TO_PANIC				= "",

		OnFallAndPlay    = "InVehicleTranquilized",
	},

	InVehicleControlled = {	
		exited_vehicle_investigate = "Job_Investigate",
		exited_vehicle = "FIRST",
		do_exit_vehicle= "FIRST",

		OnPlayerSeen =						"",
		OnTankSeen =							"",
		OnHeliSeen =							"",
		OnInterestingSoundHeard =	"",
		OnSomethingSeen =					"",
		OnBulletRain =						"",
		OnEnemyDamage	=						"",
		OnDamage =								"",
		OnGroupMemberDied	=				"",
		INCOMING_FIRE =						"",
		ENEMYSEEN_FIRST_CONTACT	 		= "",
		ENEMYSEEN_DURING_COMBAT		= "",
		GO_TO_HIDE =							"",

		GO_TO_ATTACK				= "",
		GO_TO_ATTACK_GROUP	= "",
		GO_TO_HIDE					= "",
		GO_TO_THREATENED		= "",
		GO_TO_THREATENED_STANDBY = "",
		GO_TO_INTERESTED		= "",
		GO_TO_SEEK					= "",
		GO_TO_SEARCH				= "",
		GO_TO_AVOID_TANK		= "",
		GO_TO_PANIC				= "",

		OnFallAndPlay    = "InVehicleTranquilized",
	},

	Dumb = {
		GO_TO_IDLE =							"Cover2Idle",
		RESUME_FOLLOWING =				"",
		ENTERING_VEHICLE =				"",
		USE_MOUNTED_WEAPON =			"",
		OnPlayerSeen =						"",
		OnTankSeen =							"",
		OnHeliSeen =							"",
		OnBulletRain =						"",
		OnGrenadeSeen =						"",
		OnInterestingSoundHeard =	"",
		OnThreateningSoundHeard =	"",
		entered_vehicle	=					"",
		exited_vehicle =					"",
		exited_vehicle_investigate = "",
		OnSomethingSeen =					"",
		GO_TO_ATTACK				= "",
		GO_TO_ATTACK_GROUP	= "",
		GO_TO_RUSH_ATTACK	= "",
		GO_TO_HIDE					= "",
		GO_TO_AVOID_TANK		= "",
		GO_TO_RPG_ATTACK		= "",
		GO_TO_THREATENED		= "",
		GO_TO_THREATENED_STANDBY = "",
		GO_TO_INTERESTED		= "",
		GO_TO_SEEK					= "",
		GO_TO_SEARCH				= "",
		GO_TO_RELOAD				= "",
		GO_TO_CALL_REINFORCEMENTS	= "",
		ENEMYSEEN_FIRST_CONTACT	 		= "",
		ENEMYSEEN_DURING_COMBAT		= "",
		GO_TO_PANIC				= "",
	},

	InVehicleTranquilized = {
		OnFallAndPlayWakeUp = 		"PREVIOUS",
		OnFallAndPlay			= "",
		RESUME_FOLLOWING =				"",
		ENTERING_VEHICLE =				"",
		USE_MOUNTED_WEAPON =			"",
		OnPlayerSeen =						"",
		OnBulletRain =						"",
		OnGrenadeSeen =						"",
		OnInterestingSoundHeard =	"",
		OnThreateningSoundHeard =	"",
		entered_vehicle	=					"",
		exited_vehicle =					"",
		exited_vehicle_investigate = "",
		OnSomethingSeen =					"",
		GO_TO_SEEK =							"",
		GO_TO_IDLE = 							"",
		GO_TO_SEARCH =						"",
		GO_TO_ATTACK =						"",
		GO_TO_AVOIDEXPLOSIVES =   "",
		GO_TO_ALERT =							"",
		GO_TO_CHECKDEAD =					"",
	},
	
}
