#pragma once

#include <string>
#include <vector>

struct SongMetadata {
	std::string encoding;
	std::string title;
	std::string artist;
	std::string creator;
	std::string genre;
	int year = 0;
	std::string language;
	std::string edition;
	std::string audioFile; // MP3 field
	std::string coverFile;
	std::string backgroundFile;
	std::string videoFile;
	std::string comment;
	std::string updated;
	float bpm = 0;
	double gap = 0;
	double videoGap = 0;
	bool relative = false;
	float previewStart = 0;
};

enum SongNoteType {
	SongNoteTypeNormal,
	SongNoteTypeGolden,
	SongNoteTypeFreestyle,
	SongNoteTypeRap,
	SongNoteTypeRapGolden
};

struct SongNote {
	SongNoteType type;
	int tone;
	int beat;
	int duration;
	std::string lyric;
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
	const double getGap() const { return metadata.gap; }
	const double getVideoGap() const { return metadata.videoGap; }

	// methods
	const float timeToBeat(float time) const {
		return metadata.bpm * (time - metadata.gap) / 60.0;
	}
	const float beatToTime(float beat) const {
		return metadata.gap + beat * 60.0 / metadata.bpm;
	}
private:
	// metadata
	SongMetadata metadata;
	std::vector<SongNote> notes;
	std::vector<int> sentenceBreaks;
	bool hasRap = false;
	bool isDuet = false;
};
