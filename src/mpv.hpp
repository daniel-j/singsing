#pragma once

#include <string>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <SDL2/SDL.h>

#define MPV_ENABLE_DEPRECATED 0

class MPV {
public:
    MPV();
    ~MPV();
    int init();
    void processSDLEvent(SDL_Event* event) const;
    void play(std::string videoFile = "https://www.youtube.com/watch?v=JqrQ6lSOYxI", std::string audioFile = "") const;
    void render(int width, int height, int fbo = 0, int format = 0, bool skip_rendering = false) const;
    void reportSwap() const;
    double getProgressPercent() const;
    int readAudioBuffer(void* buffer, int length) const;
    std::string getAudioFormat() const;

private:
    mpv_handle *mpv = nullptr;
    mpv_render_context *mpv_gl = nullptr;
    Uint32 wakeup_on_mpv_redraw, wakeup_on_mpv_events;

    static void on_mpv_events(void *ctx);
    static void on_mpv_redraw(void *ctx);
};
