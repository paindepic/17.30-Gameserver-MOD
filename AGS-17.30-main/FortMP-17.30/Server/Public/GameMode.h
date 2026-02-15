#pragma once

#include "Framework.h"

namespace GameMode
{
     inline bool (*ReadyToStartMatch)(AFortGameModeAthena*);
     inline void (*HandleNewSafeZonePhase)(AFortGameModeAthena*, int32);

     APawn* SpawnDefaultPawnForHook(AGameMode* GameMode, AController* NewPlayer, AActor* StartSpot);
     bool ReadyToStartMatchHook(AFortGameModeAthena* GameMode);
     void HandleNewSafeZonePhaseHook(AFortGameModeAthena* GameMode, int32 ZoneIndex);

     void Hook();
}