/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

// ReSharper disable CppClangTidyCppcoreguidelinesSpecialMemberFunctions
// ReSharper disable CppInconsistentNaming
#pragma once
#include <functional>
#include <unordered_map>
#include <deque>

#include "IEntity.h"

#include "TheOtherSideMP/Game/Modules/GenericModule.h"
#include "TheOtherSideMP/TOSSmartStruct.h"

enum ETOSEntityFlags
{
    ENTITY_MUST_RECREATED = (1 << 0),
    ENTITY_RECREATION_SCHEDULED = (1 << 1),
};

// ------------------------------------------------------------
// 1) Параметры планировщика передачи власти
// ------------------------------------------------------------
struct STOSScheduleDelegateAuthorityParams
{
    bool    forceStartControl{ false };
    string  playerName;            // имя игрока, получающего власть
    float   scheduledTimeStamp{ 0.0f }; // когда запланирована передача
};

// ------------------------------------------------------------
// 2) Базовые параметры спавна сущности
// ------------------------------------------------------------
struct STOSEntitySpawnParams : public STOSSmartStruct
{
    SEntitySpawnParams        vanilla{};               // оригинальные параметры
    SmartScriptTable          properties;
    SmartScriptTable          propertiesInstance;

    bool                       hide{ false };
    bool                       moveSpawnedToAuthorityPos{ true };
    bool                       saveParams{ false };
    bool                       forceStartControl{ false };

    string                archetypeName;
    string                authorityName;
    string                name;

    uint32_t                   tosFlags{ 0 };

    // Конструкторы и операторы — всё default, т.к. member-wise
    STOSEntitySpawnParams() = default;
    explicit STOSEntitySpawnParams(const SEntitySpawnParams& p)
        : vanilla(p)
    {
        if (vanilla.pArchetype)
        {
            archetypeName = vanilla.pArchetype->GetName();
        }
    }
    STOSEntitySpawnParams(const STOSEntitySpawnParams&) = default;
    STOSEntitySpawnParams& operator=(const STOSEntitySpawnParams&) = default;
    STOSEntitySpawnParams(STOSEntitySpawnParams&&) noexcept = default;
    STOSEntitySpawnParams& operator=(STOSEntitySpawnParams&&) noexcept = default;
    virtual ~STOSEntitySpawnParams() = default;
};

using TCallback = std::function<void(EntityId, const Vec3&, int)>;

// ------------------------------------------------------------
// 3) Отложенный спавн — наследует все конструкторы базового
// ------------------------------------------------------------
struct STOSEntityDelaySpawnParams : public STOSEntitySpawnParams
{
    // унаследовать все конструкторы из STOSEntitySpawnParams
    using STOSEntitySpawnParams::STOSEntitySpawnParams;

    float scheduledTimeStamp{ 0.0f };
    float spawnDelay{ 0.0f };
    int   clientChannelId{ 0 };
    TCallback callback;

    // Остальные спецфункции — default
    STOSEntityDelaySpawnParams() = default;
    STOSEntityDelaySpawnParams(const STOSEntityDelaySpawnParams&) = default;
    STOSEntityDelaySpawnParams& operator=(const STOSEntityDelaySpawnParams&) = default;
    STOSEntityDelaySpawnParams(STOSEntityDelaySpawnParams&&) noexcept = default;
    STOSEntityDelaySpawnParams& operator=(STOSEntityDelaySpawnParams&&) noexcept = default;
    virtual ~STOSEntityDelaySpawnParams() = default;
};

// Используем современные псевдонимы вместо typedef
using TEntities = std::vector<EntityId>;
using TMapAuthorityParams = std::unordered_map<EntityId, STOSScheduleDelegateAuthorityParams>;
using TMapTOSParams = std::unordered_map<EntityId, _smart_ptr<STOSEntitySpawnParams>>;
using TDelayTOSParams = std::deque<_smart_ptr<STOSEntityDelaySpawnParams>>;

