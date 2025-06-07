/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

// ReSharper disable CppLocalVariableMayBeConst
#include "StdAfx.h"
#include "IEntity.h"
#include "CryMemoryAllocator.h"
#include "CryMemoryManager.h"

#include "Game.h"
#include "GameRules.h"

#include "EntitySpawnModule.h"
#include "TheOtherSideMP/Helpers/TOS_Script.h"
#include "TheOtherSideMP/Helpers/TOS_Entity.h"
#include "TheOtherSideMP/Helpers/TOS_STL.h"
#include "TheOtherSideMP/Game/TOSGame.h"
#include "TheOtherSideMP/Game/TOSGameEventRecorder.h"
#include "TheOtherSideMP/Game/Modules/Zeus/ZeusSynchronizer.h"
#include "TheOtherSideMP/Game/Modules/Zeus/ZeusModule.h"


TEntities CTOSEntitySpawnModule::s_markedForRecreation;
TDelayTOSParams CTOSEntitySpawnModule::s_scheduledSpawnsDelay;

void CTOSEntitySpawnModule::GetMemoryStatistics(ICrySizer* s)
{
    s->Add(this);
    s->AddContainer(s_markedForRecreation);
    s->AddContainer(s_scheduledSpawnsDelay);
    s->AddContainer(m_scheduledRecreations);
    s->AddContainer(m_scheduledAuthorities);
    s->AddContainer(m_savedSpawnParams);
    s->AddContainer(m_removeDelay);
}

void CTOSEntitySpawnModule::DumpModuleInfo()
{
    
}

CScriptableBase *CTOSEntitySpawnModule::GetScriptBind()
{
    return nullptr;
}

void CTOSEntitySpawnModule::InitScriptBinds()
{
}

void CTOSEntitySpawnModule::ReleaseScriptBinds()
{
}

bool CTOSEntitySpawnModule::OnInputEvent(const SInputEvent &event)
{
    return false;
}

bool CTOSEntitySpawnModule::OnInputEventUI(const SInputEvent &event)
{
    return false;
}

void CTOSEntitySpawnModule::OnExtraGameplayEvent(IEntity* pEntity, const STOSGameEvent& event)
{
    TOS_INIT_EVENT_VALUES(pEntity, event);

    if (!gEnv->bServer)
        return;

    switch (event.event)
    {
    case eEGE_TOSEntityScheduleDelegateAuthority:
    {
        const char* playerName = event.description;

        auto it = m_scheduledAuthorities.find(entId);
        if (it == m_scheduledAuthorities.end())
        {
            m_scheduledAuthorities[entId].forceStartControl = static_cast<bool>(event.int_value);
            m_scheduledAuthorities[entId].playerName = playerName;
            m_scheduledAuthorities[entId].scheduledTimeStamp = gEnv->pTimer->GetFrameStartTime().GetSeconds();
        }

        break;
    }
    case eEGE_TOSEntityOnSpawn:
    {
        auto pParams = new STOSEntitySpawnParams(*static_cast<STOSEntitySpawnParams*>(event.extra_data));
        assert(pParams);

        if (pParams->saveParams && !IsSpawned(pEntity))
        {

            // id должен генерироваться
            pParams->vanilla.id = 0;

            //pParams->sName = pEntity->GetName();

            if (gEnv->pSystem->IsDevMode())
                CryLog("<c++> [OnExtraGameplayEvent] Save spawn params for spawned entity with saved name '%s', with real name '%s', id '%i'", pParams->name, pEntity->GetName(), entId);

            m_savedSpawnParams[entId] = pParams;
        }

        break;
    }
    case eEGE_EntityOnRemove:
    {
        if (IsSpawned(pEntity))
        {
            //just log
            TOS_RECORD_EVENT(entId, STOSGameEvent(eEGE_TOSEntityOnRemove, "", true));
        }

        //3
        if (MustBeRecreated(pEntity))
        {
            ScheduleRecreation(pEntity);
        }

        break;
    }
    case eEGE_SvGameRestarting:
    case eEGE_OnLevelLoadingStart:
    {
        Reset();
    }
    default:
        break;
    }
}

void CTOSEntitySpawnModule::Reset()	noexcept
{
    Init();
}

void CTOSEntitySpawnModule::Init()
{
    CTOSGenericModule::Init();

    m_scheduledRecreations.clear();
    m_savedSpawnParams.clear();
    s_markedForRecreation.clear();
    m_scheduledAuthorities.clear();
    s_scheduledSpawnsDelay.clear();
    m_removeDelay.clear();
}

