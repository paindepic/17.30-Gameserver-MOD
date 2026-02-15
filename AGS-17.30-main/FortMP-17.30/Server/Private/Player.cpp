#include "../Public/Player.h"

#include "../Public/Offsets.h"

#include "../Public/Abilities.h"
#include "../Public/Misc.h"

#include "../SDK/SDK/FortniteGame_parameters.hpp"

#include "../Public/FortInvetory.h"

#include "../Public/XP.h"

#include "../public/Vehicles.h"

#include <cstdlib>
#include <ctime>



inline UFortAthenaAISpawnerDataComponentList* GlobalList = nullptr;
void Player::ServerAcknowledgePossessionHook(AFortPlayerControllerAthena* PlayerController, APawn* P)
{
	if (!PlayerController || !PlayerController->MyFortPawn)
		return;

	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState);
	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode);

	if (!GameState || !GameMode)
		return;

	for (auto& StartingItem : GameMode->StartingItems)
	{
		FortInventory::GiveItem(PlayerController, StartingItem.Item, StartingItem.Count, 0, false);
	}
	FortInventory::GiveItem(PlayerController, PlayerController->CosmeticLoadoutPC.Pickaxe->WeaponDefinition, 1, 0, false);


	static bool bFirst = false;
	if (!bFirst)
	{
		GameState->WarmupCountdownEndTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 60;
		GameMode->WarmupCountdownDuration = 60;
		GameState->WarmupCountdownStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		GameMode->WarmupCountdownDuration = 60;

		bFirst = true;
	}

	PlayerController->AcknowledgedPawn = P;
}

void Player::ServerAttemptAircraftJumpHook(const UFortControllerComponent_Aircraft* ControllerComponent,
	const FRotator& ClientRotation)
{
	if (!ControllerComponent)
		return;
	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)ControllerComponent->GetOwner();
	AFortPlayerController* PlayerController = Cast<AFortPlayerController>(ControllerComponent->GetOwner());
	if (!PlayerController || !PlayerController->IsInAircraft()) return;

	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode);
	if (!GameMode) return;

	GameMode->RestartPlayer(PlayerController);
	PlayerController->ClientSetRotation(ClientRotation, true);

	if (PC->MyFortPawn)
	{
		PC->MyFortPawn->BeginSkydiving(true);
		if (bLategame)
		{
			PC->MyFortPawn->SetHealth(100);
			PC->MyFortPawn->SetShield(100);
		}
	}

}

//void Player::ServerLoadingScreenDroppedHook(const AFortPlayerControllerAthena* PlayerController)
//{
//	if (!PlayerController || !PlayerController->PlayerState)
//		return;
//
//	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);
//	if (!PlayerState) return;
//
//    AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState);
//	if (!GameState) return;
//
//	if (PlayerState->AbilitySystemComponent)
//		Abilities::GiveDefaultAbilitySet(PlayerState->AbilitySystemComponent);
//	else
//		LOG("PlayerState->AbilitySystemComponent is null!");
//	
//	UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
//
//	return ServerLoadingScreenDropped(PlayerController);
//}

