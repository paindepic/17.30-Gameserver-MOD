#include "../Public/Vehicles.h"
#include "../Public/Utils.h"
#include "../Public/FortInvetory.h"
#include "../Public/Player.h"

void Vehicles::SpawnVehicles() {
    auto VehicleSpawners = Utils::GetAllActorsOfClass<AFortAthenaVehicleSpawner>();

    for (auto& VehicleSpawner : VehicleSpawners) {
        if (!VehicleSpawner)
            continue;

        auto VehicleClass = VehicleSpawner->GetVehicleClass();
        if (!VehicleClass)
            continue;

        AFortAthenaVehicle* Vehicle = Utils::SpawnActor<AFortAthenaVehicle>(VehicleSpawner->K2_GetActorLocation(), VehicleSpawner->K2_GetActorRotation(), VehicleClass);

        auto FuelComponent = Cast<UFortAthenaVehicleFuelComponent>(
            Vehicle->GetComponentByClass(UFortAthenaVehicleFuelComponent::StaticClass())
        );

        if (FuelComponent) {
            FuelComponent->ServerFuel = 100;
            FuelComponent->OnRep_ServerFuel(0);
        }

        if (!Vehicle)
            continue;
    }

    VehicleSpawners.Free();
}

//void Vehicles::ServerMove(AFortPhysicsPawn* PhysicsPawn, FReplicatedPhysicsPawnState InState) {
//    if (!PhysicsPawn || !PhysicsPawn->RootComponent) { std::cout << "erm" << std::endl; return; }
//
//    UPrimitiveComponent* RootComponent = static_cast<UPrimitiveComponent*>(PhysicsPawn->RootComponent);
//
//    InState.Rotation.X -= 2.5f;
//    InState.Rotation.Y /= 0.3f;
//    InState.Rotation.Z += 2.0f;
//    InState.Rotation.W /= -1.2f;
//
//    FTransform Transform;
//    Transform.Translation = InState.Translation;
//    Transform.Rotation = InState.Rotation;
//    Transform.Scale3D = FVector(1.0f, 1.0f, 1.0f);
//
//    RootComponent->K2_SetWorldTransform(Transform, false, nullptr, true);
//    RootComponent->bComponentToWorldUpdated = true;
//
//    RootComponent->SetPhysicsLinearVelocity(InState.LinearVelocity, false, FName());
//    RootComponent->SetPhysicsAngularVelocity(InState.AngularVelocity, false, FName());
//}

void Vehicles::ServerMove(AFortPhysicsPawn* Pawn, FReplicatedPhysicsPawnState InState)
{
    printf("ServerMove called, LinearVelocity: %g, %g, %g. Angular Velocity: %g, %g, %g. RootComponent: %s. SyncKey: %d\n", InState.LinearVelocity.X, InState.LinearVelocity.Y, InState.LinearVelocity.Z, InState.AngularVelocity.X, InState.AngularVelocity.Y, InState.AngularVelocity.Z, Pawn->RootComponent->GetFullName().c_str(), (int)InState.SyncKey);
    UFortVehicleSkelMeshComponent* RootComp = (UFortVehicleSkelMeshComponent*)Pawn->RootComponent;

    Pawn->ReplicatedMovement.AngularVelocity = InState.AngularVelocity;
    Pawn->ReplicatedMovement.LinearVelocity = InState.LinearVelocity;
    Pawn->ReplicatedMovement.Location = InState.Translation;
    Pawn->ReplicatedMovement.Rotation = Utils::FQuatToRot(InState.Rotation);
    Pawn->OnRep_ReplicatedMovement();

    RootComp->SetAllPhysicsLinearVelocity(InState.LinearVelocity, false);
    RootComp->SetAllPhysicsAngularVelocityInRadians(InState.AngularVelocity, false);
    RootComp->K2_SetWorldRotation(Pawn->ReplicatedMovement.Rotation, false, nullptr, true);
    RootComp->K2_SetWorldLocation(InState.Translation, false, nullptr, true);
}


