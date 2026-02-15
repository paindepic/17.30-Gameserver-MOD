#include "../Public/NetDriver.h"
#include "../Public/Utils.h"
#include "../Public/Misc.h"

void NetDriver::TickFlushHook(UNetDriver* NetDriver)
{
    float TimeSeconds = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
    static bool bBusStarted = false;
    static bool bRestarting = false;

    AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState);
    AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode);
    if (!GameState || !GameMode) return TickFlush(NetDriver);

    if (NetDriver->ClientConnections.Num() >= 1)
    {
        UReplicationDriver* ReplicationDriver = NetDriver->ReplicationDriver;
        if (!ReplicationDriver)
            return TickFlush(NetDriver);

        ServerReplicateActors(ReplicationDriver);
    }

    if (NetDriver && (GameMode->MatchState == Misc::MatchState::InProgress))
    {
        if ((GetAsyncKeyState(VK_F5) & 1 || GameState->WarmupCountdownEndTime - TimeSeconds <= 0) && !bBusStarted)
        {
            UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"startaircraft", nullptr);
                      
            bBusStarted = true;
        }
    }
    else if (NetDriver && GameMode->MatchState == Misc::MatchState::WaitingPostMatch && !bRestarting)
    {
        // Restart Server

        bRestarting = true;
    }

    return TickFlush(NetDriver);
}

bool NetDriver::Listen(UWorld* World, FURL& InUrl)
{
    static FName GameNetDriver = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");

    static __int64 (*sub_D2B60C)(__int64 a1, __int64 a2) = decltype(sub_D2B60C)(InSDKUtils::GetImageBase() + Addresses::Context);

    auto context = sub_D2B60C(int64(UEngine::GetEngine()), int64(World));
    if (!context) { LOG("Failed to get context for NetDriver") context = (int64)World;  }

    UNetDriver* NetDriver = NetDriver::CreateNetDriver(UFortEngine::GetEngine(), context, GameNetDriver);
    if (!NetDriver) { LOG("Failed to create NetDriver") return false; }

    NetDriver->NetDriverName = GameNetDriver;
    NetDriver->World = World;

    FString Error;

    if (!NetDriver::InitListen(NetDriver, World, InUrl, true, Error))
        return false;

    NetDriver::SetWorld(NetDriver, World);

    World->NetDriver = NetDriver;

    for (FLevelCollection& LevelCollection : World->LevelCollections)
    {
        LevelCollection.NetDriver = NetDriver;
    }

    SetConsoleTitleA("FortMP 17.30 | Listening | Can Join: False");

    return true;
}

void NetDriver::Hook()
{
    ServerReplicateActors = decltype(ServerReplicateActors)(InSDKUtils::GetImageBase() + Addresses::ServerReplicateActors);
    SetWorld = decltype(SetWorld)(UNetDriver::GetDefaultObj()->VTable[0x72]);
    InitListen = decltype(InitListen)(InSDKUtils::GetImageBase() + Addresses::InitListen);
    CreateNetDriver = decltype(CreateNetDriver)(InSDKUtils::GetImageBase() + Addresses::CreateNetDriver);
    
    THook(TickFlushHook, &TickFlush).MinHook(Addresses::TickFlush);
}