inline void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);
inline void Player::ServerLoadingScreenDroppedHook(AFortPlayerControllerAthena* PC)
{
	auto Pawn = (AFortPlayerPawn*)PC->Pawn;
	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

	/*if (EnableTeams)
	{
		PlayerState->SquadId = PlayerState->TeamIndex - 2;

		PlayerState->OnRep_SquadId();
		PlayerState->OnRep_TeamIndex(0);
		PlayerState->OnRep_PlayerTeam();
		PlayerState->OnRep_PlayerTeamPrivate();

		FGameMemberInfo Info{ -1,-1,-1 };
		Info.MemberUniqueId = PlayerState->UniqueId;
		Info.SquadId = PlayerState->SquadId;
		Info.TeamIndex = PlayerState->TeamIndex;

		GameState->GameMemberInfoArray.Members.Add(Info);
		GameState->GameMemberInfoArray.MarkItemDirty(Info);
	}*/
	PC->XPComponent->bRegisteredWithQuestManager = true;
	PC->XPComponent->OnRep_bRegisteredWithQuestManager();
	PlayerState->SeasonLevelUIDisplay = PC->XPComponent->CurrentLevel;
	PlayerState->OnRep_SeasonLevelUIDisplay();

	auto test = (UFortControllerComponent_InventoryService*)PC->GetComponentByClass(UFortControllerComponent_InventoryService::StaticClass());

	if (test)
	{
		static auto GoldDef = test->GetDefaultGlobalCurrencyItemDefinition();
		FortInventory::GiveItem(PC, GoldDef, test->GlobalCurrencyData.Currency.Count);
	}

	((AFortPlayerControllerAthena*)PC)->XPComponent->bRegisteredWithQuestManager = true;
	((AFortPlayerControllerAthena*)PC)->XPComponent->OnRep_bRegisteredWithQuestManager();
	PC->XPComponent->bRegisteredWithQuestManager = true;
	PC->XPComponent->OnRep_bRegisteredWithQuestManager();
	PlayerState->SeasonLevelUIDisplay = ((AFortPlayerControllerAthena*)PC)->XPComponent->CurrentLevel;
	PlayerState->OnRep_SeasonLevelUIDisplay();

	auto QuestManager = PC->GetQuestManager(ESubGame::Athena);
	if (!QuestManager)
		return;
	QuestManager->InitializeQuestAbilities(PC->Pawn);//doesnt do anything such a W
	for (auto Quest : QuestManager->CurrentQuests)
	{
		FString AssetPathNameWStr = UKismetStringLibrary::Conv_NameToString(Quest->GetQuestDefinitionBP()->QuestAbilitySet.ObjectID.AssetPathName);
		UFortAbilitySet* QuestSet = Utils::StaticLoadObject<UFortAbilitySet>(AssetPathNameWStr.ToString());
		AssetPathNameWStr.Free();
		if (!QuestSet)
			continue;
		FFortAbilitySetHandle Real{};
		for (auto Ability2 : QuestSet->GameplayAbilities)
		{

		}
		TWeakObjectPtr<UAbilitySystemComponent> Bruh;
		Bruh.ObjectIndex = PC->MyFortPawn->AbilitySystemComponent->Index;
		Bruh.ObjectSerialNumber = UObject::GObjects->GetSerialByIndex(Bruh.ObjectIndex);
		Real.TargetAbilitySystemComponent = Bruh;
	}

	for (auto Quest : QuestManager->CurrentQuests)
	{
		for (auto Obj : Quest->Objectives)
		{
			Obj->QuestOwner = PlayerState;
		}
		Quest->bHasRegisteredWithQuestManager = true;
	}

	if (PlayerState->AbilitySystemComponent)
	{
		Abilities::GiveDefaultAbilitySet(PlayerState->AbilitySystemComponent);
	}
	else
	{
		LOG("PlayerState->AbilitySystemComponent is null!");
	}


	if (bLategame)
	{
		std::srand(static_cast<unsigned int>(std::time(nullptr)));

		std::vector<UFortItemDefinition*> Rifles = {
			Utils::StaticFindObject<UFortItemDefinition>("/MotherGameplay/Items/Exotics/PastaRipper/WID_Assault_PastaRipper_Athena_Boss.WID_Assault_PastaRipper_Athena_Boss"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03")
		};

		std::vector<UFortItemDefinition*> Shotguns = {
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Charge_Athena_SR_Ore_T03.WID_Shotgun_Charge_Athena_SR_Ore_T03"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Charge_Athena_VR_Ore_T03.WID_Shotgun_Charge_Athena_VR_Ore_T03"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Swing_Athena_SR.WID_Shotgun_Swing_Athena_SR")
		};

		std::vector<UFortItemDefinition*> SniperRifles = {
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/LTM/WID_Sniper_NoScope_Athena_SR_Ore_T03.WID_Sniper_NoScope_Athena_SR_Ore_T03"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Sniper_Heavy_Athena_SR_Ore_T03.WID_Sniper_Heavy_Athena_SR_Ore_T03"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03")
		};

		std::vector<UFortItemDefinition*> Movility = {
		//	Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/RiftItem/Athena_Rift_Item.Athena_Rift_Item"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Launcher_Shockwave_Athena_UR_Ore_T03.WID_Launcher_Shockwave_Athena_UR_Ore_T03"),
			//Utils::StaticFindObject<UFortItemDefinition>(""),
			//Utils::StaticFindObject<UFortItemDefinition>("")
		};

		std::vector<UFortItemDefinition*> Consumables = {
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Flopper/Effective/WID_Athena_Flopper_Effective.WID_Athena_Flopper_Effective"),
			Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall"),
		//	Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/WID_Athena_Medkit.WID_Athena_Medkit")
		};

		UFortItemDefinition* Wood = Utils::StaticFindObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
		UFortItemDefinition* Stone = Utils::StaticFindObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
		UFortItemDefinition* Metal = Utils::StaticFindObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

		UFortItemDefinition* Shells = Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells");
		UFortItemDefinition* Medium = Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium");
		UFortItemDefinition* Light = Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight");
		UFortItemDefinition* Heavy = Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy");
		UFortItemDefinition* RocketAmmo = Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets");
		UFortItemDefinition* RocketAmmo2 = Utils::StaticFindObject<UFortItemDefinition>("/Game/Items/Ammo/AmmoDataExplosive.AmmoDataExplosive");

		UFortItemDefinition* ChosenRifle = Rifles[std::rand() % Rifles.size()];
		UFortItemDefinition* ChosenShotgun = Shotguns[std::rand() % Shotguns.size()];
		UFortItemDefinition* ChosenConsumable = Consumables[std::rand() % Consumables.size()];
		UFortItemDefinition* ChosenMovility = Movility[std::rand() % Movility.size()];
		UFortItemDefinition* ChosenSniperRifles = SniperRifles[std::rand() % SniperRifles.size()];

		FortInventory::GiveItem(PC, ChosenRifle, 1, 30, false);
		FortInventory::GiveItem(PC, ChosenShotgun, 1, 5, false);
		FortInventory::GiveItem(PC, Wood, 800, 0, true);
		FortInventory::GiveItem(PC, Stone, 750, 0, true);
		FortInventory::GiveItem(PC, Metal, 730, 0, true);
		FortInventory::GiveItem(PC, Shells, 70, 0, true);
		FortInventory::GiveItem(PC, Medium, 300, 0, true);
		FortInventory::GiveItem(PC, Light, 300, 0, true);
		FortInventory::GiveItem(PC, Heavy, 35, 0, true);
		FortInventory::GiveItem(PC, RocketAmmo, 35, 0, true);
		FortInventory::GiveItem(PC, ChosenConsumable, 3, 0, true);
		FortInventory::GiveItem(PC, ChosenMovility, 1, 6, true);
		FortInventory::GiveItem(PC, ChosenSniperRifles, 1, 1, true);
	}




	UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
	PlayerState->OnRep_CharacterData();

	PlayerState->ForceNetUpdate();
	Pawn->ForceNetUpdate();
	PC->ForceNetUpdate();

	return ServerLoadingScreenDroppedOG(PC);
}

void Player::ServerExecuteInventoryItem(AFortPlayerControllerAthena* PlayerController, FGuid& ItemGuid) {

	if (!PlayerController || !PlayerController->MyFortPawn || PlayerController->IsInAircraft()) return;

	UFortWeaponItemDefinition* ItemDefinition = nullptr;

	for (auto ItemEntry : PlayerController->WorldInventory->Inventory.ReplicatedEntries) {
		if (ItemEntry.ItemGuid == ItemGuid) {

			if (ItemEntry.ItemDefinition->IsA(UFortWeaponItemDefinition::StaticClass())) {
				ItemDefinition = Cast<UFortWeaponItemDefinition>(ItemEntry.ItemDefinition);
				break;
			}
			else if (ItemEntry.ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass())) {
				return;
			}
			else {
				return;
			}
		}
	}
	if (ItemDefinition != nullptr)
		PlayerController->MyFortPawn->EquipWeaponDefinition(ItemDefinition, ItemGuid, FGuid(), false);
}