void Vehicles::GiveVehicleWeapon(AFortPlayerControllerAthena* PlayerController, AFortAthenaVehicle* Vehicle) {
    if (!PlayerController) { std::cout << "PlayerController is null" << std::endl;  return; }
    if (!Vehicle) { std::cout << "Vehicle is null" << std::endl;  return; }

    uint8 SeatIdx = PlayerController->GetVehicleSeatTransitionTargetIndex();
    if (SeatIdx != 0 || !PlayerController->MyFortPawn->IsInVehicle()) { /*std::cout << "SeatIdx is not 0" << std::endl;*/ return; }

    PlayerController->SwappingItemDefinition = PlayerController->MyFortPawn->CurrentWeapon->WeaponData;

    UFortVehicleSeatWeaponComponent* WeaponComponent = Vehicle->GetSeatWeaponComponent(0); // scuffed
    if (!WeaponComponent) { /*std::cout << std::format("WeaponComponent is null for SeatIdx: {}", SeatIdx) << std::endl;*/ return; }

    UFortWeaponItemDefinition* VehicleWeaponDefinition = WeaponComponent->WeaponSeatDefinitions[0].VehicleWeapon; // scuffed
    if (!VehicleWeaponDefinition) { /*std::cout << std::format("VehicleWeaponDefinition is null for SeatIdx: {}", SeatIdx) << std::endl;*/ return; }

    FortInventory::GiveItem(PlayerController, VehicleWeaponDefinition, 1, 9999, false, false);

    auto ItemEntry = FortInventory::FindItemEntry(PlayerController, VehicleWeaponDefinition);
    if (!ItemEntry) { std::cout << "no item entry" << std::endl; return; }

    PlayerController->MyFortPawn->EquipWeaponDefinition(VehicleWeaponDefinition, ItemEntry->ItemGuid, FGuid(), false);

    AFortWeaponRangedForVehicle* VehicleWeapon = Cast<AFortWeaponRangedForVehicle>(PlayerController->MyFortPawn->CurrentWeapon);
    if (!VehicleWeapon) { std::cout << "VehicleWeapon is null" << std::endl; return; }

    FMountedWeaponInfo MountedWeaponInfo{};
    MountedWeaponInfo.bNeedsVehicleAttachment = true;
    MountedWeaponInfo.bTargetSourceFromVehicleMuzzle = true;

    FMountedWeaponInfoRepped MountedWeaponInfoRepped{};
    MountedWeaponInfoRepped.HostVehicleCachedActor = Vehicle;
    MountedWeaponInfoRepped.HostVehicleSeatIndexCached = SeatIdx;

    VehicleWeapon->MountedWeaponInfo = MountedWeaponInfo;
    VehicleWeapon->MountedWeaponInfoRepped = MountedWeaponInfoRepped;

    VehicleWeapon->OnRep_MountedWeaponInfoRepped();
    VehicleWeapon->OnHostVehicleSetup();
}

void Vehicles::ServerAttemptExitVehicle(AFortPlayerControllerZone* PlayerController) {
    if (!PlayerController) { std::cout << "PlayerController is null" << std::endl; return; }

    ServerAttemptExitVehicleOG(PlayerController);

    uint8 SeatIdx = PlayerController->GetVehicleSeatTransitionTargetIndex();
    if (SeatIdx != 0) return;

    auto CurrentWeapon = PlayerController->MyFortPawn->CurrentWeapon;
    if (!CurrentWeapon) { std::cout << "CurrentWeapon is null" << std::endl; return; }

    auto VehicleWeaponDefinition = CurrentWeapon->WeaponData;
    if (!VehicleWeaponDefinition) { /*std::cout << "VehicleWeaponDefinition is null" << std::endl;*/ return; }

    FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PlayerController, VehicleWeaponDefinition);
    if (ItemEntry && ItemEntry->ItemDefinition->IsA(AFortWeaponRangedForVehicle::StaticClass()))
        FortInventory::RemoveItem(PlayerController, ItemEntry->ItemGuid, 1);

    UFortWorldItemDefinition* SwappingItemDefinition = ((AFortPlayerControllerAthena*)PlayerController)->SwappingItemDefinition;
    if (!SwappingItemDefinition) { /*std::cout << "SwappingItemDefinition is null" << std::endl;*/ return; }

    FFortItemEntry* SwappingItemEntry = FortInventory::FindItemEntry(PlayerController, SwappingItemDefinition);
    if (!SwappingItemEntry) { /*std::cout << "ItemEntry is null" << std::endl;*/ return; }

    PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)SwappingItemDefinition, SwappingItemEntry->ItemGuid, FGuid(), false);

    //if (PlayerController->MyFortPawn->AbilitySystemComponent)
        //PlayerController::GiveModifiers((AFortPlayerControllerAthena*)PlayerController, GetPlaylist());
}

void Vehicles::ServerRequestSeatChange(AFortPlayerControllerZone* PlayerController, int32 TargetSeatIndex) {// 0x434
    if (!PlayerController) { std::cout << "PlayerController is null" << std::endl; return; }

    uint8 CurrentSeatIndex = PlayerController->GetVehicleSeatTransitionTargetIndex();

    ServerRequestSeatChangeOG(PlayerController, TargetSeatIndex);

    auto CurrentWeapon = PlayerController->MyFortPawn->CurrentWeapon;
    UFortWeaponItemDefinition* VehicleWeaponDefinition = CurrentWeapon ? CurrentWeapon->WeaponData : nullptr;
    if (!VehicleWeaponDefinition) { /*std::cout << "VehicleWeaponDefinition is null" << std::endl;*/ return; }

    if (TargetSeatIndex == 0) {
        GiveVehicleWeapon(Cast<AFortPlayerControllerAthena>(PlayerController), PlayerController->MyFortPawn->GetVehicle());
    }
    else if (CurrentSeatIndex == 0 && TargetSeatIndex != 0) {
        FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PlayerController, VehicleWeaponDefinition);
        if (ItemEntry && ItemEntry->ItemDefinition->IsA(AFortWeaponRangedForVehicle::StaticClass()))
            FortInventory::RemoveItem(PlayerController, ItemEntry->ItemGuid, 1);

        UFortWorldItemDefinition* SwappingItemDefinition = ((AFortPlayerControllerAthena*)PlayerController)->SwappingItemDefinition;
        if (!SwappingItemDefinition) { std::cout << "SwappingItemDefinition is null" << std::endl; return; }

        FFortItemEntry* SwappingItemEntry = FortInventory::FindItemEntry(PlayerController, SwappingItemDefinition);
        if (!SwappingItemEntry) { /*std::cout << "ItemEntry is null" << std::endl;*/ return; }

        PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)SwappingItemDefinition, SwappingItemEntry->ItemGuid, FGuid(), false);
    }
}