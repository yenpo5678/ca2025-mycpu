#include "doomdef.h"
#include "i_sound.h"

void I_InitSound() {}
void I_SubmitSound() {}
void I_ShutdownSound() {}
void I_SetChannels() {}


void I_StartSound(void *origin, int id) {}


void I_PlaySong(void *data, int looping, int volume) {}

void I_StopSong(void) {}
void I_SetSfxVolume(int volume) {}
void I_SetMusicVolume(int volume) {}
void I_PauseSong(int handle) {}
void I_ResumeSong(int handle) {}
void I_UnRegisterSong(int handle) {}

int  I_RegisterSong(void *data) { return 1; }

int  I_GetSfxLumpNum(sfxinfo_t* sfx) { return 0; }

void I_StopSound(int handle) {}
int  I_SoundIsPlaying(int handle) { return 0; }
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch) {}
void I_InitMusic(void) {}
void I_ShutdownMusic(void) {}