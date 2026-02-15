#pragma once
#include "framework.h"
#include "BehaviorTreeSystem.h"

class BTService_HandleFocusing_ScanAroundOnly : public BTService {
public:
    float Offset = 100.f;
public:
    BTService_HandleFocusing_ScanAroundOnly(float InInterval = 0.3f, float InOffset = 100.f) {
        NodeName = "Focus Scan Around Only";

        Interval = InInterval;
        Offset = InOffset;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) return;
        auto Math = UKismetMathLibrary::GetDefaultObj();

        auto BotPos = Context.Pawn->K2_GetActorLocation();
        FVector FocusLocation = Context.Pawn->K2_GetActorLocation();
        FocusLocation.X += UKismetMathLibrary::RandomFloatInRange((Offset * -1.f), Offset);
        FocusLocation.Y += UKismetMathLibrary::RandomFloatInRange((Offset * -1.f), Offset);
        FocusLocation.Z += UKismetMathLibrary::RandomFloatInRange(((Offset / 2) * -1.f), (Offset / 2));

        FRotator Rot = Math->FindLookAtRotation(BotPos, FocusLocation);

        Context.Controller->SetControlRotation(Rot);
        Context.Controller->K2_SetActorRotation(Rot, true);

        Context.Controller->K2_SetFocalPoint(FocusLocation);
    }
};

class BTService_ThankBusDriver : public BTService {
public:
    BTService_ThankBusDriver(float InInterval = 0.2f) {
        NodeName = "Random Thank Bus Driver";

        Interval = InInterval;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller || !Context.PlayerState) return;
        
        if (!Context.PlayerState->bInAircraft) return;
        if (Context.PlayerState->bThankedBusDriver) return;

        if (UKismetMathLibrary::RandomBoolWithWeight(0.005f)) {
            Context.Controller->ThankBusDriver();
        }
    }
};

class BTService_JumpOffBus : public BTService {
public:
    float Weight = 0.005f;
public:
    BTService_JumpOffBus(float InWeight = 0.005f) {
        NodeName = "Executing... Jump Off The Bus";

        Interval = UKismetMathLibrary::RandomFloatInRange(0.05f, 0.2f);
        Weight = InWeight;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller || !Context.PlayerState) return;

        if (!Context.PlayerState->bInAircraft) return;

        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        if (GameState->bAircraftIsLocked) return;

        if (UKismetMathLibrary::RandomBoolWithWeight(Weight)) {
            FVector Destination = Context.Controller->Blackboard->GetValueAsVector(ConvFName(L"AIEvaluator_JumpOffBus_Destination"));

            Context.Controller->Blackboard->SetValueAsVector(ConvFName(L"AIEvaluator_Dive_Destination"), Destination);
            Context.Controller->Blackboard->SetValueAsVector(ConvFName(L"AIEvaluator_Glide_Destination"), Destination);

            Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Dive_ExecutionStatus"), (int)EExecutionStatus::ExecutionAllowed);
            Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Glide_ExecutionStatus"), (int)EExecutionStatus::ExecutionDenied);

            Context.Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
            Context.Pawn->BeginSkydiving(true);

            Context.PlayerState->bInAircraft = false;
            Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_Global_IsInBus"), false);

            Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_Global_HasEverJumpedFromBusKey"), true);
        }
    }
};

class BTService_ManageWeapon : public BTService {
public:
    BTService_ManageWeapon(float InInterval = 0.1f) {
        NodeName = "Manage Weapon and Firing";
        Interval = InInterval;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) return;

        AActor* Target = (AActor*)Context.Controller->Blackboard->GetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"));
        
        if (Target) {
            auto EnemyPawn = Cast<AFortPlayerPawnAthena>(Target);
            if (EnemyPawn && !EnemyPawn->IsDead()) {
                if (Context.Pawn->CurrentWeapon) {
                    if (!Context.Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass())) {
                        float Distance = Context.Pawn->GetDistanceTo(Target);
                        if (Distance < 5000.f) {
                            Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_ManageWeapon_Fire"), true);
                            Context.Pawn->PawnStartFire(0);
                            return;
                        }
                    }
                }
            }
        }

        Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_ManageWeapon_Fire"), false);
        Context.Pawn->PawnStopFire(0);
    }
};

class BTService_UpdateCombatState : public BTService {
public:
    BTService_UpdateCombatState(float InInterval = 1.f) {
        NodeName = "Update Combat State";
        Interval = InInterval;
    }

    void TickService(BTContext Context) override {
        if (!Context.Controller) return;

        AActor* Target = (AActor*)Context.Controller->Blackboard->GetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"));
        
        if (Target) {
            auto EnemyPawn = Cast<AFortPlayerPawnAthena>(Target);
            if (EnemyPawn && !EnemyPawn->IsDead()) {
                Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_Combat_InCombat"), true);
                return;
            }
        }

        Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_Combat_InCombat"), false);
        Context.Controller->Blackboard->SetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"), nullptr);
    }
};

class BTService_LobbyDance : public BTService {
public:
    float DanceInterval = 15.f;
    float NextDanceTime = 0.f;
    int LastDanceIndex = -1;

public:
    BTService_LobbyDance(float InInterval = 15.f) {
        NodeName = "Lobby Dance Service";
        Interval = 1.f;
        DanceInterval = InInterval;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.PlayerState || !Context.Controller) return;

        float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

        if (CurrentTime < NextDanceTime) return;

        auto* FortPawn = Cast<AFortPlayerPawnAthena>(Context.Pawn);
        if (!FortPawn) return;

