#pragma once
#include "framework.h"
#include "BehaviorTreeSystem.h"

#include "BehaviorTreeDecorators.h"
#include "BehaviorTreeEvaluators.h"
#include "BehaviorTreeServices.h"
#include "BehaviorTreeTasks.h"

namespace PlayerBots {
    std::vector<class PhoebeBot*> PhoebeBots{};
    class PhoebeBot
    {
    public:
        // The behaviortree for the new ai system
        BehaviorTree* BT_Phoebe = nullptr;

        // The context that should be sent to the behaviortree
        BTContext Context = {};

        // The playercontroller of the bot
        AFortAthenaAIBotController* PC;

        // The Pawn of the bot
        AFortPlayerPawnAthena* Pawn;

        // The PlayerState of the bot
        AFortPlayerStateAthena* PlayerState;

        // Are we ticking the bot?
        bool bTickEnabled = true;

        // So we can track the current tick that the bot is doing
        uint64_t tick_counter = 0;

    public:
        PhoebeBot(AFortAthenaAIBotController* PC, AFortPlayerPawnAthena* Pawn, AFortPlayerStateAthena* PlayerState)
        {
            this->PC = PC;
            this->Pawn = Pawn;
            this->PlayerState = PlayerState;

            Context.Controller = PC;
            Context.Pawn = Pawn;
            Context.PlayerState = PlayerState;

            PhoebeBots.push_back(this);
        }

        bool IsPickaxeEquiped() {
            if (!Pawn || !Pawn->CurrentWeapon)
                return false;

            if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
            {
                return true;
            }
            return false;
        }