void CTOSEntitySpawnModule::ProcessDelayedRemoving()
{
    for (auto it = m_removeDelay.begin(); it != m_removeDelay.end();)
    {
        const int currentFrameId = gEnv->pRenderer->GetFrameID();
        if (currentFrameId == it->second.targetFrameId)
        {
            gEnv->pEntitySystem->RemoveEntity(it->first);
            it = m_removeDelay.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void CTOSEntitySpawnModule::ProcessRecreating()
{
    auto it = m_scheduledRecreations.begin();
    auto end = m_scheduledRecreations.end();

    for (; it != end;)
    {
        const auto scheduledId = it->first;
        const auto pScheduledEnt = gEnv->pEntitySystem->GetEntity(scheduledId);

        auto pSpawnParams = it->second.get();

        // Проверка на необходимость выполнить пересоздание
        if (!pScheduledEnt && (pSpawnParams->tosFlags & ENTITY_RECREATION_SCHEDULED))
        {
            // После рекреации помечаем флагом, чтобы рекреация повторилась
            pSpawnParams->tosFlags = ENTITY_MUST_RECREATED;

            TOS_RECORD_EVENT(SpawnEntity(*pSpawnParams)->GetId(),
                STOSGameEvent(eEGE_TOSEntityRecreated, "", true));

            m_scheduledRecreations.erase(it++); // Удаление текущего элемента и переход к следующему
            break; // Выход из цикла после удаления элемента
        }
        else
        {
            ++it; // Переход к следующему элементу, если условие не выполнено
        }
    }
}

void CTOSEntitySpawnModule::ProcessAuthorityDelegation()
{
    for (TMapAuthorityParams::iterator it = m_scheduledAuthorities.begin(); it != m_scheduledAuthorities.end();)
    {
        EntityId scheduledId = it->first;
        IEntity* pScheduledEnt = gEnv->pEntitySystem->GetEntity(scheduledId);
        if (!pScheduledEnt)
            break;

        const char* schedName = pScheduledEnt->GetName();
        const char* playerName = it->second.playerName.c_str();
        const bool forceStartControl = it->second.forceStartControl;

        IEntity* pPlayerEnt = gEnv->pEntitySystem->FindEntityByName(playerName);

        if (pPlayerEnt)
        {
            IGameObject* pGO = g_pGame->GetIGameFramework()->GetGameObject(pPlayerEnt->GetId());
            if (!pGO)
                break;

            const auto playerChannelId = pGO->GetChannelId();
            INetChannel* pPlayerNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(playerChannelId);
            INetContext* pNetContext = g_pGame->GetIGameFramework()->GetNetContext();

            bool isAuth = pNetContext->RemoteContextHasAuthority(pPlayerNetChannel, scheduledId);
            if (!isAuth) pNetContext->DelegateAuthority(scheduledId, pPlayerNetChannel);
            else
            {
                m_scheduledAuthorities.erase(it++);
                char buffer[256];
                sprintf(buffer, "%s take own of %s", playerName, schedName);
                TOS_RECORD_EVENT(0,
                    STOSGameEvent(eEGE_TOSEntityAuthorityDelegated, buffer, true));

                if (forceStartControl)
                    TOS_RECORD_EVENT(scheduledId,
                        STOSGameEvent(eEGE_ForceStartControl, "", true, false, nullptr, 0.0f, playerChannelId));
            }

            break;
        }
        else
        {
            const float currentTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();

            if (currentTime - it->second.scheduledTimeStamp > 10.0f)
            {
                m_scheduledAuthorities.erase(it++);
                break;
            }
            else
            {
                ++it;
            }
        }
    }
}

//void CTOSEntitySpawnModule::ProcessDelayedCallbacks()
//{
//    while (!m_delayedCallbacks.empty())
//    {
//        auto& callback = m_delayedCallbacks.front();
//        callback();
//        m_delayedCallbacks.pop_front();
//    }
//}

void CTOSEntitySpawnModule::Update(float frametime)
{
    if (!gEnv->bServer)
        return;

    ProcessDelayedRemoving();
    ProcessDelayedSpawning();
    ProcessRecreating();
    ProcessAuthorityDelegation();
    //ProcessDelayedCallbacks();
}

void CTOSEntitySpawnModule::Serialize(TSerialize ser)
{
}

IEntity* CTOSEntitySpawnModule::SpawnEntity(STOSEntitySpawnParams& params, bool sendEvent /*= true*/)
{
    CRY_ASSERT_MESSAGE(gEnv->bServer,
        "Entity spawning process only can be on the server");
    if (!gEnv->bServer)
        return nullptr;

    const auto pEntitySystem = gEnv->pEntitySystem;
    if (!pEntitySystem)
        return nullptr;

    const auto pSpawnModule = g_pTOSGame->GetEntitySpawnModule();
    if (!pSpawnModule)
        return nullptr;

    // 1. Проверка: у игрока уже есть «раб»
    if (pSpawnModule->AuthorityAlreadyHasSlave(params))
        return nullptr;

    auto pSpawned = pSpawnModule->SpawnAndInit(pEntitySystem, params);
    if (!pSpawned)
        return nullptr;

    const EntityId spawnedId = pSpawned->GetId();

    if (auto pActor = TOS_GET_ACTOR(spawnedId))
        pSpawnModule->HandleSpawnedActor(pActor, params);

    if (sendEvent)
        TOS_RECORD_EVENT(spawnedId, STOSGameEvent(eEGE_TOSEntityOnSpawn, "", true, false, &params));

    if (params.tosFlags & ENTITY_MUST_RECREATED)
    {
        auto alreadyInside = stl::find(s_markedForRecreation, spawnedId);
        if (!alreadyInside)
        {
            s_markedForRecreation.push_back(spawnedId);
            TOS_RECORD_EVENT(spawnedId, STOSGameEvent(eEGE_TOSEntityMarkForRecreation, "", true));
        }
    }

    // Планирование передачи игроку власти на сущность
    // Осуществление самой передачи происходит тогда, когда указатель на игрока будет валидным
    if (!params.authorityName.empty())
    {
        const char* plName = params.authorityName;
        const bool forceStartControl = params.forceStartControl;

        TOS_RECORD_EVENT(spawnedId, STOSGameEvent(eEGE_TOSEntityScheduleDelegateAuthority, plName, true, false, nullptr, 0.0f, forceStartControl));
    }

    pSpawned->Hide(params.hide);
    pSpawned->EnablePhysics(!params.hide);

    CTOSZeusSynchronizer::NetHideParams hideParams;
    hideParams.bHide = params.hide;
    hideParams.id = pSpawned->GetId();

    auto pZeusModule = g_pTOSGame->GetZeusModule();
    if (pZeusModule)
    {
        auto pSync = pZeusModule->GetSynchronizer();
        if (pSync)
            pSync->RMISend(CTOSZeusSynchronizer::ClHideEntity(), hideParams, eRMI_ToAllClients | eRMI_NoLocalCalls);
    }

    return pSpawned;
}

void CTOSEntitySpawnModule::ProcessDelayedSpawning()
{
    while (!s_scheduledSpawnsDelay.empty())
    {
        auto& pSpawnParams = s_scheduledSpawnsDelay.front();

        const float curTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
        const float recordedTime = pSpawnParams->scheduledTimeStamp;
        const float delay = pSpawnParams->spawnDelay;

        if (curTime - recordedTime > delay)
        {
            if (auto pSpawned = SpawnEntity(*pSpawnParams, true))
            {
                if (pSpawnParams->callback)
                {
                    pSpawnParams->callback(
                        pSpawned->GetId(),
                        pSpawned->GetWorldPos(),
                        pSpawnParams->clientChannelId);


                    //ScheduleCallback([=]()
                    //    {
                    //        pSpawnParams->callback(
                    //            pSpawned->GetId(),
                    //            pSpawned->GetWorldPos(),
                    //            pSpawnParams->clientChannelId);
                    //    });
                }

            }

            s_scheduledSpawnsDelay.pop_front();
        }
        else
        {
            break; // Первый элемент ещё не готов — остальные тоже не готовы
        }
    }

}

void CTOSEntitySpawnModule::HandleSpawnedActor(IActor* pActor, const STOSEntitySpawnParams& params)
{
    if (params.moveSpawnedToAuthorityPos)
    {
        IEntity* pAuthorityPlayerEnt = gEnv->pEntitySystem->FindEntityByName(params.authorityName);
        if (pAuthorityPlayerEnt)
            g_pGame->GetGameRules()->MovePlayer(
                static_cast<CTOSActor*>(pActor),
                pAuthorityPlayerEnt->GetWorldPos(),
                Ang3(pAuthorityPlayerEnt->GetWorldRotation())
            );
    }
}

IEntity* CTOSEntitySpawnModule::SpawnAndInit(IEntitySystem* pEntitySystem, STOSEntitySpawnParams& params)
{
    if (!pEntitySystem)
        return nullptr;

    if (!params.name.empty())
        params.vanilla.sName = params.name.c_str();

    auto pArchetype = pEntitySystem->LoadEntityArchetype(params.archetypeName);
    if (pArchetype)
        params.vanilla.pArchetype = pArchetype;

    IEntity* pSpawned = pEntitySystem->SpawnEntity(params.vanilla, false);

    if (pArchetype)
        tos::script::SetEntityValue(pSpawned, "Properties", pArchetype->GetProperties());;

    auto& props = params.properties;
    auto& propsInstance = params.propertiesInstance;

    if (props.GetPtr()) tos::script::SetEntityValue(pSpawned, "Properties", props);
    if (propsInstance.GetPtr()) tos::script::SetEntityValue(pSpawned, "PropertiesInstance", propsInstance);

    gEnv->pEntitySystem->InitEntity(pSpawned, params.vanilla);

    CryLog("[CTOSEntitySpawnModule::SpawnAndInit] Spawned: %s, ArchetypeName: %s, ArchetypeVanillaName: %s", 
        pSpawned->GetName(), 
        params.archetypeName.c_str(),
        params.vanilla.pArchetype ? params.vanilla.pArchetype->GetName() : "null");

    return pSpawned;
}

bool CTOSEntitySpawnModule::AuthorityAlreadyHasSlave(const STOSEntitySpawnParams& params) const
{
    if (!params.saveParams || params.authorityName.empty())
        return false;

    for (auto& pair : m_savedSpawnParams)
    {
        const auto id = pair.first;
        const auto& saved = pair.second;

        if (saved->authorityName == params.authorityName)
        {
            if (gEnv->pSystem->IsDevMode())
            {
                CryLogAlways(
                    "%s[CTOSEntitySpawnModule::AuthorityAlreadyHasSlave] Warning!!! "
                    "The system already has a saved slave(id:%i) for player %s",
                    TOS_COLOR_YELLOW, id, params.authorityName.c_str()
                );
            }
            return true;
        }
    }
    return false;
}

bool CTOSEntitySpawnModule::ShouldSpawnImmediately(const STOSEntityDelaySpawnParams& params) const
{
    return params.spawnDelay < 0.001f;
}

void CTOSEntitySpawnModule::ScheduleDelayedSpawn(const STOSEntityDelaySpawnParams& params)
{
    s_scheduledSpawnsDelay.push_back(new STOSEntityDelaySpawnParams(params));
}

//void CTOSEntitySpawnModule::ScheduleCallback(const std::function<void()>& callback)
//{
//    m_delayedCallbacks.push_back(callback);
//}

bool CTOSEntitySpawnModule::SpawnEntityDelay(STOSEntityDelaySpawnParams& params, bool sendTosEvent /*= true*/)
{
    CRY_ASSERT_MESSAGE(gEnv->bServer,
        "Entity spawning process only can be on the server");
    if (!gEnv->bServer)
        return false;

    auto pEntitySystem = gEnv->pEntitySystem;
    if (!pEntitySystem)
        return false;

    auto pSpawnModule = g_pTOSGame->GetEntitySpawnModule();
    if (!pSpawnModule)
        return false;

    // 1. Проверка: у игрока уже есть «раб»
    if (pSpawnModule->AuthorityAlreadyHasSlave(params))
        return false;

    // 2. Если задержка почти нулевая — спавним сразу
    if (pSpawnModule->ShouldSpawnImmediately(params))
    {
        IEntity* pEnt = SpawnEntity(params, sendTosEvent);
        if (params.callback)
        {
            params.callback(
                pEnt->GetId(),
                pEnt->GetWorldPos(),
                params.clientChannelId);

            //pSpawnModule->ScheduleCallback([=]()
            //    {
            //        params.callback(
            //            pEnt->GetId(), 
            //            pEnt->GetWorldPos(), 
            //            params.clientChannelId);
            //    });
        }

        return true;
    }

    // 3. Иначе — планируем спавн с задержкой
    pSpawnModule->ScheduleDelayedSpawn(params);
    return true;
}

void CTOSEntitySpawnModule::CleanupEntity(EntityId id)
{
    stl::find_and_erase(s_markedForRecreation, id);
    tos::stl::FindAndEraseMap(m_scheduledAuthorities, id);
    tos::stl::FindAndEraseMap(m_scheduledRecreations, id);
    tos::stl::FindAndEraseMap(m_savedSpawnParams, id);
    tos::stl::FindAndEraseMap(m_removeDelay, id);
}

void CTOSEntitySpawnModule::RemoveEntityForced(EntityId id)
{
    auto pSM = g_pTOSGame->GetEntitySpawnModule();
    assert(pSM);
    if (!pSM)
        return;

    pSM->CleanupEntity(id);

    TOS_RECORD_EVENT(id, STOSGameEvent(eEGE_EntityRemovedForced, "", true));

    gEnv->pEntitySystem->RemoveEntity(id);
}

void CTOSEntitySpawnModule::RemoveEntityDelayed(EntityId id, int framesCount)
{
    auto pEntity = TOS_GET_ENTITY(id);
    if (!pEntity)
        return;

    auto iter = m_removeDelay.find(id);
    if (iter == m_removeDelay.end())
        m_removeDelay[id] = SFrameTimer(framesCount + m_removeDelay.size());
}

bool CTOSEntitySpawnModule::MustBeRecreated(const IEntity* pEntity) const
{
    assert(pEntity);
    if (!pEntity)
        return false;

    auto entId = pEntity->GetId();
    auto result = stl::find(s_markedForRecreation, entId) && IsSpawned(pEntity);

    return result;
}

IEntity* CTOSEntitySpawnModule::GetSpawnedSlave(const char* authorityPlayerName) const
{
    TMapTOSParams::const_iterator it = m_savedSpawnParams.begin();
    TMapTOSParams::const_iterator end = m_savedSpawnParams.end();

    for (; it != end; ++it)
    {
        const bool find = it->second->authorityName == authorityPlayerName && it->second->forceStartControl;

        if (find)
            return TOS_GET_ENTITY(it->first);
    }

    return nullptr;
}

bool CTOSEntitySpawnModule::IsSpawned(const IEntity* pEntity) const
{
    assert(pEntity);
    if (!pEntity)
        return false;

    return m_savedSpawnParams.find(pEntity->GetId()) != m_savedSpawnParams.end();
}

void CTOSEntitySpawnModule::DebugDraw(const Vec2& screenPos, float fontSize, float interval, int maxElemNum, bool draw) const
{
    if (!draw)
        return;

    //Header
    DRAW_2D_TEXT(
        screenPos.x,
        screenPos.y - interval * 2,
        fontSize + 0.2f,
        "--- TOS Entity Spawn Module (savedName|realName) ---");

    //Body
    TMapTOSParams::const_iterator it = m_savedSpawnParams.begin();

    for (; it != m_savedSpawnParams.end(); ++it)
    {
        const EntityId id = it->first;
        const auto pEnt = gEnv->pEntitySystem->GetEntity(id);
        if (!pEnt)
            continue;

        const string& entName = pEnt->GetName();
        const string& savedName = it->second->vanilla.sName;

        const int index = tos::stl::GetIndexFromMapKey(m_savedSpawnParams, id) + 1;

        float color[] = { 1,1,1,1 };

        gEnv->pRenderer->Draw2dLabel(
            screenPos.x,
            screenPos.y + index * interval,
            fontSize,
            color,
            false,
            "%i) %s:%s",
            index, savedName.c_str(), entName.c_str());
    }
}

void CTOSEntitySpawnModule::ScheduleRecreation(const IEntity* pEntity)
{
    // До вызова этой функции дожно быть выполнено
    // 1) запись с параметрами спавна pEntity в m_savedParams
    // 2) entityId сущности pEntity должен быть в s_markedForRecreation
    // Или
    // 1) MustBeRecreated(pEntity) должен вернуть True

    CRY_ASSERT_MESSAGE(gEnv->bServer, "Entity scheduling process only can be on the server");
    if (!gEnv->bServer)
        return;

    assert(pEntity);
    if (!pEntity)
        return;

    const auto entId = pEntity->GetId();
    const bool alreadyScheduled = m_scheduledRecreations.find(entId) != m_scheduledRecreations.end();
    if (alreadyScheduled)
        return;

    _smart_ptr<STOSEntitySpawnParams> pParams(new STOSEntitySpawnParams());

    tos::script::GetEntityScriptValue(pEntity, "Properties", pParams->properties);
    tos::script::GetEntityScriptValue(pEntity, "PropertiesInstance", pParams->propertiesInstance);

    pParams->tosFlags |= ENTITY_RECREATION_SCHEDULED;
    pParams->vanilla = m_savedSpawnParams[entId]->vanilla;
    pParams->name = pEntity->GetName();
    pParams->authorityName = m_savedSpawnParams[entId]->authorityName;
    pParams->forceStartControl = m_savedSpawnParams[entId]->forceStartControl;

    m_scheduledRecreations[entId] = pParams;

    stl::find_and_erase(s_markedForRecreation, entId);
    tos::stl::FindAndEraseMap(m_savedSpawnParams, entId);
}