        if (FortPawn->CosmeticLoadout.Dances.Num() > 0) {
            int DanceIndex;
            do {
                DanceIndex = UKismetMathLibrary::RandomIntegerInRange(0, FortPawn->CosmeticLoadout.Dances.Num() - 1);
            } while (DanceIndex == LastDanceIndex && FortPawn->CosmeticLoadout.Dances.Num() > 1);

            LastDanceIndex = DanceIndex;

            UAthenaDanceItemDefinition* Dance = FortPawn->CosmeticLoadout.Dances[DanceIndex];
            if (Dance) {
                Log("Bot playing dance: " + Dance->GetName());
                
                auto* PlayerController = Cast<AFortPlayerController>(Context.Controller);
                if (PlayerController) {
                    PlayerController->PlayEmoteItem(Dance, EFortEmotePlayMode::ForcePlay);
                }
            }
        }

        NextDanceTime = CurrentTime + DanceInterval + UKismetMathLibrary::RandomFloatInRange(-5.f, 5.f);
    }
};

class BTService_LobbyFindTarget : public BTService {
public:
    float SearchRadius = 5000.f;
    float SearchInterval = 2.f;
    float LastSearchTime = 0.f;

public:
    BTService_LobbyFindTarget(float InSearchRadius = 5000.f, float InSearchInterval = 2.f) {
        NodeName = "Lobby Find Target";
        Interval = 0.5f;
        SearchRadius = InSearchRadius;
        SearchInterval = InSearchInterval;
    }

    void TickService(BTContext Context) override {
        if (!Context.Controller || !Context.Pawn) return;

        float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

        if (CurrentTime - LastSearchTime < SearchInterval && LastSearchTime != 0.f) return;

        AActor* CurrentTarget = (AActor*)Context.Controller->Blackboard->GetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"));
        if (CurrentTarget) {
            auto EnemyPawn = Cast<AFortPlayerPawnAthena>(CurrentTarget);
            if (EnemyPawn && !EnemyPawn->IsDead()) {
                float Distance = Context.Pawn->GetDistanceTo(CurrentTarget);
                if (Distance <= SearchRadius) {
                    return;
                }
            }
        }

        TArray<AActor*> AllPawns;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerPawnAthena::StaticClass(), &AllPawns);

        AActor* ClosestTarget = nullptr;
        float ClosestDistance = SearchRadius;

        for (int32 i = 0; i < AllPawns.Num(); i++) {
            if (!AllPawns[i] || AllPawns[i] == Context.Pawn) continue;

            auto TargetPawn = Cast<AFortPlayerPawnAthena>(AllPawns[i]);
            if (!TargetPawn || TargetPawn->IsDead()) continue;

            auto TargetState = Cast<AFortPlayerStateAthena>(TargetPawn->PlayerState);
            if (!TargetState) continue;

            if (TargetState->bIsABot) continue;

            float Distance = Context.Pawn->GetDistanceTo(AllPawns[i]);
            if (Distance < ClosestDistance) {
                ClosestDistance = Distance;
                ClosestTarget = AllPawns[i];
            }
        }

        if (ClosestTarget) {
            Context.Controller->K2_SetFocalPoint(ClosestTarget->K2_GetActorLocation());
            Context.Controller->Blackboard->SetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"), ClosestTarget);
        }

        LastSearchTime = CurrentTime;
    }
};

class BTService_LobbyShooting : public BTService {
public:
    float MinFireInterval = 0.5f;
    float MaxFireInterval = 2.f;
    float NextFireTime = 0.f;
    float BurstDuration = 0.f;
    float BurstEndTime = 0.f;
    bool bIsBursting = false;

public:
    BTService_LobbyShooting(float InMinFireInterval = 0.5f, float InMaxFireInterval = 2.f) {
        NodeName = "Lobby Shooting Service";
        Interval = 0.1f;
        MinFireInterval = InMinFireInterval;
        MaxFireInterval = InMaxFireInterval;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) return;

        float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

        AActor* Target = (AActor*)Context.Controller->Blackboard->GetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"));
        if (!Target) {
            if (bIsBursting) {
                Context.Pawn->PawnStopFire(0);
                bIsBursting = false;
            }
            return;
        }

        auto EnemyPawn = Cast<AFortPlayerPawnAthena>(Target);
        if (!EnemyPawn || EnemyPawn->IsDead()) {
            if (bIsBursting) {
                Context.Pawn->PawnStopFire(0);
                bIsBursting = false;
            }
            return;
        }

        if (Context.Pawn->CurrentWeapon && Context.Pawn->CurrentWeapon->WeaponData &&
            Context.Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass())) {
            return;
        }

        float Distance = Context.Pawn->GetDistanceTo(Target);
        if (Distance > 4000.f) return;

        Context.Controller->K2_SetFocalPoint(Target->K2_GetActorLocation());

        if (!bIsBursting) {
            if (CurrentTime >= NextFireTime) {
                bIsBursting = true;
                BurstDuration = UKismetMathLibrary::RandomFloatInRange(0.5f, 2.f);
                BurstEndTime = CurrentTime + BurstDuration;
                Context.Pawn->PawnStartFire(0);
            }
        } else {
            if (CurrentTime >= BurstEndTime) {
                Context.Pawn->PawnStopFire(0);
                bIsBursting = false;
                NextFireTime = CurrentTime + UKismetMathLibrary::RandomFloatInRange(MinFireInterval, MaxFireInterval);
            } else {
                Context.Pawn->PawnStartFire(0);
            }
        }
    }
};