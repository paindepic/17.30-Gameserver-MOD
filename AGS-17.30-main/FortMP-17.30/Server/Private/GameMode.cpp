#include "../Public/GameMode.h"

#include "../Public/Utils.h"
#include "../Public/Offsets.h"
#include "../Public/Misc.h"

#include "../Public/NetDriver.h"

#include "../Public/XP.h"

#include "../Public/Vehicles.h"

#include "../Public/Llamas.h"

APawn* GameMode::SpawnDefaultPawnForHook(AGameMode* GameMode, AController* NewPlayer, AActor* StartSpot)
{
	return GameMode->SpawnDefaultPawnAtTransform(NewPlayer, StartSpot->GetTransform());
}

bool GameMode::ReadyToStartMatchHook(AFortGameModeAthena* GameMode)
{
	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GameMode->GameState);

	TArray<AActor*> WarmupSpots;
	UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &WarmupSpots);

	int WarmupSpotsNum = WarmupSpots.Num();

	WarmupSpots.Free();

	if (WarmupSpotsNum == 0)
		return false;

	static bool bSetup = false;
	if (!bSetup)
	{
		bSetup = true;

		if (UFortPlaylistAthena* Playlist = Misc::GetPlaylist())
		{
			GameMode->WarmupRequiredPlayerCount = 1;
			
			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GameState->CurrentPlaylistInfo.MarkArrayDirty();

			GameMode->CurrentPlaylistId = Playlist->PlaylistId;
			GameMode->CurrentPlaylistName = Playlist->PlaylistName;;

			GameState->CurrentPlaylistId = Playlist->PlaylistId;

			
			GameState->OnRep_CurrentPlaylistId();
			GameState->OnRep_CurrentPlaylistInfo();

			//GameMode->PlaylistHotfixOriginalGCFrequency = Playlist->GarbageCollectionFrequency;

			if (Playlist->bIsTournament) {
				GameState->EventTournamentRound = EEventTournamentRound::Open;
				GameState->OnRep_EventTournamentRound();
			}

			//GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
			//GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;
			//GameMode->AISettings = Playlist->AISettings;

			GameMode->bDBNOEnabled = true;
			GameState->bDBNOEnabledForGameMode = true;
			GameState->bDBNODeathEnabled = true;
			GameMode->bAllowSpectateAfterDeath = true;

			FURL URL;
			URL.Port = 7777;
			NetDriver::Listen(UWorld::GetWorld(), URL);
		}
		else
		{
			LOG("Playlist not found")
			return false;
		}

		GameMode->bWorldIsReady = true;
	}

	if (!GameState->MapInfo || !UWorld::GetWorld()->NetDriver)
		return false;

	static bool bSetMapInfo = false;
	if (!bSetMapInfo)
	{
		bSetMapInfo = true;

		GameState->WarmupCountdownEndTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 99999.f;
		GameMode->WarmupCountdownDuration = 99999.f;
		GameState->WarmupCountdownStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		GameMode->WarmupCountdownDuration = 99999.f;

		Vehicles::SpawnVehicles();
		//SpawnLlamas();

		SetConsoleTitleA("FortMP 17.30 | Listening | Can Join: True");
	}

	return GameMode->AlivePlayers.Num() > 0;
}

