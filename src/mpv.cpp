
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <SDL2/SDL.h>

#include "mpv.hpp"


static void die(const std::string& msg) {
    std::cerr << msg << std::endl;
    exit(1);
}

static void *get_proc_address_mpv(void *fn_ctx, const char *name) {
    return SDL_GL_GetProcAddress(name);
}

MPV::MPV() {

}
MPV::~MPV() {
    if (mpv_gl) {
        mpv_render_context_free(mpv_gl);
        mpv_gl = nullptr;
    }
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = nullptr;
    }
}

void MPV::on_mpv_events(void *ctx) {
    auto self = (MPV*)ctx;
    SDL_Event event{.type = self->wakeup_on_mpv_events};
    SDL_PushEvent(&event);
}

void MPV::on_mpv_redraw(void *ctx) {
    auto self = (MPV*)ctx;
    SDL_Event event{.type = self->wakeup_on_mpv_redraw};
    SDL_PushEvent(&event);
}

int MPV::init(bool useLibmpvAudio) {
    if (mpv) die("mpv already initialized");

    mpv = mpv_create();
    if (!mpv) {
        die("mpv create failed");
    }

    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "msg-level", "all=info");
    mpv_set_option_string(mpv, "hwdec", "auto");
    mpv_set_option_string(mpv, "video-sync", "audio");
    mpv_set_option_string(mpv, "video-timing-offset", "0.0");
    mpv_set_option_string(mpv, "audio-client-name", "singsing");
    mpv_set_option_string(mpv, "video-latency-hacks", "yes");
    mpv_set_option_string(mpv, "osd-level", "0");
    mpv_set_option_string(mpv, "panscan", "1.0"); // fill view and crop
    if (useLibmpvAudio) {
        mpv_set_option_string(mpv, "ao", "libmpv");
        mpv_set_option_string(mpv, "ao-libmpv-format", "float");
        mpv_set_option_string(mpv, "ao-libmpv-samplerate", "48000");
        mpv_set_option_string(mpv, "ao-libmpv-channel-layouts", "stereo");
    }

    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0) {
        die("mpv init failed");
    }

    mpv_opengl_init_params gl_init_params{ .get_proc_address = get_proc_address_mpv };
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    // This makes mpv use the currently set GL context. It will use the callback
    // (passed via params) to resolve GL builtin functions, as well as extensions.
    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0) {
        die("failed to initialize mpv GL context");
    }

    // We use events for thread-safe notification of the SDL main loop.
    // Generally, the wakeup callbacks (set further below) should do as least
    // work as possible, and merely wake up another thread to do actual work.
    // On SDL, waking up the mainloop is the ideal course of action. SDL's
    // SDL_PushEvent() is thread-safe, so we use that.
    wakeup_on_mpv_redraw = SDL_RegisterEvents(1);
    wakeup_on_mpv_events = SDL_RegisterEvents(1);
    if (wakeup_on_mpv_redraw == (Uint32)-1 || wakeup_on_mpv_events == (Uint32)-1) {
        die("could not register mpv events");
    }

    // When normal mpv events are available.
    mpv_set_wakeup_callback(mpv, MPV::on_mpv_events, this);

    // When a new frame should be drawn with mpv_opengl_cb_draw().
    // (Separate from the normal event handling mechanism for the sake of
    //  users which run OpenGL on a different thread.)
    mpv_render_context_set_update_callback(mpv_gl, MPV::on_mpv_redraw, this);
    return 0;
}

void MPV::processSDLEvent(SDL_Event* event) const {
    switch (event->type) {
    case SDL_WINDOWEVENT:
        if (event->window.event == SDL_WINDOWEVENT_EXPOSED) {
        }
        break;
    case SDL_KEYDOWN:
        if (event->key.keysym.sym == SDLK_SPACE) {
            mpv_command_string(mpv, "cycle pause");
        }
        break;
    default:
        // Happens when a new video frame should be rendered, or if the
        // current frame has to be redrawn e.g. due to OSD changes.
        if (event->type == wakeup_on_mpv_redraw) {
        }
        // Happens when at least 1 new event is in the mpv event queue.
        else if (event->type == wakeup_on_mpv_events) {
            // Handle all remaining mpv events.
            while (true) {
                mpv_event *mp_event = mpv_wait_event(mpv, 0);
                if (mp_event->event_id == MPV_EVENT_NONE) break;

                std::string event_name = mpv_event_name(mp_event->event_id);
                std::cout << "mpv event: " << event_name << std::endl;
                if (event_name == "end-file") {
                    // playback stopped
                }
            }
        }
    }
}

int MPV::setProperty(const char* name, double value) const {
    return mpv_set_property(mpv, name, MPV_FORMAT_DOUBLE, &value);
}
int MPV::setProperty(const char* name, const char* value) const {
    return mpv_set_property(mpv, name, MPV_FORMAT_STRING, &value);
}

int MPV::setOption(const char* name, double value) const {
    return mpv_set_option(mpv, name, MPV_FORMAT_DOUBLE, &value);
}
int MPV::setOption(const char* name, const char* value) const {
    return mpv_set_option_string(mpv, name, value);
}

void MPV::play(std::string videoFile, std::string audioFile) const {
    // Play this file. Note that this starts playback asynchronously.
    const char *command[]{"loadfile", videoFile.c_str(), NULL};
    mpv_command(mpv, command);

    if (!audioFile.empty()) {
        mpv_set_option_string(mpv, "audio-files", audioFile.c_str());
    }
}

void MPV::render(int width, int height, int fbo, int format, bool skip_rendering) const {
    mpv_opengl_fbo mpfbo{
        .fbo = fbo,
        .w = width,
        .h = height,
        .internal_format = format
    };
    int flip_y{0};
    int skip_render{skip_rendering};
    mpv_render_param params[]{
        // Specify the default framebuffer (0) as target. This will
        // render onto the entire screen. If you want to show the video
        // in a smaller rectangle or apply fancy transformations, you'll
        // need to render into a separate FBO and draw it manually.
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        // Flip rendering (needed due to flipped GL coordinate system).
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_SKIP_RENDERING, &skip_render},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    // See render_gl.h on what OpenGL environment mpv expects, and
    // other API details.
    mpv_render_context_render(mpv_gl, params);
}

void MPV::reportSwap() const {
    mpv_render_context_report_swap(mpv_gl);
}

double MPV::getProgressPercent() const {
    double progress = 0.0;
    mpv_get_property(mpv, "percent-pos", MPV_FORMAT_DOUBLE, &progress);
    return progress / 100.0;
}

int MPV::readAudioBuffer(void* buffer, int length) const {
    if (!mpv) return 0;
    return mpv_audio_callback(mpv, buffer, length);
}

std::string MPV::getAudioFormat() const {
    auto result = mpv_get_property_string(mpv, "audio-out-params/format");
    if (!result) {
        return "";
    }
    return result;
}
