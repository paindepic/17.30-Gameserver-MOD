#pragma once

#include "Framework.h"
#include "../Public/Utils.h"

namespace Player
{
	inline void (*ServerLoadingScreenDropped)(const AFortPlayerControllerAthena*);

	void ServerAcknowledgePossessionHook(AFortPlayerControllerAthena* PlayerController, APawn* P);
	void ServerAttemptAircraftJumpHook(const UFortControllerComponent_Aircraft* ControllerComponent, const FRotator& ClientRotation);
	void ServerLoadingScreenDroppedHook(AFortPlayerControllerAthena* PC);
	static void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PlayerController, FGuid& ItemGuid);
	inline __int64 (*OnDamageServerOG)(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext);
	__int64 OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext);
	void ServerEditBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored);
	void ServerEndEditingBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToStopEditing);
	static inline void (*ServerCreateBuildingActorOG)(AFortPlayerControllerAthena* PlayerController, FCreateBuildingActorData& CreateBuildingData);
	static void ServerCreateBuildingActor(AFortPlayerControllerAthena* PlayerController, FCreateBuildingActorData& CreateBuildingData);
	void ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToEdit);
	inline __int64 (*CantBuild)(UObject*, UObject*, FVector, FRotator, char, TArray<ABuildingSMActor*>*, char*);
	inline ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor*, __int64, UClass*, int, int, uint8_t, AFortPlayerController*);
	inline void (*ServerEditBuildingActorOG)(AFortPlayerController*, ABuildingSMActor*, TSubclassOf<ABuildingSMActor>, int32, bool);
	static inline void (*ServerSetInAircraftOG)(AFortPlayerState*, bool);

	void ServerPlaySquadQuickChatMessage(AFortPlayerControllerAthena* PlayerController, FAthenaQuickChatActiveEntry ChatEntry, __int64);

	static void ServerSetInAircraft(AFortPlayerState* PlayerState, bool bNewInAircraft);
	void ServerReviveFromDBNO(AFortPlayerPawnAthena* Pawn, AFortPlayerControllerAthena* Instigator);
	EFortTeam PickTeam();

	inline void (*OnReloadOG)(AFortWeapon* Weapon, int AmmoUsed);
	void OnReload(AFortWeapon* Weapon, int AmmoUsed);
	static void ClientOnPawnDiedHook(AFortPlayerControllerZone* DeadPlayer, FFortPlayerDeathReport& DeathReport);
	void ServerHandlePickup(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo Info);
	void ServerAttemptInteractHook(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted);
	inline void (*ServerAttemptInteractOG)(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted);
	void ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PC, FGuid ItemGuid, int Count, bool bTrash);
	void Hook();
}