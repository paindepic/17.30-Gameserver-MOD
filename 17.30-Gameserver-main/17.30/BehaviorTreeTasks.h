#pragma once
#include "framework.h"
#include "BehaviorTreeSystem.h"

class BTTask_Wait : public BTNode {
public:
    float WaitTime = 0.f;
    float WorldWaitTime = 0.f;
    bool bFinishedWait = false;
public:
    BTTask_Wait(float InWaitTime) {
        WaitTime = InWaitTime;
        WorldWaitTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + InWaitTime;
    }

    EBTNodeResult ChildTask(BTContext Context) override
    {
        if (WaitTime == 0.f || WorldWaitTime == 0.f) {
            return EBTNodeResult::Failed;
        }
        if (UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) >= WorldWaitTime) {
            if (bFinishedWait) {
                WorldWaitTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + WaitTime;
                bFinishedWait = false;
                return EBTNodeResult::InProgress;
            }
            bFinishedWait = true;
            if (!NodeName.empty())
            {
                Log("BTTask_Wait Finished Wait For NodeName: " + NodeName);
            }
            return EBTNodeResult::Succeeded;
        }
        return EBTNodeResult::InProgress;
    }
};

class BTTask_BotMoveTo : public BTNode {
public:
    float AcceptableRadius = 50.f;
    bool bAllowStrafe = true;
    bool bStopOnOverlapNeedsUpdate = false;
    bool bUsePathfinding = false;
    bool bProjectDestinationToNavigation = false;
    bool bAllowPartialPath = true;

    bool bShouldSetFocalPoint = true;

    FName SelectedKeyName;
public:
    EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Controller) {
            return EBTNodeResult::Failed;
        }

        FVector Dest = Context.Controller->Blackboard->GetValueAsVector(SelectedKeyName);
        if (Dest.IsZero()) return EBTNodeResult::Failed;

        if (bShouldSetFocalPoint)
        {
            Context.Controller->K2_SetFocalPoint(Dest);
        }
        EPathFollowingRequestResult RequestResult = Context.Controller->MoveToLocation(Dest, AcceptableRadius, bStopOnOverlapNeedsUpdate, bUsePathfinding, bProjectDestinationToNavigation, bAllowStrafe, nullptr, bAllowPartialPath);
        if (RequestResult == EPathFollowingRequestResult::Failed) {
            return EBTNodeResult::Failed;
        }

        if (RequestResult == EPathFollowingRequestResult::RequestSuccessful) {
            return EBTNodeResult::InProgress;
        }

        return EBTNodeResult::Succeeded;
    }
};

class BTTask_SteerMovement : public BTNode {
public:
    float RandDirOffset = 600.f;
    float DirectionChangeInterval = 1.f;
    float NextDirectionChangeTime = 0.f;
public:
    BTTask_SteerMovement(float Offset = 600.f, float DestChangeInterval = 1.f)
    {
        RandDirOffset = Offset;
        DirectionChangeInterval = DestChangeInterval;
    }

    EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) {
            return EBTNodeResult::Failed;
        }
        float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

        if (DirectionChangeInterval == 0.f || CurrentTime >= NextDirectionChangeTime)
        {
            FVector OffsetLoc = Context.Pawn->K2_GetActorLocation();
            OffsetLoc.X += UKismetMathLibrary::RandomFloatInRange((RandDirOffset * -1.f), RandDirOffset);
            OffsetLoc.Y += UKismetMathLibrary::RandomFloatInRange((RandDirOffset * -1.f), RandDirOffset);

            EPathFollowingRequestResult Result = Context.Controller->MoveToLocation(OffsetLoc, (RandDirOffset / 10), false, false, false, true, nullptr, true);
            NextDirectionChangeTime = CurrentTime + DirectionChangeInterval;

            if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
            {
                return EBTNodeResult::Succeeded;
            }
            else if (Result == EPathFollowingRequestResult::RequestSuccessful)
            {
                return EBTNodeResult::InProgress;
            }
        }

        return EBTNodeResult::Failed;
    }
};

class BTTask_RunSelector : public BTNode {
public:
    BTComposite_Selector* SelectorToRun = nullptr;
public:
    virtual EBTNodeResult ChildTask(BTContext Context) override {
        if (SelectorToRun) {
            return SelectorToRun->Tick(Context);
        }
        return EBTNodeResult::Failed;
    }
};