        void EquipPickaxe()
        {
            if (!Pawn || !Pawn->CurrentWeapon)
                return;

            if (IsPickaxeEquiped()) {
                return;
            }

            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
                {
                    Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition, PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid, PC->Inventory->Inventory.ReplicatedEntries[i].TrackerGuid, false);
                    break;
                }
            }
        }

        void SwitchToWeapon() {
            if (!Pawn || !Pawn->CurrentWeapon || !Pawn->CurrentWeapon->WeaponData || !PC || !PC->Inventory)
                return;

            if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass())) {
                return;
            }

            if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
            {
                for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
                {
                    auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
                    if (Entry.ItemDefinition) {
                        if (Entry.ItemDefinition->ItemType == EFortItemType::Weapon) {
                            Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, Entry.ItemGuid, Entry.TrackerGuid, false);
                            break;
                        }
                    }
                }
            }
        }
    };

    BehaviorTree* ConstructBehaviorTree() {
        auto* Tree = new BehaviorTree();

        auto* RootSelector = new BTComposite_Selector();
        RootSelector->NodeName = "Alive";

        {
            auto* Selector = new BTComposite_Selector();
            Selector->NodeName = "In Bus";

            {
                auto* Task = new BTTask_Wait(999.f);
                auto* Decorator = new BTDecorator_CheckEnum();
                Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Global_GamePhaseStep");
                Decorator->IntValue = (int)EAthenaGamePhaseStep::BusLocked;
                Task->AddDecorator(Decorator);
                Selector->AddChild(Task);
            }

            {
                auto* Task = new BTTask_Wait(999.f);
                auto* Decorator = new BTDecorator_CheckEnum();
                Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Global_GamePhaseStep");
                Decorator->IntValue = (int)EAthenaGamePhaseStep::BusFlying;
                Task->AddDecorator(Decorator);

                Selector->AddChild(Task);
            }

            {
                auto* Task = new BTTask_Wait(1.f);
                Selector->AddChild(Task);
            }

            {
                auto* Service = new BTService_ThankBusDriver();
                Selector->AddService(Service);
            }

            {
                auto* Service = new BTService_JumpOffBus();
                Selector->AddService(Service);
            }

            Tree->AllNodes.push_back(Selector);
        }

        {
            auto* Selector = new BTComposite_Selector();
            Selector->NodeName = "On Ground";

            {
                auto* Task = new BTTask_Wait(0.5f);
                auto* Decorator = new BTDecorator_CheckEnum();
                Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Global_GamePhaseStep");
                Decorator->IntValue = (int)EAthenaGamePhaseStep::EndGame;
                Decorator->Operator = EBlackboardCompareOp::GreaterThanOrEqual;
                Task->AddDecorator(Decorator);
                Selector->AddChild(Task);
            }

            {
                auto* Task = new BTTask_Wait(1.f);
                auto* Decorator = new BTDecorator_IsSet();
                Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Global_IsMovementBlocked");
                Task->AddDecorator(Decorator);
                Selector->AddChild(Task);
            }

            {
                auto* Selector_Combat = new BTComposite_Selector();
                Selector_Combat->NodeName = "Combat";

                {
                    auto* Task = new BTTask_EngageCombat(500.f, 3000.f);
                    auto* Service = new BTService_ManageWeapon(0.1f);
                    Task->AddService(Service);
                    auto* Decorator = new BTDecorator_IsSet();
                    Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Combat_InCombat");
                    Task->AddDecorator(Decorator);
                    Selector_Combat->AddChild(Task);
                }

                {
                    auto* Task = new BTTask_FindEnemy(8000.f, 2.f);
                    Selector_Combat->AddChild(Task);
                }

                {
                    auto* Service = new BTService_UpdateCombatState(1.f);
                    Selector_Combat->AddService(Service);
                }

                Tree->AllNodes.push_back(Selector_Combat);
                
                auto* Task_RunCombat = new BTTask_RunSelector();
                Task_RunCombat->SelectorToRun = Selector_Combat;
                Selector->AddChild(Task_RunCombat);
            }

            {
                auto* Task = new BTTask_SearchForLoot(5000.f, 5.f);
                Selector->AddChild(Task);
            }

            {
                auto* Task = new BTTask_SteerMovement(1200.f, 3.f);
                auto* Service = new BTService_HandleFocusing_ScanAroundOnly(0.4f, 150.f);
                Task->AddService(Service);
                Selector->AddChild(Task);
            }

            Tree->AllNodes.push_back(Selector);
        }

        {
            auto* Selector = new BTComposite_Selector();
            Selector->NodeName = "Skydiving";

            {
                auto* Task = new BTTask_Dive();
                auto* Decorator = new BTDecorator_CheckEnum();
                Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Dive_ExecutionStatus");
                Decorator->IntValue = (int)EExecutionStatus::ExecutionAllowed;
                Task->AddDecorator(Decorator);
                Selector->AddChild(Task);
            }

            {
                auto* Task = new BTTask_Glide();
                auto* Decorator = new BTDecorator_CheckEnum();
                Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Glide_ExecutionStatus");
                Decorator->IntValue = (int)EExecutionStatus::ExecutionAllowed;
                Task->AddDecorator(Decorator);
                Selector->AddChild(Task);
            }

            Tree->AllNodes.push_back(Selector);
        }

        {
            auto* Task = new BTTask_Wait(1.f);
            auto* Decorator = new BTDecorator_CheckEnum();
            Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Global_GamePhaseStep");
            Decorator->IntValue = (int)EAthenaGamePhaseStep::GetReady;
            Decorator->Operator = EBlackboardCompareOp::LessThanOrEqual;
            auto* Decorator2 = new BTDecorator_CheckEnum();
            Decorator2->SelectedKeyName = ConvFName(L"AIEvaluator_Global_GamePhase");
            Decorator2->IntValue = (int)EAthenaGamePhase::Warmup;
            Task->AddDecorator(Decorator);
            Task->AddDecorator(Decorator2);
            RootSelector->AddChild(Task);
        }

        {
            auto* Task = new BTTask_RunSelector();
            Task->SelectorToRun = Tree->FindSelectorByName("In Bus");
            auto* Decorator = new BTDecorator_IsSet();
            Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Global_IsInBus");
            Task->AddDecorator(Decorator);
            if (Task->SelectorToRun) {
                RootSelector->AddChild(Task);
            }
        }

        {
            auto* Task = new BTTask_RunSelector();
            Task->SelectorToRun = Tree->FindSelectorByName("Skydiving");
            if (Task->SelectorToRun) {
                RootSelector->AddChild(Task);
            }
        }

        {
            auto* Task = new BTTask_RunSelector();
            Task->SelectorToRun = Tree->FindSelectorByName("On Ground");
            if (Task->SelectorToRun) {
                RootSelector->AddChild(Task);
            }
        }

        Tree->RootNode = RootSelector;
        Tree->AllNodes.push_back(RootSelector);

        return Tree;
    }

    void TickBots() {
        for (auto bot : PhoebeBots)
        {
            if (!bot->bTickEnabled) continue;

            if (bot->BT_Phoebe) {
                bot->BT_Phoebe->Tick(bot->Context);
                bot->tick_counter++;
            }
        }
    }
}