__int64 Player::OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext)
{
	if (!Actor || !Actor->IsA(ABuildingSMActor::StaticClass()) || !InstigatedBy || !InstigatedBy->IsA(AFortPlayerControllerAthena::StaticClass()) || !DamageCauser->IsA(AFortWeapon::StaticClass()) || !((AFortWeapon*)DamageCauser)->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || Actor->bPlayerPlaced)
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto Def = UFortKismetLibrary::K2_GetResourceItemDefinition(Actor->ResourceType);

	if (Def)
	{
		auto& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;
		if (!BuildingResourceAmountOverride.RowName.ComparisonIndex)
			return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		FString CurveTableAssetPath = UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->ResourceRates.ObjectID.AssetPathName);
		static auto CurveTable = Utils::StaticLoadObject<UCurveTable>(CurveTableAssetPath.ToString());
		CurveTableAssetPath.Free();

		if (!CurveTable)
			CurveTable = Utils::StaticLoadObject<UCurveTable>("/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates");

		float Average = 1;
		EEvaluateCurveTableResult OutCurveTable;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(CurveTable, BuildingResourceAmountOverride.RowName, 0.f, &OutCurveTable, &Average, FString());

		float FinalResourceCount = round(Average / (Actor->GetMaxHealth() / Damage));

		if (FinalResourceCount > 0)
		{
			InstigatedBy->ClientReportDamagedResourceBuilding(Actor, Actor->ResourceType, FinalResourceCount, false, Damage == 100.f);
			FortInventory::GiveItemStack(InstigatedBy, Def, FinalResourceCount, 0);

			static auto AccoladeDef = Utils::StaticLoadObject<UFortAccoladeItemDefinition>("/BattlePassPermanentQuests/Items/Accolades/AccoladeId_066_WeakSpotsInARow.AccoladeId_066_WeakSpotsInARow");

			if (AccoladeDef)
			{
				static std::unordered_map<AFortPlayerControllerAthena*, int> WeakspotCounter;

				WeakspotCounter[InstigatedBy]++;

				if (WeakspotCounter[InstigatedBy] >= 5)// give xp only when 5 weakspots Andreu ur so proper trust my ass!
				{
					XP::Accolades::GiveAccolade(InstigatedBy, AccoladeDef, nullptr, EXPEventPriorityType::Normal);
					WeakspotCounter[InstigatedBy] = 0;
					printf("¡XP 5 weakspots!");
				}
			}
		}
	}

	return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}


void Player::ServerCreateBuildingActor(AFortPlayerControllerAthena* PlayerController, FCreateBuildingActorData& CreateBuildingData) {
	if (!PlayerController || PlayerController->IsInAircraft())
		return;

	auto BuildingClass = PlayerController->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();
	if (!BuildingClass) { std::cout << "BuildingClass is null" << std::endl; return; }

	TArray<ABuildingSMActor*> ExistingBuildings;
	char BuildRestrictionFlag;
	if (CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &ExistingBuildings, &BuildRestrictionFlag)) { return; }

	auto NewBuilding = Utils::SpawnActor<ABuildingSMActor>(CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, BuildingClass);
	if (!NewBuilding) { std::cout << "Failed to spawn NewBuilding" << std::endl; return; }

	NewBuilding->bPlayerPlaced = true;
	auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);
	NewBuilding->TeamIndex = PlayerState->TeamIndex;
	NewBuilding->Team = static_cast<EFortTeam>(PlayerState->TeamIndex);
	NewBuilding->OnRep_Team();
	NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, PlayerController, true, nullptr);

	for (auto& Building : ExistingBuildings) {
		Building->K2_DestroyActor();
	}
	ExistingBuildings.Free();

	auto ItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(NewBuilding->ResourceType);
	//FortInventory::RemoveItem(PlayerController, FortInventory::FindItemEntry(PlayerController, ItemDefinition)->ItemGuid, 10);

	FGameplayTagContainer Empty{};
	bool bor;
	XP::Challanges::SendStatEvent(PlayerController->GetQuestManager(ESubGame::Athena), NewBuilding, Empty, Empty, &bor, &bor, 1, EFortQuestObjectiveStatEvent::Build);
}
void Player::ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToEdit)
{
	if (!PlayerController || !BuildingActorToEdit || !PlayerController->MyFortPawn)
		return;

	auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);
	if (!PlayerState)
	{
		std::cout << "PlayerState is null" << std::endl;
		return;
	}

	auto EditToolItemDefinition = Utils::StaticFindObject<UFortItemDefinition>(
		"/Game/Items/Weapons/BuildingTools/EditTool.EditTool");

	if (!EditToolItemDefinition)
	{
		std::cout << "Failed to find EditToolItemDefinition" << std::endl;
		return;
	}

	auto CurrentWeapon = PlayerController->MyFortPawn->CurrentWeapon; // FUCK THIS SHIT FR WORKS AND FIXED ALL EDITING BUGS/DELAY!!!!!!!!!!!
	if (!CurrentWeapon || CurrentWeapon->WeaponData != EditToolItemDefinition)
	{
		if (auto ItemEntry = FortInventory::FindItemEntry(PlayerController, EditToolItemDefinition))
		{
			PlayerController->ServerExecuteInventoryItem(ItemEntry->ItemGuid);
		}
		else
		{
			std::cout << "EditTool item not found in inventory." << std::endl;
			return;
		}
		CurrentWeapon = PlayerController->MyFortPawn->CurrentWeapon;
	}

	auto EditTool = static_cast<AFortWeap_EditingTool*>(PlayerController->MyFortPawn->CurrentWeapon);
	if (!EditTool)
	{
		std::cout << "EditTool is null or cast failed." << std::endl;
		return;
	}

	//if (EditTool->EditActor == BuildingActorToEdit)
	//	return;

	EditTool->EditActor = BuildingActorToEdit;
	EditTool->OnRep_EditActor();

	BuildingActorToEdit->EditingPlayer = PlayerState;
	BuildingActorToEdit->OnRep_EditingPlayer();
}



void Player::ServerEditBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored) {
	if (!BuildingActorToEdit || !NewBuildingClass.Get() || BuildingActorToEdit->bDestroyed || BuildingActorToEdit->EditingPlayer != Cast<AFortPlayerStateAthena>(PlayerController->PlayerState))
		return;

	BuildingActorToEdit->EditingPlayer = nullptr;

	if (auto NewBuilding = ReplaceBuildingActor(BuildingActorToEdit, 1, NewBuildingClass.Get(), BuildingActorToEdit->CurrentBuildingLevel, RotationIterations, bMirrored, PlayerController)) {
		NewBuilding->bPlayerPlaced = true;
		NewBuilding->SetTeam(Cast<AFortPlayerStateAthena>(PlayerController->PlayerState)->TeamIndex);
		NewBuilding->OnRep_Team();
	}
}

