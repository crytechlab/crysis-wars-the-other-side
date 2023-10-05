// ReSharper disable CppClangTidyCppcoreguidelinesSpecialMemberFunctions
// ReSharper disable CppInconsistentNaming
#pragma once

#include "../GenericModule.h"
#include "IEntity.h"

#include "TheOtherSideMP/TOSSmartStruct.h"

enum ETOSEntityFlags
{
	TOS_ENTITY_FLAG_MUST_RECREATED = (1 << 0),
	TOS_ENTITY_FLAG_SCHEDULED_RECREATION = (1 << 1),
	//TOS_ENTITY_FLAG_ALREADY_RECREATED = (1 << 2),
};

struct STOSScheduleDelegateAuthorityParams
{
	STOSScheduleDelegateAuthorityParams()
		: willBeSlave(false),
		scheduledTimeStamp(0) {}

	bool willBeSlave;
	string playerName; // имя игрока, который получит власть
	float scheduledTimeStamp; // штамп времени, когда свершилось планирование
};

struct STOSEntitySpawnParams : public STOSSmartStruct
{
	STOSEntitySpawnParams()
		: pSavedScript(nullptr),
		willBeSlave(false),
		tosFlags(0)
	{
		m_refs = 0;
		vanilla = SEntitySpawnParams();
	}

	explicit STOSEntitySpawnParams(const SEntitySpawnParams& _vanillaParams)
		: pSavedScript(nullptr),
		willBeSlave(false),
		tosFlags(0)
	{
		m_refs = 0;
		vanilla = _vanillaParams;
	}

	explicit STOSEntitySpawnParams(const STOSEntitySpawnParams& params)
		: STOSSmartStruct(params)
	{
		this->m_refs = 0;
		this->tosFlags = params.tosFlags;
		this->vanilla = params.vanilla;
		this->pSavedScript = params.pSavedScript;
		this->authorityPlayerName = params.authorityPlayerName;
		this->savedName = params.savedName;
		this->willBeSlave = params.willBeSlave;
	}

	~STOSEntitySpawnParams() override { }

	IScriptTable* pSavedScript;
	SEntitySpawnParams vanilla; 

	bool willBeSlave; ///< Если \a true, то при передаче власти игроку генерирует событие \a eEGE_SlaveReadyToObey \n Используется вместе с \a authorityPlayerName
	string authorityPlayerName; ///< Имя персонажа игрока, которому будет передана власть над сущностью после её пересоздания 
	string savedName; ///< Имя сущности, которая будет спавнится
	uint32 tosFlags; ///< Флаги появления сущности. \n Смотреть \a ETOSEntityFlags

private:
};

struct STOSEntityDelaySpawnParams : public STOSEntitySpawnParams
{
	STOSEntityDelaySpawnParams()
		: scheduledTimeStamp(0),
		spawnDelay(0) { }

	explicit STOSEntityDelaySpawnParams(const SEntitySpawnParams& _vanillaParams)
		: STOSEntitySpawnParams(_vanillaParams),
		scheduledTimeStamp(0),
		spawnDelay(0) { }

	explicit STOSEntityDelaySpawnParams(const STOSEntitySpawnParams& params)
		: STOSEntitySpawnParams(params),
		scheduledTimeStamp(0),
		spawnDelay(0) { }

	float scheduledTimeStamp; ///< Штамп времени, когда свершилось планирование
	float spawnDelay; ///< Задержка перед спавном. \n Если значение > 0.0f, то использовать функцию \a SpawnDelay(), иначе задержки не будет */
};


typedef std::vector<EntityId>									TVecEntities;
// typedef std::vector<_smart_ptr<STOSEntitySpawnParams>>          TVecTOSParams;
typedef std::map<EntityId, STOSScheduleDelegateAuthorityParams> TMapAuthorityParams;
typedef std::map<EntityId, _smart_ptr<STOSEntitySpawnParams>>   TMapTOSParams;
typedef std::map<int, _smart_ptr<STOSEntityDelaySpawnParams>>        TMapDelayTOSParams;

