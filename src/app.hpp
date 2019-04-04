#pragma once

// this needs to be first
#include <glad/glad.h>

#include <SDL2/SDL.h>
#include <aubio/aubio.h>


class App {
 private:
    SDL_Window* mainWindow = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GLContext glctx = nullptr;
    void initAudio();
    int initGL();
 public:
    aubio_pitch_t** aubioPitchChannels = nullptr;
    App();
    ~App();
    int init();
    int launch();
    void analyzeAudio(const int& channel, const float * const buffer, float * const pitch, float * const probability);
};
