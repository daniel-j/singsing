#include "song.hpp"

#include <fstream>
#include <iostream>

Song::Song() {

}

const int Song::parse(const std::string &path) {

    std::ifstream filein(path);

    auto finishedHeaders = false;
    auto endFound = false;

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
                metadata.gap = std::stol(value);
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
            int startBeat, length;

            switch (tag) {
                case 'E':
                    endFound = true;
                    break;
                case 'P':
                    // trimChars
                    std::cout << "Got P: " << line << std::endl;
                    break;
                case ':':
                case '*':
                case 'F':
                    auto pos1 = line.find(' ');
                    startBeat = std::stoi(line.substr(0, pos1));
                    auto pos2 = line.find(' ', pos1 + 1);
                    length = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
                    pos1 = line.find(' ', pos2 + 1);
                    int pitch = std::stoi(line.substr(pos2 + 1, pos1 - pos2 - 1));
                    auto text = line.substr(pos1 + 1);
                    if (text.empty()) {
                        break;
                    }
                    // printf("type: %c, startBeat: %d, length: %d, pitch: %d, text: \"%s\"\n", tag, startBeat, length, pitch, text.c_str());
            }
        }
    }
    if (!finishedHeaders) {
        std::cerr << "Lyrics/Notes missing" << std::endl;
        return 0;
    }
    return 0;
}