void Player::ServerEndEditingBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToStopEditing) {
	if (!PlayerController || !BuildingActorToStopEditing || BuildingActorToStopEditing->EditingPlayer != Cast<AFortPlayerStateAthena>(PlayerController->PlayerState) || !PlayerController->MyFortPawn)
		return;

	BuildingActorToStopEditing->SetNetDormancy(ENetDormancy::DORM_DormantAll);
	BuildingActorToStopEditing->EditingPlayer = nullptr;

	auto EditTool = static_cast<AFortWeap_EditingTool*>(PlayerController->MyFortPawn->CurrentWeapon);
	if (!EditTool) { std::cout << "No EditTool" << std::endl; return; }

	EditTool->EditActor = nullptr;
	EditTool->OnRep_EditActor();
}

void Player::ServerSetInAircraft(AFortPlayerState* PlayerState, bool bNewInAircraft) {
	if (!PlayerState || bLategame == true)// on lategame we dont want to clean the inventory items ig?
		return;

	auto PlayerController = Cast<AFortPlayerControllerAthena>(PlayerState->GetOwner());
	if (!PlayerController)
		return;

	FortInventory::RemoveAllDroppableItems(PlayerController);

	return ServerSetInAircraftOG(PlayerState, bNewInAircraft);
}

void Player::ServerPlaySquadQuickChatMessage(AFortPlayerControllerAthena* PlayerController, FAthenaQuickChatActiveEntry ChatEntry, __int64)
{
	auto PlayerState = reinterpret_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState);

	switch (ChatEntry.Index)
	{
	case 0:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::ChatBubble;
		break;
	case 1:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::EnemySpotted;
		break;
	case 2:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedMaterials;
		break;
	case 3:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedBandages;
		break;
	case 4:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedShields;
		break;
	case 5:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedAmmoHeavy;
		break;
	case 6:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedAmmoLight;
		break;
	case 7:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::FIRST_CHAT_MESSAGE;
		break;
	case 8:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedAmmoMedium;
		break;
	case 9:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedAmmoShells;
		break;
	default:
		break;
	}

	PlayerState->OnRep_ReplicatedTeamMemberState();

	static auto EmojiComm = Utils::StaticFindObject<UAthenaEmojiItemDefinition>("/Game/Athena/Items/Cosmetics/Dances/Emoji/Emoji_Comm.Emoji_Comm");

	if (EmojiComm)
	{
		PlayerController->ServerPlayEmoteItem(EmojiComm, 0);
	}
}

void Player::ServerReviveFromDBNO(AFortPlayerPawnAthena* Pawn, AFortPlayerControllerAthena* Instigator)
{
	float ServerTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
	printf("ServerReviveFromDBNO called\n");
	if (!Pawn || !Instigator)
		return;

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PC || !PC->PlayerState)
		return;
	auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	auto AbilitySystemComp = (UFortAbilitySystemComponentAthena*)PlayerState->AbilitySystemComponent;

	//Pawn->ReviveFromDBNOTime = 30;
	//Pawn->ServerWorldTimeRevivalTime = 30;
	//Pawn->DBNORevivalStacking = 0;

	FGameplayEventData Data{};
	Data.EventTag = Pawn->EventReviveTag;
	Data.ContextHandle = PlayerState->AbilitySystemComponent->MakeEffectContext();
	Data.Instigator = Instigator;
	Data.Target = Pawn;
	Data.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Pawn);
	Data.TargetTags = Pawn->GameplayTags;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Pawn->EventReviveTag, Data);

	for (auto& Ability : AbilitySystemComp->ActivatableAbilities.Items)
	{
		if (Ability.Ability->Class == UGAB_AthenaDBNO_C::StaticClass())
		{
			printf("UGAB_AthenaDBNO_C\n");
			AbilitySystemComp->ServerCancelAbility(Ability.Handle, Ability.ActivationInfo);
			AbilitySystemComp->ServerEndAbility(Ability.Handle, Ability.ActivationInfo, Ability.ActivationInfo.PredictionKeyWhenActivated);
			AbilitySystemComp->ClientCancelAbility(Ability.Handle, Ability.ActivationInfo);
			AbilitySystemComp->ClientEndAbility(Ability.Handle, Ability.ActivationInfo);
			break;
		}
	}

	Pawn->bIsDBNO = false;
	Pawn->OnRep_IsDBNO();
	Pawn->SetHealth(30);
	PlayerState->DeathInfo = {};
	PlayerState->OnRep_DeathInfo();

	PC->ClientOnPawnRevived(Instigator);
}

EFortTeam Player::PickTeam()
{
	static int MaxTeamSize = 2;
	static int Team = 3;
	static int CurrentPlayers = 0;

	int Ret = Team;

	CurrentPlayers++;
	if (CurrentPlayers == MaxTeamSize)
	{
		CurrentPlayers = 0;
		Team++;
	}
	return EFortTeam(Ret);
}

static void ApplySiphonEffect()
{
	for (int i = 0; i < UWorld::GetWorld()->NetDriver->ClientConnections.Num(); i++)
	{
		auto PlayerState = (AFortPlayerState*)UWorld::GetWorld()->NetDriver->ClientConnections[i]->PlayerController->PlayerState;

		auto AbilitySystemComponent = PlayerState->AbilitySystemComponent;

		auto Handle = AbilitySystemComponent->MakeEffectContext();
		AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(FGameplayTag(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"GameplayCue.Shield.PotionConsumed")), FPredictionKey(), Handle);
		AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(FGameplayTag(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"GameplayCue.Shield.PotionConsumed")), FPredictionKey(), Handle);
	}
}

static void (*RemoveFromAlivePlayers)(UObject* GameMode, UObject* PlayerController, APlayerState* PlayerState, APawn* FinisherPawn, UFortWeaponItemDefinition* FinishingWeapon, uint8_t DeathCause, char a7) = decltype(RemoveFromAlivePlayers)(InSDKUtils::GetImageBase() + 0x478b2e0);
void Siphon(AFortPlayerControllerAthena* PC)
{
	if (!PC || !PC->MyFortPawn || !PC->WorldInventory || !bEnableSiphon)
		return;
	static auto Wood = Utils::StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
	static auto Stone = Utils::StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
	static auto Metal = Utils::StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

	FortInventory::GiveItem(PC, Wood, 50);
	FortInventory::GiveItem(PC, Stone, 50);
	FortInventory::GiveItem(PC, Metal, 50);

	ApplySiphonEffect();
}

