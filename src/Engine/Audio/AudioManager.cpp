#include "AudioManager.hpp"
#include <iostream>

AudioManager::AudioManager()
{
	SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "5");

	if (!MIX_Init()) {
		SDL_Log("Couldn't init SDL_mixer library: %s", SDL_GetError());
	}

	mixer.reset(MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL));
	if (!mixer) {
		SDL_Log("Couldn't create mixer on default device: %s", SDL_GetError());
	}

	musicTrack.reset(MIX_CreateTrack(mixer.get()));
	if (!musicTrack) {
		throw std::runtime_error("Failed to create Music Track: " + std::string(SDL_GetError()));
	}
}

AudioManager::~AudioManager()
{
	if (mixer) {
		MIX_StopAllTracks(mixer.get(), 0);
	}

	MIX_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

MIX_Audio *AudioManager::loadAudio(const std::filesystem::path &filePath)
{
	const std::string key = filePath.string();

	if (const auto it = audioCache.find(key); it != audioCache.end()) {
		return it->second.get();
	}

	MIX_Audio *rawAudio = MIX_LoadAudio(mixer.get(), key.c_str(), true);

	if (!rawAudio) {
		std::cerr << "Failed to load audio: " << key << " Error: " << SDL_GetError() << std::endl;
		return nullptr;
	}

	audioCache[key] = std::unique_ptr<MIX_Audio, MixAudioDeleter>(rawAudio);
	return audioCache[key].get();
}

void AudioManager::playMusic(const std::filesystem::path &filePath, const int fadeMs)
{
	MIX_Audio *audio = loadAudio(filePath);
	if (!audio)
		return;

	if (!MIX_SetTrackAudio(musicTrack.get(), audio)) {
		std::cerr << "Failed to set track audio: " << SDL_GetError() << std::endl;
		return;
	}

	const SDL_PropertiesID options = SDL_CreateProperties();
	if (!options) {
		SDL_Log("Couldn't create play options: %s", SDL_GetError());
	}
	SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
	SDL_SetNumberProperty(options, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, fadeMs);

	MIX_PlayTrack(musicTrack.get(), options);
}

void AudioManager::stopMusic(const int fadeOutMs) const
{
	if (musicTrack) {
		const Sint64 fadeFrames = MIX_TrackMSToFrames(musicTrack.get(), fadeOutMs);

		MIX_StopTrack(musicTrack.get(), fadeFrames);
	}
}

void AudioManager::pauseMusic() const
{
	if (musicTrack) {
		MIX_PauseTrack(musicTrack.get());
	}
}

void AudioManager::resumeMusic() const
{
	if (musicTrack) {
		MIX_ResumeTrack(musicTrack.get());
	}
}

void AudioManager::playSound(const std::filesystem::path &filePath, float volume)
{
	MIX_Audio *audio = loadAudio(filePath);
	if (!audio)
		return;
	MIX_PlayAudio(mixer.get(), audio);
}

void AudioManager::setMasterVolume(const float volume) const
{
	if (mixer) {
		MIX_SetMixerGain(mixer.get(), volume);
	}
}

void AudioManager::setMusicVolume(const float volume) const
{
	if (musicTrack) {
		MIX_SetTrackGain(musicTrack.get(), volume);
	}
}