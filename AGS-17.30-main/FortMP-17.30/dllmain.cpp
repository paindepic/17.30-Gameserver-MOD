#include "Server/Public/Framework.h"
#include "Vendor/Memcury.h"

#include "Server/Public/Offsets.h"
#include "Server/Public/Utils.h"

#include "Server/Public/GameMode.h"
#include "Server/Public/NetDriver.h"
#include "Server/Public/Player.h"
#include "Server/Public/Abilities.h"
#include "Server/Public/Misc.h"

DWORD WINAPI Main(LPVOID)
{
    AllocConsole();
	srand(time(0));

	SetConsoleTitleA("FortMP 17.30 | Waiting");

    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

	Addresses::BaseAddress = InSDKUtils::GetImageBase();

	THook::Init();
	
	THook(THook::RetTrue, nullptr).MinHook(Addresses::KickPlayer);
	THook(NetDriver::GetNetMode, nullptr).MinHook(Addresses::WorldNetMode);
	THook(THook::RetTrue, nullptr).MinHook(0x4d79308); 
	THook(THook::RetFalse, nullptr).MinHook(0xfe4eb0);
	THook(THook::RetTrue, nullptr).MinHook(0x3860958); 
	THook(DispatchRequestHook, &DispatchRequest).MinHook(Addresses::DispatchRequest);
	THook(THook::RetTrue, nullptr).MinHook(Addresses::ChangeGameSessionId);

	Utils::Null(InSDKUtils::GetImageBase() + 0x2F327B0); 
	Utils::Null(InSDKUtils::GetImageBase() + 0x1371e94);
	Utils::Null(InSDKUtils::GetImageBase() + 0x28932fc);
	Utils::Null(InSDKUtils::GetImageBase() + 0x4efb7dc);

	*(bool*)(InSDKUtils::GetImageBase() + 0x973E49B) = false;
	*(bool*)(InSDKUtils::GetImageBase() + 0x973E49C) = true;

	GameMode::Hook();
	NetDriver::Hook();
	Player::Hook();
	Abilities::Hook();

	MH_EnableHook(MH_ALL_HOOKS);
	
	UFortEngine::GetEngine()->GameInstance->LocalPlayers.Remove(0);
	UGameplayStatics::OpenLevel(UWorld::GetWorld(), UKismetStringLibrary::Conv_StringToName(L"Apollo_Terrain"), true, FString());
	
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, const DWORD ulReasonForCall
)
{
    switch (ulReasonForCall)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, 0, Main, nullptr, 0, nullptr);
        break;
    default:
        break;
    }
    return TRUE;
}