void OnKilled(AFortPlayerControllerAthena* DeadPC, AFortPlayerControllerAthena* KillerPC, UFortWeaponItemDefinition* KillerWeapon)
{
	Siphon(KillerPC);
	bool bruh;
	FGameplayTagContainer Empty{};
	FGameplayTagContainer Empty2{};
	XP::Challanges::SendStatEvent(KillerPC->GetQuestManager(ESubGame::Athena), DeadPC, KillerWeapon ? KillerWeapon->GameplayTags : (KillerPC->MyFortPawn->CurrentWeapon ? KillerPC->MyFortPawn->CurrentWeapon->WeaponData->GameplayTags : Empty), Empty2, &bruh, &bruh, 1, EFortQuestObjectiveStatEvent::Kill);
	Empty.GameplayTags.Free();
	Empty.ParentTags.Free();
	Empty2.ParentTags.Free();
	Empty2.GameplayTags.Free();
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	for (auto PC : GameMode->AlivePlayers)
	{
		XP::Challanges::SendStatEvent(PC->GetQuestManager(ESubGame::Athena), nullptr, Empty, Empty2, &bruh, &bruh, 1, EFortQuestObjectiveStatEvent::AthenaOutlive);
	}

	auto KillerState = (AFortPlayerStateAthena*)KillerPC->PlayerState;
	if (!KillerState)
		return;
	auto DeadState = (AFortPlayerStateAthena*)DeadPC->PlayerState;
	if (!DeadState)
		return;

	static FGameplayTag EarnedElim = { UKismetStringLibrary::Conv_StringToName(TEXT("Event.EarnedElimination")) };
	FGameplayEventData Data{};
	Data.EventTag = EarnedElim;
	Data.ContextHandle = KillerState->AbilitySystemComponent->MakeEffectContext();
	Data.Instigator = KillerPC;
	Data.Target = DeadState;
	Data.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(DeadState);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(KillerPC->Pawn, Data.EventTag, Data);

	static auto BountyHunterClass = Utils::StaticLoadObject<UClass>("/Bounties/HuntingPlayer/Abilities/Hunter/GA_QuestBountyHunter.GA_QuestBountyHunter_C");
	static auto BountyTargetClass = Utils::StaticLoadObject<UClass>("/Bounties/HuntingPlayer/Abilities/Target/GA_QuestBountyTarget.GA_QuestBountyTarget_C");
	static auto TargetQuest = Utils::StaticLoadObject<UFortUrgentQuestItemDefinition>("/Bounties/HuntingPlayer/Quests/Items/quest_bounty_target_ind.quest_bounty_target_ind");
	static auto HunterQuest = Utils::StaticLoadObject<UFortUrgentQuestItemDefinition>("/Bounties/HuntingPlayer/Quests/Items/quest_bounty_hunter_ind.quest_bounty_hunter_ind");

	if (DeadPC->TransientQuestsComponent->ActiveUrgentQuests.Num() > 0)
	{
		for (auto& AbilitySpec : DeadState->AbilitySystemComponent->ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability->Class == BountyTargetClass)
			{
				((UFortPlayerBountyGameplayAbility*)AbilitySpec.ReplicatedInstances[0])->FailQuest();
				((UFortPlayerBountyGameplayAbility*)AbilitySpec.ReplicatedInstances[0])->StopUrgentQuestEvent(DeadPC->TransientQuestsComponent->ActiveUrgentQuests[0]);
				DeadState->AbilitySystemComponent->ClientEndAbility(AbilitySpec.Handle, AbilitySpec.ActivationInfo);
				DeadState->AbilitySystemComponent->ServerEndAbility(AbilitySpec.Handle, AbilitySpec.ActivationInfo, AbilitySpec.ActivationInfo.PredictionKeyWhenActivated);
				DeadPC->TransientQuestsComponent->StopPlayerBountyThreatLevelUpdates();
				DeadPC->TransientQuestsComponent->ClientRemoveThreatLevelBind();
				//DeadPC->TransientQuestsComponent->ClientBroadcastOnUrgentQuestEnded(DeadPC->TransientQuestsComponent->ActiveUrgentQuests[0].EventTag);
				DeadPC->TransientQuestsComponent->ClientRemoveTransientQuest(TargetQuest);
				DeadPC->TransientQuestsComponent->ActiveUrgentQuests.Remove(0);
				break;
			}
		}
	}

	if (KillerPC->TransientQuestsComponent->ActiveUrgentQuests.Num() > 0)
	{
		for (auto& AbilitySpec : KillerState->AbilitySystemComponent->ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability->Class == BountyHunterClass)
			{
				int Price = KillerPC->TransientQuestsComponent->TrackedHunterBountyTargetPrice;
				((UFortPlayerBountyGameplayAbility*)AbilitySpec.ReplicatedInstances[0])->CompleteQuest();
				((UFortPlayerBountyGameplayAbility*)AbilitySpec.ReplicatedInstances[0])->StopUrgentQuestEvent(KillerPC->TransientQuestsComponent->ActiveUrgentQuests[0]);
				KillerState->AbilitySystemComponent->ClientEndAbility(AbilitySpec.Handle, AbilitySpec.ActivationInfo);
				KillerState->AbilitySystemComponent->ServerEndAbility(AbilitySpec.Handle, AbilitySpec.ActivationInfo, AbilitySpec.ActivationInfo.PredictionKeyWhenActivated);
				//KillerPC->TransientQuestsComponent->ClientBroadcastOnUrgentQuestEnded(KillerPC->TransientQuestsComponent->ActiveUrgentQuests[0].EventTag);
				KillerPC->TransientQuestsComponent->ClientRemoveTransientQuest(HunterQuest);
				KillerPC->TransientQuestsComponent->ActiveUrgentQuests.Remove(0);
				break;
			}
		}
	}
}

inline static void (*RemoveFromAlivePlayerOG)(void*, void*, void*, void*, void*, EDeathCause, char) = decltype(RemoveFromAlivePlayerOG)(Utils::GetOffsetA(0x478b2e0));
void (*ClientOnPawnDiedOG)(AFortPlayerControllerZone* a1, FFortPlayerDeathReport a2);