class CTOSEntitySpawnModule : public CTOSGenericModule
{
public:
    struct SFrameTimer
    {
        SFrameTimer() = default;
        SFrameTimer(int framesDelta)
            : creationFrameId(gEnv->pRenderer->GetFrameID())
            , targetFrameId(creationFrameId + framesDelta)
        {
        };

        int creationFrameId{};
        int targetFrameId{};
    };

    CTOSEntitySpawnModule() noexcept = default;
    ~CTOSEntitySpawnModule() override = default;

    // ITOSGameModule interface
    void OnExtraGameplayEvent(IEntity* pEntity, const STOSGameEvent& event) override;
    const char* GetName() const override { return "ModuleEntitySpawn"; }
    void Init() override;
    void Update(float frametime) override;
    void Serialize(TSerialize ser) override;
    void InitCVars(IConsole* pConsole) override;
    void InitCCommands(IConsole* pConsole) override;
    void ReleaseCVars() override;
    void ReleaseCCommands() override;
    void GetMemoryStatistics(ICrySizer* s) override;
    void DumpModuleInfo() override;
    CScriptableBase* GetScriptBind() override;
    void InitScriptBinds() override;
    void ReleaseScriptBinds() override;
    bool OnInputEvent(const SInputEvent& event) override;
    bool OnInputEventUI(const SInputEvent& event) override;
    // ~ITOSGameModule

    void Reset() noexcept;

    // Console commands
    static void CmdSpawnEntity(IConsoleCmdArgs* pArgs);
    static void CmdRemoveEntity(IConsoleCmdArgs* pArgs);
    static void CmdRemoveEntityForced(IConsoleCmdArgs* pArgs);
    static void CmdDumpSpawned(IConsoleCmdArgs* pArgs);
    static void CmdDumpEntityRotation(IConsoleCmdArgs* pArgs);

    // Спавн сущностей
    static IEntity* SpawnEntity(STOSEntitySpawnParams& params, bool sendTosEvent = true);
    static bool SpawnEntityDelay(STOSEntityDelaySpawnParams& params, bool sendTosEvent = true);
    static void RemoveEntityForced(EntityId id);

    // Отложенное удаление
    void RemoveEntityDelayed(EntityId id, int framesCount);

    IEntity* GetSpawnedSlave(const char* authorityPlayerName) const;

private:
    void ProcessDelayedRemoving();
    void ProcessDelayedSpawning();
    void ProcessRecreating();
    void ProcessAuthorityDelegation();
    //void ProcessDelayedCallbacks();
    bool MustBeRecreated(const IEntity* pEntity) const;
    void HandleSpawnedActor(IActor* pActor, const STOSEntitySpawnParams& params);
    IEntity* SpawnAndInit(IEntitySystem* pEntitySystem, STOSEntitySpawnParams& params);
    bool AuthorityAlreadyHasSlave(const STOSEntitySpawnParams& params) const;
    bool ShouldSpawnImmediately(const STOSEntityDelaySpawnParams& params) const;
    void ScheduleDelayedSpawn(const STOSEntityDelaySpawnParams& params);
    void ScheduleCallback(const std::function<void()>& callback);
    void CleanupEntity(EntityId id);
    void ScheduleRecreation(const IEntity* pEntity);
    bool IsSpawned(const IEntity* pEntity) const;
    void DebugDraw(const Vec2& screenPos, float fontSize, float interval,
        int maxElemNum, bool draw) const;

    // Статические контейнеры модульного уровня
    static TEntities             s_markedForRecreation;
    static TDelayTOSParams       s_scheduledSpawnsDelay;

    // Экземплярные данные
    TMapAuthorityParams  m_scheduledAuthorities;
    TMapTOSParams        m_scheduledRecreations;
    TMapTOSParams        m_savedSpawnParams;
    std::map<EntityId, SFrameTimer> m_removeDelay;
    //std::deque<std::function<void()>> m_delayedCallbacks;
};
