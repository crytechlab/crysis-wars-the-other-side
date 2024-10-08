/*************************************************************************
AlienKeeper Source File.
Copyright (C), AlienKeeper, 2024.
**************************************************************************/

#pragma once
#include "Actor.h"
#include "WeaponSystem.h"

namespace TOS_Inventory
{
	inline bool IsHaveWeapons(const CActor* pActor)
	{
		const auto pInventory = pActor->GetInventory();
		if (pInventory)
		{
			return pInventory->GetCount() > 0;
		}

		return false;
	};

	inline bool SelectPrimary(const IActor* pActor)
	{
		if (!pActor)
			return false;

		return Script::CallMethod(pActor->GetEntity()->GetScriptTable(), "SelectPrimaryWeapon");
	};

	inline bool SelectItemByClass(IActor* pActor, const string& classname)
	{
		if (!pActor)
			return false;

		g_pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(pActor, classname);
		return true;
	};

	inline bool SelectSecondary(const IActor* pActor)
	{
		if (!pActor)
			return false;

		return Script::CallMethod(pActor->GetEntity()->GetScriptTable(), "SelectSecondaryWeapon");
	};

	// TODO: Закэшировать паки
	inline void GiveEquipmentPack(IActor* pActor, const string& name, const bool resetInventory = false)
	{
		if (!pActor || name.empty())
			return;

		if (resetInventory)
		{
			IInventory* pInventory = pActor->GetInventory();
			if (pInventory)
			{
				pInventory->HolsterItem(true); // не проверено (не уверен)
				pInventory->RemoveAllItems(); // ок
				pInventory->Clear(); // не проверено
			}
		}

		const string filePath = "Game/Libs/EquipmentPacks/" + name + ".xml";
		//CryLogAlways("[EquipManager][GiveEquipmentPack][filePath] %s", filePath);

		const XmlNodeRef rootNode = GetISystem()->LoadXmlFile(filePath.c_str());
		if (!rootNode)
			return;

		//CryLogAlways("[EquipManager][GiveEquipmentPack] XML File Loaded!");

		XmlString primaryItemName;
		rootNode->getAttr("primary", primaryItemName);

		//CryLogAlways("[EquipManager][GiveEquipmentPack][rootNode][primaryParam] %s", primaryParam);

		const XmlNodeRef itemsNode = rootNode->findChild("Items");
		if (itemsNode)
		{
			//CryLogAlways("[EquipManager][GiveEquipmentPack] Items Node Founded!");
			//CryLogAlways("[EquipManager][GiveEquipmentPack][itemsNode][getChildCount] %i", ItemsNode->getChildCount());

			for (int i = 0; i < itemsNode->getChildCount(); ++i)
			{
				XmlNodeRef weaponNode = itemsNode->getChild(i);
				if (weaponNode)
				{
					XmlString type;
					weaponNode->getAttr("type", type);

					string weaponName = weaponNode->getTag();

					if (weaponName != primaryItemName)
						g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, weaponName, false, false, false);
					//	selectWeapon = true;


					//CryLogAlways("[EquipManager][GiveEquipmentPack][weaponNode][getTag] %s", weaponNode->getTag());
					//CryLogAlways("[EquipManager][GiveEquipmentPack][weaponNode][type] %s", type);

				}
			}

			g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, primaryItemName, false, true, false);
		}
	};

	inline EntityId GiveItem(IActor* pActor, string weaponName, bool sound, bool select, bool keephistory)
	{
		return g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, weaponName, sound, select, keephistory);
	}
}