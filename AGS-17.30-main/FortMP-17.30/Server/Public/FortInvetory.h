
#pragma once
#include "Framework.h"
class FortInventory {
private:
	FortInventory() = default;
public:
	static FFortItemEntry* FindItemEntry(AFortPlayerController* PlayerController, UFortItemDefinition* ItemDefinition);
	static FFortItemEntry* FindItemEntry(AFortPlayerController* PlayerController, FGuid* Guid);
	static void GiveItem(AFortPlayerController* PlayerController, UFortItemDefinition* ItemDefintion, int Count = 1, int LoadedAmmo = 0, bool bStack = false, bool bShowToast = true);
	static void RemoveItem(AFortPlayerController* PlayerController, FGuid Guid, int Count);
	static void RemoveAllDroppableItems(AFortPlayerControllerAthena* PlayerController);
	static FFortItemEntry* FindEntry22(AFortPlayerController* PC, FGuid Guid);
	static FFortItemEntry* FindEntry22(AFortPlayerController* PC, UFortItemDefinition* Def);
	static void UpdateInventory(AFortPlayerController* PC, FFortItemEntry& Entry);
	static void GiveItemStack(AFortPlayerController* PC, UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 0);
	static void RemoveItem22(AFortPlayerController* PC, UFortItemDefinition* Def, int Count);

};
