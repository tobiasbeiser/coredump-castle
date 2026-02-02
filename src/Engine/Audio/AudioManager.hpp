#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <filesystem>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>

struct MixMixerDeleter {
	void operator()(MIX_Mixer *mixer) const
	{
		if (mixer)
			MIX_DestroyMixer(mixer);
	}
};

struct MixAudioDeleter {
	void operator()(MIX_Audio *audio) const
	{
		if (audio)
			MIX_DestroyAudio(audio);
	}
};

struct MixTrackDeleter {
	void operator()(MIX_Track *track) const
	{
		if (track)
			MIX_DestroyTrack(track);
	}
};

class AudioManager {
  public:
	AudioManager();
	~AudioManager();

	AudioManager(const AudioManager &) = delete;
	AudioManager &operator=(const AudioManager &) = delete;
	AudioManager(AudioManager &&) = delete;
	AudioManager &operator=(AudioManager &&) = delete;

	void playMusic(const std::filesystem::path &filePath, int fadeMs = 0);

	void stopMusic(int fadeOutMs = 500) const;

	void pauseMusic() const;
	void resumeMusic() const;

	void playSound(const std::filesystem::path &filePath, float volume = 1.0f);

	void loadSoundGroup(const std::string &groupId, const std::filesystem::path &folderPath);
	void playGroup(const std::string &groupId);

	void setMasterVolume(float volume) const;
	void setMusicVolume(float volume) const;

	void setAudioDisabled(bool disabled) { isAudioDisabled = disabled; }

  private:
	bool isAudioDisabled = true;
	MIX_Audio *loadAudio(const std::filesystem::path &filePath);
	std::map<std::string, std::vector<MIX_Audio *>> soundGroups;
	mutable std::mt19937 rng{std::random_device{}()};
	std::unique_ptr<MIX_Mixer, MixMixerDeleter> mixer;

	std::unique_ptr<MIX_Track, MixTrackDeleter> musicTrack;

	std::unordered_map<std::string, std::unique_ptr<MIX_Audio, MixAudioDeleter>> audioCache;
	std::filesystem::path basePath = "assets/audio/";
};