void Player::ClientOnPawnDiedHook(AFortPlayerControllerZone* DeadPlayer, FFortPlayerDeathReport& DeathReport)
{
	auto DeadPawn = (AFortPlayerPawnAthena*)DeadPlayer->Pawn;
	auto DeadPlayerState = (AFortPlayerStateAthena*)DeadPlayer->PlayerState;
	auto KillerPlayerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
	auto KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;
	auto KillerPC = Cast<AFortPlayerControllerAthena>(DeathReport.KillerPawn ? DeathReport.KillerPawn->Controller : nullptr);
	auto DeadPC = Cast<AFortPlayerControllerAthena>(DeadPlayer); // Corrección aquí

	if (!DeadPawn || !DeadPlayerState)
		return ClientOnPawnDiedOG(DeadPlayer, DeathReport);

	EDeathCause DeathCause = DeadPlayerState->ToDeathCause(DeathReport.Tags, DeadPawn->bIsDBNO);
	FDeathInfo& DeathInfo = DeadPlayerState->DeathInfo;
	DeathInfo.bInitialized = true;
	DeathInfo.bDBNO = DeadPawn->bIsDBNO;
	DeathInfo.DeathCause = DeathCause;
	DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : DeadPlayerState;
	DeathInfo.Distance = DeathCause == EDeathCause::FallDamage ? DeadPawn->LastFallDistance : DeadPawn->GetDistanceTo(KillerPawn);
	DeathInfo.DeathLocation = DeadPawn ? DeadPawn->K2_GetActorLocation() : FVector{};

	DeadPlayerState->PawnDeathLocation = DeathInfo.DeathLocation;
	DeadPlayerState->OnRep_DeathInfo();

	if (KillerPlayerState && KillerPlayerState != DeadPlayerState)
	{
		KillerPlayerState->KillScore++;
		KillerPlayerState->TeamKillScore++;
		KillerPlayerState->ClientReportKill(DeadPlayerState);
		KillerPlayerState->OnRep_Kills();
		KillerPlayerState->OnRep_TeamScore();

		KillerPlayerState->Score++;
		KillerPlayerState->TeamScore++;
		KillerPlayerState->OnRep_Score();

		if (bEnableSiphon)
		{
			if (KillerPawn)
			{
				float Health = KillerPawn->GetHealth();
				float NewHealthAmount = Health + 50;

				KillerPawn->SetHealth(NewHealthAmount);

				if (NewHealthAmount > 100)
				{
					float ShieldToGive = (NewHealthAmount - 100) + KillerPawn->GetShield();

					KillerPawn->SetHealth(100);
					KillerPawn->SetShield(ShieldToGive);

					if (KillerPawn->GetShield() > 100)
					{
						KillerPawn->SetShield(100);
					}
				}

				if (DeadPC && KillerPC)
				{
					OnKilled(DeadPC, KillerPC, DeathReport.KillerWeapon);
				}
			}
		}
	}

	if (!Utils::GetGameState()->IsRespawningAllowed(DeadPlayerState))
	{
		if (!DeadPawn->IsDBNO())
		{
			if (DeadPlayer->WorldInventory)
			{
				for (int i = 0; i < DeadPlayer->WorldInventory->Inventory.ItemInstances.Num(); i++)
				{
					if (DeadPlayer->WorldInventory->Inventory.ItemInstances[i]->CanBeDropped())
					{
						auto UWU = DeadPlayer->WorldInventory->Inventory.ItemInstances[i]->ItemEntry;
						Utils::SpawnPickup(UWU.ItemDefinition, UWU.Count, UWU.LoadedAmmo, DeadPawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination);
					}
				}
			}

			UFortItemDefinition* WeaponDef = nullptr;
			auto DamageCauser = DeathReport.DamageCauser;
			if (DamageCauser)
			{
				if (auto WEAPON = Cast<AFortWeapon>(DamageCauser))
				{
					WeaponDef = WEAPON->WeaponData;
				}
			}

			RemoveFromAlivePlayerOG(Utils::GetGameMode(), DeadPlayer, KillerPlayerState == DeadPlayerState ? nullptr : KillerPlayerState, KillerPawn, WeaponDef, DeathInfo.DeathCause, 0);
		}
	}

	return ClientOnPawnDiedOG(DeadPlayer, DeathReport);
}

void Player::OnReload(AFortWeapon* Weapon, int AmmoUsed)
{
	OnReloadOG(Weapon, AmmoUsed);

	if (!Weapon || !Weapon->WeaponData)
		return;
	auto WeaponData = Weapon->WeaponData->GetAmmoWorldItemDefinition_BP();
	AFortPlayerPawnAthena* Pawn = Cast<AFortPlayerPawnAthena>(Weapon->Owner);
	if (!Pawn)
		return;
	AFortPlayerControllerAthena* PC = Cast<AFortPlayerControllerAthena>(Pawn->Controller);
	if (!PC)
		return;

	FortInventory::RemoveItem22(PC, WeaponData, AmmoUsed);

	for (auto& Entry : PC->WorldInventory->Inventory.ReplicatedEntries)
	{
		if (Entry.ItemGuid == Weapon->ItemEntryGuid)
		{
			Entry.LoadedAmmo += AmmoUsed;
			PC->WorldInventory->Inventory.MarkItemDirty(Entry);
			FortInventory::UpdateInventory(PC, Entry);
			PC->WorldInventory->HandleInventoryLocalUpdate();
			break;
		}
	}
}

