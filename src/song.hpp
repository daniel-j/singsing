#pragma once

#include <string>

struct SongMetadata {
	std::string encoding;
	std::string title;
	std::string artist;
	std::string creator;
	std::string genre;
	std::string language;
	std::string edition;
	std::string audioFile; // MP3
	std::string coverFile;
	std::string backgroundFile;
	std::string videoFile;
	std::string comment;
	std::string updated;
	float bpm = 0;
	long gap = 0;
};

class Song {
public:
	Song();
	//~Song();

	// getters
	const int parse(const std::string& path);
	const std::string getTitle() const { return metadata.title; }
	const std::string getArtist() const { return metadata.artist; }
	const std::string getAudioFile() const { return metadata.audioFile; }
	const std::string getVideoFile() const { return metadata.videoFile; }
	const float getBPM() const { return metadata.bpm; }
	const long getGap() const { return metadata.gap; }

	// methods
	const float timeToBeat(float time) const {
		return metadata.bpm * (time - metadata.gap / 1000.0) / 60.0;
	}
private:
	// metadata
	SongMetadata metadata;
};