/**
 * \brief Модуль создания сущностей, используемых в моде The Other Side
 * \note Также модуль предназначен для пересоздания сущностей, удаленных во время работы консольной команды sv_restart.
 */
class CTOSEntitySpawnModule final :
	public CTOSGenericModule
{
public:
	CTOSEntitySpawnModule();
	~CTOSEntitySpawnModule() override;

	//ITOSGameModule
	void OnExtraGameplayEvent(IEntity* pEntity, const STOSGameEvent& event) override;
	const char* GetName() override { return "CTOSEntitySpawnModule"; };
	void Init() override;
	void Update(float frametime) override;
	void Serialize(TSerialize ser) override;
	void InitCVars(IConsole* pConsole) override;
	void InitCCommands(IConsole* pConsole) override;
	void ReleaseCVars() override;
	void ReleaseCCommands() override;
	//~ITOSGameModule

	//Console command's functions
	static void CmdSpawnEntity(IConsoleCmdArgs* pArgs);
	static void CmdRemoveEntityById(IConsoleCmdArgs* pArgs);
	static void CmdRemoveEntityByIdForced(IConsoleCmdArgs* pArgs);

	/**
	 * \brief Создаёт сущность по определенным параметрам
	 * \param params - параметры создания сущности
	 * \param sendTosEvent - если True, то при спавне произойдет отправка события eEGE_TOSEntityOnSpawn
	 * \return Если успешно, то указатель IEntity* на созданную сущность, иначе nullptr
	 */
	static IEntity* SpawnEntity(STOSEntitySpawnParams& params, bool sendTosEvent = true);

	/**
	 * \brief Создаёт сущность по определенным параметрам с некоторой задержкой
	 * \param params - параметры создания сущности. Задержка прописана внутри параметров.
	 * \param sendTosEvent - если True, то при спавне произойдет отправка события eEGE_TOSEntityOnSpawn
	 * \return Если успешно, то True
	 */
	static bool SpawnEntityDelay(STOSEntityDelaySpawnParams& params, bool sendTosEvent = true);

	/**
	 * \brief Удаляет сущность и очищает её записи, несмотря на флаг пересоздания \n TOS_ENTITY_FLAG_MUST_RECREATED в ETOSEntityFlags
	 * \param id - идентификатор сущности, которую нужно удалить
	 */
	static void RemoveEntityForced(EntityId id);

	/**
	 * \brief Проверяет, должна ли быть воссоздана сущность после sv_restart
	 * \param pEntity - указатель на проверяемую сущность
	 * \return Если сущность должна быть воссоздана после sv_restart, то True, иначе False
	 */
	bool MustBeRecreated(const IEntity* pEntity) const;

private:

	/**
	 * \brief Запланировать пересоздание сущности после sv_restart 
	 * \param pEntity - указатель на сущность, которую нужно пересоздать
	 */
	void ScheduleRecreation(const IEntity* pEntity);

	/**
	 * \brief Проверяет, спавнилась ли сущность через \a EntitySpawnModule::SpawnEntity()
	 * \param pEntity - указатель на сущность, у которого проверяется наличие сохраненных параметров
	 * \return Если сущность pEntity когда либо спавнилась через \a EntitySpawnModule::SpawnEntity(), то вернёт True
	 */
	bool HaveSavedParams(const IEntity* pEntity) const;

	void DebugDraw(const Vec2& screenPos, float fontSize, float interval, int maxElemNum, bool draw) const;

	static TVecEntities s_markedForRecreation; ///< \b Что \b хранит: \n сущности которые должны быть пересозданы после sv_restart
	static TMapDelayTOSParams s_scheduledSpawnsDelay;
	TMapAuthorityParams m_scheduledAuthorities; ///< \b Что \b хранит: \n ключ - id сущности, \n значение - структура, где есть имя игрока, который получит власть над сущностью и штамп времени, когда случилось планирование
	TMapTOSParams m_scheduledRecreations; 
	TMapTOSParams m_savedParams; ///< \b Что \b хранит: \n ключ - id сущности, которая была заспавнена в этом модуле, \n значение - её \a STOSEntitySpawnParams, захваченные при спавне в этом модуле
};