//void ServerSendZiplineStateHook(AFortPlayerPawn* Pawn, FZiplinePawnState InZiplineState) {
//	if (!InZiplineState.Zipline)
//		return;
//	//joel fix or gay
//	//static int MAX_ZIPLINE_DISTANCE = 50; // Seems way too high, i have to do some tests ingame!
//
//	//// Calculate the distance between the pawn and the InZipline State actor
//	//auto PawnLoc = Pawn->K2_GetActorLocation();
//
//	//// Let's just force the cast, what could possibly go wrong? xD
//	//AFortAthenaZipline* Zipline = (AFortAthenaZipline*)InZiplineState.Zipline;
//	//auto ZiplineStartLoc = Zipline->StartPosition;
//	//auto ZiplineEndPosition = Zipline->EndPosition;
//
//	//if (!InZiplineState.bIsZiplining) {
//	//    if (
//	//        // Start check
//	//        ZiplineStartLoc.GetDistanceToInMeters(PawnLoc) > MAX_ZIPLINE_DISTANCE &&
//	//        // End check
//	//        ZiplineEndPosition.GetDistanceToInMeters(PawnLoc) > MAX_ZIPLINE_DISTANCE
//	//    ) {
//	//        return; // Not allowed to zipline!
//	//    }
//	//} else {
//	//    // Well, uh in that case calculate how far he has come and uh yea then calculate it back to get that shit? :skull:
//	//    // (Easier, just check if he's on a straight line, uh, i should have been more active when we discussed vectors in school :skull:)
//	//    
//	//    // First check, Z
//	//    auto LowestPoint = min(ZiplineEndPosition.Z, ZiplineStartLoc.Z);
//	//    auto HighestPoint = max(ZiplineEndPosition.Z, ZiplineStartLoc.Z);
//
//	//    // First check, Z
//	//    // Example:
//	//        // Player is on Z 40, but Zipline low is on Z 200, 40 - 200 is -160 which is smaller than -50, so its getting triggered
//	//        // Player is on Z 500 (skybase?), but Zipline high is on Z 250, 500 - 200 is 300, which is higher than 50, so its getting triggered
//	//    if (PawnLoc.Z - LowestPoint < (-MAX_ZIPLINE_DISTANCE) || HighestPoint - PawnLoc.Z > MAX_ZIPLINE_DISTANCE) {
//	//        return; // Not allowed to zipline!
//	//    }
//
//	//    // X, Y to be done 
//	//}
//
//	Pawn->ZiplineState = InZiplineState;
//	static void(*OnRep_ZiplineState)(AFortPlayerPawn * Pawn) = decltype(OnRep_ZiplineState)(InSDKUtils::GetImageBase() + 0x4eb6dd0);
//	OnRep_ZiplineState(Pawn);
//	if (InZiplineState.bJumped)
//	{
//		//we love EndZiplining
//		EEvaluateCurveTableResult res;
//		float ZiplineJumpDampening = 0;
//		float ZiplineJumpStrength = 0;
//		UDataTableFunctionLibrary::EvaluateCurveTableRow(Pawn->ZiplineJumpDampening.CurveTable, Pawn->ZiplineJumpDampening.RowName, 0, &res, &ZiplineJumpDampening, FString());
//		UDataTableFunctionLibrary::EvaluateCurveTableRow(Pawn->ZiplineJumpStrength.CurveTable, Pawn->ZiplineJumpStrength.RowName, 0, &res, &ZiplineJumpStrength, FString());
//		FVector Velocity = Pawn->CharacterMovement->Velocity;
//		FVector LaunchVelocity = FVector{ -750, -750, ZiplineJumpStrength };
//
//		if (ZiplineJumpDampening * Velocity.X >= -750.f)
//		{
//			LaunchVelocity.X = fminf(ZiplineJumpDampening * Velocity.X, 750);
//		}
//		if (ZiplineJumpDampening * Velocity.Y >= -750.f)
//		{
//			LaunchVelocity.Y = fminf(ZiplineJumpDampening * Velocity.Y, 750);
//		}
//
//		Pawn->LaunchCharacter(LaunchVelocity, false, false);
//	}
//}

static void(*OnRep_ZiplineState)(AFortPlayerPawn*) = decltype(OnRep_ZiplineState)(InSDKUtils::GetImageBase() + 0x4eb6dd0);
void ServerSendZiplineStateHook(AFortPlayerPawn* Pawn, FZiplinePawnState& InZiplineState)
{
	if (Pawn)
	{
		Pawn->ZiplineState = InZiplineState;

		if (InZiplineState.bJumped)
		{
			Pawn->LaunchCharacter({ 0,0,1000 }, false, true);
		}

		OnRep_ZiplineState(Pawn);
	}
}

void Player::ServerAttemptInteractHook(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted)
{
	auto PC = ((AFortPlayerControllerAthena*)Comp->GetOwner());

	ServerAttemptInteractOG(Comp, ReceivingActor, InteractComponent, InteractType, OptionalData, InteractionBeingAttempted);

	bool bruh;

	FGameplayTagContainer TargetTags{};
	FGameplayTagContainer Empty{};

	if (auto Cont = Cast<ABuildingActor>(ReceivingActor))
		TargetTags = Cont->StaticGameplayTags;

	XP::Challanges::SendStatEvent(((AFortPlayerControllerAthena*)Comp->GetOwner())->GetQuestManager(ESubGame::Athena), ReceivingActor, /*((AFortPlayerControllerAthena*)Comp->GetOwner())->MyFortPawn->GameplayTags*/Empty, TargetTags, &bruh, &bruh, 1, EFortQuestObjectiveStatEvent::Interact);

	if (auto ConvComp = Cast<UFortNonPlayerConversationParticipantComponent>(ReceivingActor->GetComponentByClass(UFortNonPlayerConversationParticipantComponent::StaticClass())))
	{
		UFortPlayerConversationComponent_C* PlayerComp = (UFortPlayerConversationComponent_C*)Comp->GetOwner()->GetComponentByClass(UFortPlayerConversationComponent_C::StaticClass());
		if (!PlayerComp)
			PlayerComp = (UFortPlayerConversationComponent_C*)((AFortPlayerControllerAthena*)Comp->GetOwner())->Pawn->GetComponentByClass(UFortPlayerConversationComponent_C::StaticClass());

		if (ConvComp && PlayerComp && ConvComp->ConversationsActive == 0)
		{
			//AIs::StartConversation(ConvComp->ConversationEntryTag, PlayerComp->GetOwner(), ConvComp->InteractorParticipantTag, ReceivingActor, ConvComp->SelfParticipantTag);
		}
	}

	if (ReceivingActor->IsA(AFortAthenaSupplyDrop::StaticClass()) && !ReceivingActor->GetName().contains("Llama"))
	{
		printf("llama or supply drop");
	}
	else if (ReceivingActor->IsA(ABGA_Petrol_Pickup_C::StaticClass()))
	{
		auto petrolpikcup = (ABGA_Petrol_Pickup_C*)ReceivingActor;
		FortInventory::GiveItem(PC, petrolpikcup->WeaponItemDefinition, 1, 100);
	}
	else if (auto Vehicle = Cast<AFortAthenaVehicle>(ReceivingActor))
	{
		if (!Vehicle || !PC->MyFortPawn || !PC || !PC->MyFortPawn->IsInVehicle())
			return;
		Vehicles::GiveVehicleWeapon(PC, Vehicle);
	}
}

__int64 (*TestOG)(UObject* Ability, __int64* a2, char* a3);
__int64 Test(UGameplayAbility* Ability, __int64* a2, char* a3)
{
	if (Ability)
	{
		if (!XP::Challanges::SendDistanceUpdate(Ability))
			return 0;
	}
	return TestOG(Ability, a2, a3);
}

