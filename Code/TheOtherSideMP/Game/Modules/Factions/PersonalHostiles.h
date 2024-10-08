#pragma once

#include "TheOtherSideMP\Game\Events\ITOSGameEventListener.h"

/// @brief Класс, отвечающий за управление персональной враждебностью между сущностями.
class CTOSPersonalHostiles : public IEntityEventListener, ITOSGameEventListener
{
public:
	/// @brief Класс, отвечающий за управление персональной враждебностью между сущностями.
	CTOSPersonalHostiles(IEntitySystem* pEntSys);
	~CTOSPersonalHostiles();

	//IEntityEventListener
	virtual void OnEntityEvent( IEntity *pEntity,SEntityEvent &event ) override;
	//~IEntityEventListener

	//ITOSGameEventListener
	virtual void OnExtraGameplayEvent(IEntity* pEntity, const STOSGameEvent& event) override;
	//~ITOSGameEventListener

	/// @brief Делает две сущности враждебными друг другу.
	/// @param one Идентификатор первой сущности.
	/// @param two Идентификатор второй сущности.
	/// @return true, если сущности успешно сделаны враждебными, иначе false.
	bool MakeHostile(EntityId one, EntityId two);

	/// @brief Удаляет враждебность между двумя сущностями.
	/// @param one Идентификатор первой сущности.
	/// @param two Идентификатор второй сущности.
	/// @return true, если враждебность успешно удалена, иначе false.
	bool RemoveHostile(EntityId one, EntityId two);

	/// @brief Удаляет записи обо всех личных врагах сущности `one`.
	/// @param one Идентификатор сущности.
	/// @return true, если враждебность успешно удалена для всех сущностей, иначе false.
	bool RemoveHostileAll(EntityId one);

	/// @brief Проверяет, имеет ли сущность врагов.
	/// @param one Идентификатор сущности.
	/// @return true, если сущность имеет врагов, иначе false.
	bool IsHaveHostiles(EntityId one);

	/// @brief Проверяет, являются ли две сущности личными врагами.
	/// @param one Идентификатор первой сущности.
	/// @param two Идентификатор второй сущности.
	/// @return true, если сущности являются врагами, иначе false.
	bool IsHostile(EntityId one, EntityId two);

private:

	typedef std::map<EntityId, std::set<EntityId>> Hostiles;
	Hostiles m_hostiles;
	IEntitySystem* m_pEntitySystem;
};