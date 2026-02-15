#pragma once

#include <cstdint>

namespace Addresses
{
    inline uintptr_t BaseAddress = 0x0;

    constexpr uint64_t SetWorld = 0x176E8D8;
    constexpr uint64_t InitListen = 0x3A01BF4;
    constexpr uint64_t CreateNetDriver = 0x176D7C0;
    constexpr uint64_t ServerReplicateActors = 0x3DDF340;
    constexpr uint64_t Context = 0xD2B60C;
    constexpr uint64_t ChangeGameSessionId = 0x28932FC;
    constexpr uint64_t KickPlayer = 0x4D68614;
    constexpr uint64_t TickFlush = 0xE4043C;
    constexpr uint64_t GIsClient = 0x973E49B;
    constexpr uint64_t GIsServer = 0x973E49C;
    constexpr uint64_t WorldNetMode = 0xCD2164;
    constexpr uint64_t DispatchRequest = 0x15A07D8;
    constexpr uint64_t InternalTryActivateAbility = 0x3892DB8;
    constexpr uint64_t StaticLoadObject = 0x136DFB0;
    constexpr uint64_t StaticFindObject = 0xE3DF60;
    constexpr uint64_t InternalGiveAbility = 0x1343068;
    constexpr uint64_t SpecConstructor = 0x1342F9C;
    constexpr uint64_t HandleNewSafeZonePhase = 0x4799688;
    constexpr uint64_t GiveAbilityAndActivateOnce = 0x389156C;
    constexpr uint64_t SetGameMode = 0x126721C;
    constexpr uint64_t CantBuild = 0x4bbeb18;
    constexpr uint64_t ReplaceBuildingActor = 0x49a9aac;
}

namespace Indexes
{
    constexpr uint64_t ReadyToStartMatch = 0x104;    
    constexpr uint64_t SpawnDefaultPawnFor = 0xCA;   
    constexpr uint64_t ServerAcknowledgePossession = 0x114;
    constexpr uint64_t ServerAttemptAircraftJump = 0x94;
    constexpr uint64_t ServerLoadingScreenDropped = 0x27A;
    constexpr uint64_t InternalServerTryActiveAbility = 0xFE;
    constexpr uint64_t ServerExecuteInventoryItem = 0x215;
    constexpr uint64_t ServerCreateBuildingActor = 0x239;
    constexpr uint64_t ServerBeginEditingBuildingActor = 0x240;
    constexpr uint64_t ServerEditBuildingActor = 0x23B;
    constexpr uint64_t ServerEndEditingBuildingActor = 0x23E;
}