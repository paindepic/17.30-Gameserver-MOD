#include "../Public/Misc.h"

UFortPlaylistAthena* Misc::GetPlaylist()
{
	static UFortPlaylistAthena* Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");
	return Playlist;
}