class BTTask_Dive : public BTNode {
public:
    virtual EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) {
            return EBTNodeResult::Failed;
        }

        if (Context.Pawn->IsSkydiving()) {
            FVector LocationToGo = Context.Pawn->K2_GetActorLocation();
            LocationToGo.Z -= 100.f;

            Context.Controller->K2_SetFocalPoint(LocationToGo);
            Context.Controller->MoveToLocation(LocationToGo, 20.f, true, false, false, true, nullptr, true);
        }
        else {
            Context.Pawn->BeginSkydiving(false);
        }

        return EBTNodeResult::Succeeded;
    }
};

class BTTask_Glide : public BTNode {
public:
    virtual EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) {
            return EBTNodeResult::Failed;
        }

        // I dont think we need any custom logic right now for gliding
        return EBTNodeResult::Succeeded;
    }
};

class BTTask_ShootTrap : public BTNode {
public:
    FName TargetActorKey;
public:
    virtual EBTNodeResult ChildTask(BTContext Context) override {
        // I will look into this later, since im not sure the best way to do this
        return EBTNodeResult::Failed;
    }
};

class BTTask_SearchForLoot : public BTNode {
public:
    float SearchRadius = 5000.f;
    float AcceptableRadius = 100.f;
    float SearchInterval = 5.f;
    float LastSearchTime = 0.f;
    FVector CachedLootLocation;
    bool bHasFoundLoot = false;
public:
    BTTask_SearchForLoot(float InSearchRadius = 5000.f, float InSearchInterval = 5.f) {
        SearchRadius = InSearchRadius;
        SearchInterval = InSearchInterval;
        NodeName = "Search For Loot";
    }

    EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Controller || !Context.Pawn) {
            return EBTNodeResult::Failed;
        }

        float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

        if (bHasFoundLoot && !CachedLootLocation.IsZero()) {
            float Distance = Context.Pawn->GetDistanceTo(Context.Pawn->K2_GetActorLocation());
            if (Distance < AcceptableRadius) {
                bHasFoundLoot = false;
                CachedLootLocation = FVector();
                return EBTNodeResult::Succeeded;
            }

            Context.Controller->K2_SetFocalPoint(CachedLootLocation);
            EPathFollowingRequestResult Result = Context.Controller->MoveToLocation(CachedLootLocation, AcceptableRadius, false, true, false, true, nullptr, true);
            
            if (Result == EPathFollowingRequestResult::RequestSuccessful) {
                return EBTNodeResult::InProgress;
            } else if (Result == EPathFollowingRequestResult::AlreadyAtGoal) {
                bHasFoundLoot = false;
                CachedLootLocation = FVector();
                return EBTNodeResult::Succeeded;
            }
        }

        if (CurrentTime - LastSearchTime >= SearchInterval || LastSearchTime == 0.f) {
            TArray<AActor*> FoundPickups;
            UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPickup::StaticClass(), &FoundPickups);

            AActor* ClosestPickup = nullptr;
            float ClosestDistance = SearchRadius;

            for (int32 i = 0; i < FoundPickups.Num(); i++) {
                if (!FoundPickups[i]) continue;

                float Distance = Context.Pawn->GetDistanceTo(FoundPickups[i]);
                if (Distance < ClosestDistance) {
                    ClosestDistance = Distance;
                    ClosestPickup = FoundPickups[i];
                }
            }

            if (ClosestPickup) {
                CachedLootLocation = ClosestPickup->K2_GetActorLocation();
                bHasFoundLoot = true;
                LastSearchTime = CurrentTime;
                return EBTNodeResult::InProgress;
            }

            LastSearchTime = CurrentTime;
        }

        return EBTNodeResult::Failed;
    }
};

class BTTask_FindEnemy : public BTNode {
public:
    float SearchRadius = 8000.f;
    float SearchInterval = 2.f;
    float LastSearchTime = 0.f;
    AActor* CachedEnemy = nullptr;
public:
    BTTask_FindEnemy(float InSearchRadius = 8000.f, float InSearchInterval = 2.f) {
        SearchRadius = InSearchRadius;
        SearchInterval = InSearchInterval;
        NodeName = "Find Enemy";
    }

    EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Controller || !Context.Pawn) {
            return EBTNodeResult::Failed;
        }

        float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

        if (CachedEnemy && !CachedEnemy->IsPendingKillOrNull()) {
            auto EnemyPawn = Cast<AFortPlayerPawnAthena>(CachedEnemy);
            if (EnemyPawn && !EnemyPawn->IsDead()) {
                float Distance = Context.Pawn->GetDistanceTo(CachedEnemy);
                if (Distance <= SearchRadius) {
                    Context.Controller->K2_SetFocalPoint(CachedEnemy->K2_GetActorLocation());
                    Context.Controller->Blackboard->SetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"), CachedEnemy);
                    return EBTNodeResult::Succeeded;
                }
            }
            CachedEnemy = nullptr;
        }

        if (CurrentTime - LastSearchTime >= SearchInterval || LastSearchTime == 0.f) {
            TArray<AActor*> AllPawns;
            UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerPawnAthena::StaticClass(), &AllPawns);

            AActor* ClosestEnemy = nullptr;
            float ClosestDistance = SearchRadius;

            for (int32 i = 0; i < AllPawns.Num(); i++) {
                if (!AllPawns[i] || AllPawns[i] == Context.Pawn) continue;

                auto EnemyPawn = Cast<AFortPlayerPawnAthena>(AllPawns[i]);
                if (!EnemyPawn || EnemyPawn->IsDead()) continue;

                auto EnemyState = Cast<AFortPlayerStateAthena>(EnemyPawn->PlayerState);
                auto MyState = Cast<AFortPlayerStateAthena>(Context.PlayerState);
                
                if (EnemyState && MyState) {
                    if (EnemyState->TeamIndex == MyState->TeamIndex) continue;
                }

                float Distance = Context.Pawn->GetDistanceTo(AllPawns[i]);
                if (Distance < ClosestDistance) {
                    ClosestDistance = Distance;
                    ClosestEnemy = AllPawns[i];
                }
            }

            if (ClosestEnemy) {
                CachedEnemy = ClosestEnemy;
                Context.Controller->K2_SetFocalPoint(ClosestEnemy->K2_GetActorLocation());
                Context.Controller->Blackboard->SetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"), ClosestEnemy);
                LastSearchTime = CurrentTime;
                return EBTNodeResult::Succeeded;
            }

            LastSearchTime = CurrentTime;
        }

        return EBTNodeResult::Failed;
    }
};

class BTTask_EngageCombat : public BTNode {
public:
    float MinEngageDistance = 500.f;
    float MaxEngageDistance = 3000.f;
    float AcceptableRadius = 200.f;
public:
    BTTask_EngageCombat(float InMinDistance = 500.f, float InMaxDistance = 3000.f) {
        MinEngageDistance = InMinDistance;
        MaxEngageDistance = InMaxDistance;
        NodeName = "Engage Combat";
    }

    EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Controller || !Context.Pawn) {
            return EBTNodeResult::Failed;
        }

        AActor* Target = (AActor*)Context.Controller->Blackboard->GetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"));
        if (!Target || Target->IsPendingKillOrNull()) {
            return EBTNodeResult::Failed;
        }

        auto EnemyPawn = Cast<AFortPlayerPawnAthena>(Target);
        if (!EnemyPawn || EnemyPawn->IsDead()) {
            Context.Controller->Blackboard->SetValueAsObject(ConvFName(L"AIEvaluator_RangeAttack_Target"), nullptr);
            return EBTNodeResult::Failed;
        }

        float Distance = Context.Pawn->GetDistanceTo(Target);
        
        if (Distance > MaxEngageDistance) {
            FVector TargetLocation = Target->K2_GetActorLocation();
            Context.Controller->K2_SetFocalPoint(TargetLocation);
            Context.Controller->MoveToLocation(TargetLocation, AcceptableRadius, false, true, false, true, nullptr, true);
            return EBTNodeResult::InProgress;
        } else if (Distance < MinEngageDistance) {
            FVector MyLocation = Context.Pawn->K2_GetActorLocation();
            FVector TargetLocation = Target->K2_GetActorLocation();
            FVector Direction = MyLocation - TargetLocation;
            Direction.Normalize();
            FVector RetreatLocation = MyLocation + (Direction * 300.f);
            
            Context.Controller->K2_SetFocalPoint(TargetLocation);
            Context.Controller->MoveToLocation(RetreatLocation, 50.f, false, false, false, true, nullptr, true);
            return EBTNodeResult::InProgress;
        } else {
            Context.Controller->K2_SetFocalPoint(Target->K2_GetActorLocation());
            Context.Controller->StopMovement();
            return EBTNodeResult::Succeeded;
        }
    }
};