void GameMode::HandleNewSafeZonePhaseHook(AFortGameModeAthena* GameMode, int32 ZoneIndex)
{
	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GameMode->GameState);
	if (!GameState) return HandleNewSafeZonePhase(GameMode, ZoneIndex);

	AFortAthenaMapInfo* MapInfo = GameState->MapInfo;
	if (!MapInfo) return HandleNewSafeZonePhase(GameMode, ZoneIndex);

	auto& WaitTimes = MapInfo->SafeZoneDefinition.WaitTimes();
	auto& Durations = MapInfo->SafeZoneDefinition.Durations();

	static bool bSetup = false;
	static int LategameSafeZonePhase = 2;

	if (!bSetup) {
		static UCurveTable* GameData = Utils::StaticLoadObject<UCurveTable>("/Game/Athena/Balance/DataTables/AthenaGameData.AthenaGameData");

		static FName ShrinkTimeName = UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.ShrinkTime");
		static FName WaitTimeName = UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.WaitTime");

		for (int32 i = 0; i < WaitTimes.Num(); i++)
		{
			float Out;
			EEvaluateCurveTableResult Res;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, WaitTimeName, i, &Res, &Out, FString());
			WaitTimes[i] = Out;
		}
		for (int32 i = 0; i < Durations.Num(); i++)
		{
			float Out;
			EEvaluateCurveTableResult Res;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, ShrinkTimeName, i, &Res, &Out, FString());
			Durations[i] = Out;
		}

		bSetup = true;
	}

	static auto Accolade = Utils::StaticLoadObject<UFortAccoladeItemDefinition>("/BattlePassPermanentQuests/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle");
	for (auto PC : GameMode->AlivePlayers)
	{
		XP::Accolades::GiveAccolade(PC, Accolade, nullptr, EXPEventPriorityType::NearReticle);
		bool bruh;
		FGameplayTagContainer Empty{};
		FGameplayTagContainer Empty2{};
		XP::Challanges::SendStatEvent(PC->GetQuestManager(ESubGame::Athena), nullptr, Empty, Empty2, &bruh, &bruh, 1, EFortQuestObjectiveStatEvent::StormPhase);
	}

	if (bLategame)
	{
		GameMode->SafeZonePhase = LategameSafeZonePhase;
		GameState->SafeZonePhase = LategameSafeZonePhase;
		HandleNewSafeZonePhase(GameMode, ZoneIndex);
		LategameSafeZonePhase++;
	}
	else
	{
		//HandleNewSafeZonePhase(GameMode, ZoneIndex);

		//ZoneIndex = 4;
		//GameMode->SafeZonePhase = 4;
		//GameState->SafeZonePhase = 4;
		//GameState->OnRep_SafeZonePhase();
		HandleNewSafeZonePhase(GameMode, ZoneIndex);
		GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + WaitTimes[GameMode->SafeZonePhase];
		GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Durations[GameMode->SafeZonePhase];

	}

	float WaitTime = (GameMode->SafeZonePhase >= 0 && GameMode->SafeZonePhase < WaitTimes.Num()) ? WaitTimes[GameMode->SafeZonePhase] : 0.f;
	float Duration = (GameMode->SafeZonePhase >= 0 && GameMode->SafeZonePhase < Durations.Num()) ? Durations[GameMode->SafeZonePhase] : 0.f;

	GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = GameState->GetServerWorldTimeSeconds() + WaitTime;
	GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;

	static FVector ZoneLocation = GameMode->SafeZoneLocations[4];
	static FVector_NetQuantize100 ZoneLocationQuantize{ ZoneLocation.X, ZoneLocation.Y, ZoneLocation.Z };

	switch (GameMode->SafeZonePhase)
	{
		if (bLategame == false)
			return;
	case 2:
	case 3:
		GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = GameState->GetServerWorldTimeSeconds();
		GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameState->GetServerWorldTimeSeconds() + 0.3f;
		break;
	case 4:
		GameMode->SafeZoneIndicator->NextCenter = ZoneLocationQuantize;
		GameMode->SafeZoneIndicator->NextRadius = 10000.f;
		GameMode->SafeZoneIndicator->LastRadius = 20000.f;
		break;
	case 5:
		GameMode->SafeZoneIndicator->NextCenter = ZoneLocationQuantize;
		GameMode->SafeZoneIndicator->NextRadius = 5000.f;
		GameMode->SafeZoneIndicator->LastRadius = 10000.f;
		break;
	case 6:
		GameMode->SafeZoneIndicator->NextCenter = ZoneLocationQuantize;
		GameMode->SafeZoneIndicator->NextRadius = 2500.f;
		GameMode->SafeZoneIndicator->LastRadius = 5000.f;
		break;
	case 7:
		GameMode->SafeZoneIndicator->NextCenter = ZoneLocationQuantize;
		GameMode->SafeZoneIndicator->NextRadius = 1650.2f;
		GameMode->SafeZoneIndicator->LastRadius = 2500.f;
		break;
	case 8:
		GameMode->SafeZoneIndicator->NextCenter = ZoneLocationQuantize;
		GameMode->SafeZoneIndicator->NextRadius = 1090.12f;
		GameMode->SafeZoneIndicator->LastRadius = 1650.2f;
		break;
	}

	std::cout << "SafeZonePhase: " << GameMode->SafeZonePhase << std::endl;
}

void OnAircraftEnteredDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* Aircraft)
{
	auto GameState = Cast<AFortGameStateAthena>(GameMode->GameState);
	if (!GameState)
		return;

	std::cout << __FUNCTION__ << std::endl;

	if (bLategame)
	{
		FVector AircraftLocation = Aircraft->K2_GetActorLocation();
		FRotator AircraftRotation = Aircraft->K2_GetActorRotation();
		float Pitch = AircraftRotation.Pitch * (3.14159265359f / 180.0f);
		float Yaw = AircraftRotation.Yaw * (3.14159265359f / 180.0f);

		FVector AircraftForward{};
		AircraftForward.X = cos(Yaw) * cos(Pitch);
		AircraftForward.Y = sin(Yaw) * cos(Pitch);
		AircraftForward.Z = sin(Pitch);

		float Length = sqrt(AircraftForward.X * AircraftForward.X +
			AircraftForward.Y * AircraftForward.Y +
			AircraftForward.Z * AircraftForward.Z);

		if (Length > 0.0f) {
			AircraftForward.X /= Length;
			AircraftForward.Y /= Length;
			AircraftForward.Z /= Length;
		}

		FVector SafeZoneCenter = GameMode->SafeZoneLocations[4];
		SafeZoneCenter.Z += 15000;

		FVector NewLocation = SafeZoneCenter;

		Aircraft->K2_SetActorLocation(NewLocation, false, nullptr, true);

		auto& FlightInfo = Aircraft->FlightInfo;

		FlightInfo.FlightStartLocation = FVector_NetQuantize100(NewLocation);
		FlightInfo.FlightSpeed = 2000;
		FlightInfo.TimeTillDropStart = 2.0f;
		FlightInfo.TimeTillFlightEnd = 5.0f;

		Aircraft->FlightInfo = FlightInfo;

		GameState->bGameModeWillSkipAircraft = true;

		UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"skipaircraft", nullptr);
		GameState->SafeZonesStartTime = 1;
	}
	else
	{
		printf("No lategame jump!\n");
		GameState->bGameModeWillSkipAircraft = false;  // Asegúrate de que no se salta automáticamente
	}

	GameState->bAircraftIsLocked = false;  // Esto sí se puede mantener fuera, para permitir el salto manual
}




void GameMode::Hook()
{
    void** GameModeVTable = AFortGameModeAthena::GetDefaultObj()->VTable;

	THook(SpawnDefaultPawnForHook, nullptr).VFT(GameModeVTable, Indexes::SpawnDefaultPawnFor);
	THook(ReadyToStartMatchHook, &ReadyToStartMatch).VFT(GameModeVTable, Indexes::ReadyToStartMatch);

	THook(HandleNewSafeZonePhaseHook, &HandleNewSafeZonePhase).MinHook(Addresses::HandleNewSafeZonePhase);

	MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x4780d4c), OnAircraftEnteredDropZone, nullptr);
}
