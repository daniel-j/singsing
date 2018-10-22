#include "song.hpp"

#include <fstream>
#include <iostream>

std::ostream& operator<<(std::ostream& out, const SongNote& note) {
    return out << "type=" << note.type << ", beat=" << note.beat
               << ", duration=" << note.duration << ", tone=" << note.tone
               << ", lyric=" << note.lyric << "";
}

Song::Song() {

}

const int Song::parse(const std::string &path) {

    std::ifstream filein(path);

    auto finishedHeaders = false;
    auto endFound = false;

    notes.clear();
    sentenceBreaks.clear();
    hasRap = false;
    isDuet = false;
    metadata.relative = false;

    for (std::string line; std::getline(filein, line); ) {
        if (!line.empty() && *line.rbegin() == '\r') {
            line.erase( line.length() - 1, 1);
        }
        auto len = line.length();
        if (len == 0 || line[0] == ' ' || (finishedHeaders && line[0] == '#')) {
            std::cerr << "Invalid linestart found in " << path << " :: \"" << line << "\". Aborting." << std::endl;
            return 0;
        }
        if (!finishedHeaders && line[0] == '#') {
            auto pos = line.find(':');
            if (pos == std::string::npos) {
                std::cerr << "Missing : in header \"" << line << "\"" << std::endl;
                return 0;
            }
            auto identifier = line.substr(1, pos - 1);
            auto value = line.substr(pos + 1);
            if (value.empty()) {
                std::cerr << "Empty value in header \"" << line << "\"" << std::endl;
                return 0;
            }

            if (identifier == "ENCODING") {
                metadata.encoding = value;
            } else if (identifier == "TITLE") {
                metadata.title = value;
            } else if (identifier == "ARTIST") {
                metadata.artist = value;
            } else if (identifier == "CREATOR") {
                metadata.creator = value;
            } else if (identifier == "GENRE") {
                metadata.genre = value;
            } else if (identifier == "YEAR") {
                metadata.year = std::stoi(value);
            } else if (identifier == "LANGUAGE") {
                metadata.language = value;
            } else if (identifier == "EDITION") {
                metadata.edition = value;
            } else if (identifier == "MP3") {
                metadata.audioFile = value;
            } else if (identifier == "COVER") {
                metadata.coverFile = value;
            } else if (identifier == "BACKGROUND") {
                metadata.backgroundFile = value;
            } else if (identifier == "VIDEO") {
                metadata.videoFile = value;
            } else if (identifier == "COMMENT") {
                metadata.comment = value;
            } else if (identifier == "UPDATED") {
                metadata.updated = value;
            } else if (identifier == "BPM") {
                metadata.bpm = std::stof(value);
            } else if (identifier == "GAP") {
                metadata.gap = std::stol(value) / 1000.0;
            } else if (identifier == "VIDEOGAP") {
                metadata.videoGap = std::stod(value);
            } else if (identifier == "RELATIVE") {
                metadata.relative = value == "YES";
            } else if (identifier == "PREVIEWSTART") {
                metadata.previewStart = std::stof(value);
            } else {
                std::cout << identifier << " " << value << std::endl;
            }
        } else {
            if (!finishedHeaders) {
                finishedHeaders = true;
            }

            auto tag = line[0];
            line = (len >= 2 && line[1] == ' ') ? line.substr(2) : line.substr(1);
            // std::cout << tag << " " << line << std::endl;
            int beat, duration;
            SongNoteType noteType = SongNoteTypeNormal;
            bool isNote = false;
            int pos1;

            switch (tag) {
                case 'E':
                    std::cout << std::endl;
                    endFound = true;
                    break;
                case 'P':
                    // trimChars
                    std::cout << "Got P: " << line << std::endl;
                    isDuet = true;
                    break;
                case '-':
                    pos1 = line.find(' ');
                    beat = std::stoi(line.substr(0, pos1));
                    sentenceBreaks.push_back(beat);
                    std::cout << std::endl;
                    break;
                case ':':
                    noteType = SongNoteTypeNormal;
                    isNote = true;
                    break;
                case '*':
                    noteType = SongNoteTypeGolden;
                    isNote = true;
                    break;
                case 'F':
                    noteType = SongNoteTypeFreestyle;
                    isNote = true;
                    break;
                case 'R':
                    noteType = SongNoteTypeRap;
                    hasRap = true;
                    isNote = true;
                    break;
                case 'G':
                    noteType = SongNoteTypeRapGolden;
                    hasRap = true;
                    isNote = true;
                    break;
            }

            if (isNote) {
                pos1 = line.find(' ');
                beat = std::stoi(line.substr(0, pos1));
                auto pos2 = line.find(' ', pos1 + 1);
                duration = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
                pos1 = line.find(' ', pos2 + 1);
                int tone = std::stoi(line.substr(pos2 + 1, pos1 - pos2 - 1));
                auto lyric = line.substr(pos1 + 1);
                if (lyric.empty()) {
                    break;
                }
                SongNote note{
                    .type = noteType,
                    .tone = tone,
                    .beat = beat,
                    .duration = duration,
                    .lyric = lyric
                };
                notes.push_back(note);
                //std::cout << note << std::endl;
                std::cout << lyric;
            }
        }
    }
    if (!finishedHeaders) {
        std::cerr << "Lyrics/Notes missing" << std::endl;
        return 0;
    }
    if (!endFound) {
        std::cerr << "No end found in notes file" << std::endl;
        return 0;
    }
    return 0;
}
