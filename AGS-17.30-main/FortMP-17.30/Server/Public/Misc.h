#pragma once

#include "Framework.h"
#include "Utils.h"

namespace Misc
{
	namespace MatchState
	{
		static FName InProgress = UKismetStringLibrary::Conv_StringToName(L"InProgress");
		static FName WaitingPostMatch = UKismetStringLibrary::Conv_StringToName(L"WaitingPostMatch");
	}

	UFortPlaylistAthena* GetPlaylist();
}