void Player::ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PC, FGuid ItemGuid, int Count, bool bTrash)
{
	if (Count < 1)
		return;
	if (!PC->WorldInventory)
		return;

	FFortItemEntry* Entry = FortInventory::FindEntry22(PC, ItemGuid);
	if (Entry->Count < Count)
		return;
	static auto petrolpickupclass = Utils::StaticLoadObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/Prototype/WID_Launcher_Petrol.WID_Launcher_Petrol");
	if (Entry->ItemDefinition == petrolpickupclass)
	{
		std::cout << Entry->ItemDefinition->GetFullName() << std::endl;
		FTransform Transform{};
		Transform.Translation = PC->Pawn->K2_GetActorLocation();
		Transform.Rotation = FQuat();
		Transform.Scale3D = FVector(1, 1, 1);
		UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), ABGA_Petrol_Pickup_C::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, nullptr), Transform);
		FortInventory::RemoveItem(PC, ItemGuid, Count);
	}
	else {

		Utils::SpawnPickup(PC->Pawn->K2_GetActorLocation(), Entry->ItemDefinition, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Count, Entry->LoadedAmmo);
		FortInventory::RemoveItem(PC, ItemGuid, Count);
	}
}

void Player::ServerHandlePickup(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo Info)
{
	if (!Pickup || !Pawn || !Pawn->Controller || Pickup->bPickedUp)
		return;

	Pickup->bPickedUp = true;

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;

	Pickup->PickupLocationData.bPlayPickupSound = Info.bPlayPickupSound;
	Pickup->PickupLocationData.FlyTime = 0.4f;
	Pickup->PickupLocationData.ItemOwner = Pawn;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	Pickup->OnRep_PickupLocationData();
	Pickup->OnRep_bPickedUp();

	auto ItemDef = (UFortItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition;
	int Count = Pickup->PrimaryPickupItemEntry.Count;
	int LoadedAmmo = Pickup->PrimaryPickupItemEntry.LoadedAmmo;

	FortInventory::GiveItem(PC, ItemDef, Count, LoadedAmmo, true, true);

	auto Entry = FortInventory::FindItemEntry(PC, &Pickup->PrimaryPickupItemEntry.ItemGuid);
	if (Entry)
		FortInventory::UpdateInventory(PC, *Entry);
}

void Player::Hook()
{
	__int64 BaseAddr = __int64(GetModuleHandleW(0));
	void** PlayerControllerVTable = AFortPlayerControllerAthena::GetDefaultObj()->VTable;
	void** UFortControllerComponent_AircraftVTable = UFortControllerComponent_Aircraft::GetDefaultObj()->VTable;

	CantBuild = decltype(CantBuild)(InSDKUtils::GetImageBase() + Addresses::CantBuild);
	ReplaceBuildingActor = decltype(ReplaceBuildingActor)(InSDKUtils::GetImageBase() + Addresses::ReplaceBuildingActor);

	THook(ServerAcknowledgePossessionHook, nullptr).VFT(PlayerControllerVTable, Indexes::ServerAcknowledgePossession);
	THook(ServerLoadingScreenDroppedHook, &ServerLoadingScreenDroppedOG).VFT(PlayerControllerVTable, Indexes::ServerLoadingScreenDropped);
	THook(ServerAttemptAircraftJumpHook, nullptr).VFT(UFortControllerComponent_AircraftVTable, Indexes::ServerAttemptAircraftJump);

	THook(ServerExecuteInventoryItem, nullptr).VFT(PlayerControllerVTable, Indexes::ServerExecuteInventoryItem);

	MH_CreateHook((LPVOID)(BaseAddr + 0x515fea4), Player::OnDamageServer, (PVOID*)&Player::OnDamageServerOG);

	MH_CreateHook((LPVOID)(BaseAddr + 0x53b4a78), Player::ClientOnPawnDiedHook, (PVOID*)&ClientOnPawnDiedOG);

	MH_CreateHook((LPVOID)(BaseAddr + 0x4B575B0), Test, (LPVOID*)&TestOG);

	THook(ServerCreateBuildingActor, nullptr).VFT(PlayerControllerVTable, Indexes::ServerCreateBuildingActor);
	THook(ServerBeginEditingBuildingActor, nullptr).VFT(PlayerControllerVTable, Indexes::ServerBeginEditingBuildingActor);
	THook(ServerEditBuildingActor, &ServerEditBuildingActorOG).VFT(PlayerControllerVTable, Indexes::ServerEditBuildingActor);
	THook(ServerEndEditingBuildingActor, nullptr).VFT(PlayerControllerVTable, Indexes::ServerEndEditingBuildingActor);

	Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x41E, Vehicles::ServerAttemptExitVehicle, (PVOID*)&Vehicles::ServerAttemptExitVehicleOG);//41F or 420
	Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x434, Vehicles::ServerRequestSeatChange, (PVOID*)&Vehicles::ServerRequestSeatChangeOG);

	for (size_t i = 0; i < UObject::GObjects->Num(); i++) {
		auto Object = UObject::GObjects->GetByIndex(i);
		if (!Object)
			continue; //More actions

			if (Object->IsA(AFortPhysicsPawn::StaticClass())) {
				Utils::HookVTable(Object->Class->DefaultObject, 0xEF, Vehicles::ServerMove);
			}
	}


	//Utils::HookVTable(AFortPlayerStateAthena::GetDefaultObj(), 0xFE, Player::ServerSetInAircraft, (PVOID*)&Player::ServerSetInAircraftOG); // or 0xFD or or or or 0xFF

	Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x4CA, Player::ServerPlaySquadQuickChatMessage, nullptr);

	Utils::SwapVFTs(APlayerPawn_Athena_C::StaticClass()->DefaultObject, 0x1EC, Player::ServerReviveFromDBNO, nullptr);
	MH_CreateHook((LPVOID)(BaseAddr + 0x4785400), Player::PickTeam, nullptr);

	//MH_CreateHook((LPVOID)(BaseAddr + 0x31EB470), Player::OnReload, (PVOID*)&Player::OnReloadOG);

	Utils::SwapVFTs(APlayerPawn_Athena_C::StaticClass()->DefaultObject, 0x216, ServerSendZiplineStateHook, nullptr);
	Utils::SwapVFTs(APlayerPawn_Athena_C::StaticClass()->DefaultObject, 0x208, Player::ServerHandlePickup, nullptr);
	MH_CreateHook((LPVOID)(BaseAddr + 0x52583cc), Player::ServerAttemptInteractHook, (LPVOID*)&Player::ServerAttemptInteractOG);
	Utils::SwapVFTs(AAthena_PlayerController_C::StaticClass()->DefaultObject, 0x221, Player::ServerAttemptInventoryDrop, nullptr);



}