--*************************************************************************
--AlienKeeper Source File.
--Copyright (C), AlienKeeper, 2024.
--*************************************************************************

-- Global variables
TextMessageCenter = 0
TextMessageConsole = 1
TextMessageError = 2
TextMessageInfo = 3
TextMessageServer = 4
TextMessageBig = 5

TextMessageToAll = 8
TextMessageToAllRemote = 65544
TextMessageToClient = 1
TextMessageToOtherClients = 4

eGE_DiscreetSample = 0
eGE_GameReset = 1
eGE_GameStarted = 2
eGE_GameEnd = 3
eGE_Connected = 4
eGE_Disconnected = 5
eGE_Renamed = 6
eGE_ChangedTeam = 7
eGE_Died = 8
eGE_Scored = 9
eGE_Currency = 10
eGE_Rank = 11
eGE_Spectator = 12
eGE_ScoreReset = 13
eGE_AttachedAccessory = 14
eGE_ZoomedIn = 15
eGE_ZoomedOut = 16
eGE_Kill = 17
eGE_Death = 18
eGE_Revive = 19
eGE_SuitModeChanged = 20
eGE_Hit = 21
eGE_Damage = 22
eGE_WeaponHit = 23
eGE_WeaponReload = 24
eGE_WeaponShot = 25
eGE_WeaponMelee = 26
eGE_WeaponFireModeChanged = 27
eGE_Explosion = 28
eGE_ItemSelected = 29
eGE_ItemPickedUp = 30
eGE_ItemDropped = 31
eGE_ItemBought = 32
eGE_EnteredVehicle = 33
eGE_LeftVehicle = 34

eBLE_Information = 2
eBLE_Currency = 1
eBLE_Warning = 3
eBLE_System = 4

ent_static = 1
ent_sleeping_rigid = 2
ent_rigid = 4
ent_living = 8
ent_independent = 16
ent_deleted = 128
ent_terrain = 256

local g_SpawnParams_point = { x = 0, y = 0, z = 0 }
local g_SpawnParams_rot = { x = 0, y = -1, z = 0 };
local g_SpawnParams_scale = { x = 1, y = 1, z = 1 }

g_SpawnParams = {
    name = "",
    class = "",
    archetype = "",
    flags = 0, -- ENTITY_FLAG_NEVER_NETWORK_STATIC ||| ENTITY_FLAG_ON_RADAR ||| ENTITY_FLAG_CASTSHADOW ||| ENTITY_FLAG_UNREMOVABLE ||| ENTITY_FLAG_CLIENT_ONLY ||| ENTITY_FLAG_SERVER_ONLY ||| ENTITY_FLAG_AI_HIDEABLE ||| ENTITY_FLAG_HAS_AI
    position = g_SpawnParams_point,
    orientation = g_SpawnParams_rot,
    scale = g_SpawnParams_scale,
    properties = {},
    propertiesInstance = {},

    reset = function(self)
        self.name = ""
        self.class = ""
        self.archetype = ""
        self.flags = 0
        self.position = g_SpawnParams_point
        self.orientation = g_SpawnParams_rot
        self.scale = g_SpawnParams_scale
        self.properties = {}
        self.propertiesInstance = {}
    end
}

g_HitExample = {
	dir=g_Vectors.v000;
	radius = 0;	
	partId = -1;
	target = nil;
	targetId = nil;
	weapon = nil;
	weaponId = nil;
	shooter = nil;
	shooterId = nil;
	materialId = 0;
	damage = 0;
	typeId = 0;
	type = "normal";
}

function LogAlways(fmt, ...)
    System.LogAlways(string.format(fmt, ...));
end

function __FILE__() return debug.getinfo(2, 'S').source end

function __LINE__() return debug.getinfo(2, 'l').currentline end

function __FUNC__() return debug.getinfo(2, 'n').name end
