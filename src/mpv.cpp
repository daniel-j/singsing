
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <SDL2/SDL.h>

#include "mpv.hpp"

#include <mpv/client.h>
#include <mpv/render_gl.h>

static Uint32 wakeup_on_mpv_redraw, wakeup_on_mpv_events;
static mpv_handle *mpv = nullptr;
static mpv_render_context *mpv_gl = nullptr;

static void die(const std::string& msg) {
    std::cerr << msg << std::endl;
    exit(1);
}

static void *get_proc_address_mpv(void *fn_ctx, const char *name) {
    return SDL_GL_GetProcAddress(name);
}

static void on_mpv_events(void *ctx) {
    SDL_Event event = {.type = wakeup_on_mpv_events};
    SDL_PushEvent(&event);
}

static void on_mpv_redraw(void *ctx) {
    SDL_Event event = {.type = wakeup_on_mpv_redraw};
    SDL_PushEvent(&event);
}

int init_mpv() {
    if (mpv) die("mpv already initialized");

    mpv = mpv_create();
    if (!mpv) {
        die("mpv create failed");
    }

    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "msg-level", "all=status");
    mpv_set_option_string(mpv, "hwdec", "auto");
    // mpv_set_option_string(mpv, "video-sync", "display-resample");
    mpv_set_option_string(mpv, "video-timing-offset", "0.0");
    mpv_set_option_string(mpv, "audio-client-name", "singsing");
    mpv_set_option_string(mpv, "video-latency-hacks", "yes");
    mpv_set_option_string(mpv, "osd-level", "0");
    mpv_set_option_string(mpv, "panscan", "1.0"); // fill view and crop

    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0) {
        die("mpv init failed");
    }

    mpv_opengl_init_params gl_init_params{ .get_proc_address = get_proc_address_mpv };
    mpv_render_param params[] = {
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
    mpv_set_wakeup_callback(mpv, on_mpv_events, NULL);

    // When a new frame should be drawn with mpv_opengl_cb_draw().
    // (Separate from the normal event handling mechanism for the sake of
    //  users which run OpenGL on a different thread.)
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_redraw, NULL);
    return 0;
}

int mpv_process_sdl_event(SDL_Event* event) {
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
            while (1) {
                mpv_event *mp_event = mpv_wait_event(mpv, 0);
                if (mp_event->event_id == MPV_EVENT_NONE)
                    break;
                printf("event: %s\n", mpv_event_name(mp_event->event_id));
                if (strcmp(mpv_event_name(mp_event->event_id), "end-file") == 0) {
                    // playback stopped
                }
            }
        }
    }
    return 0;
}

void mpv_play(std::string videoFile, std::string audioFile) {
    // Play this file. Note that this starts playback asynchronously.
    const char *command[] = {"loadfile", videoFile.c_str(), NULL};
    mpv_command(mpv, command);

    if (!audioFile.empty()) {
        mpv_set_option_string(mpv, "audio-files", audioFile.c_str());
    }
}

void mpv_render(int width, int height, int fbo, int format, int skip_rendering) {
    mpv_opengl_fbo mpfbo{
        .fbo = fbo,
        .w = width,
        .h = height,
        .internal_format = format
    };
    int flip_y{0};
    mpv_render_param params[] = {
        // Specify the default framebuffer (0) as target. This will
        // render onto the entire screen. If you want to show the video
        // in a smaller rectangle or apply fancy transformations, you'll
        // need to render into a separate FBO and draw it manually.
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        // Flip rendering (needed due to flipped GL coordinate system).
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_SKIP_RENDERING, &skip_rendering},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    // See render_gl.h on what OpenGL environment mpv expects, and
    // other API details.
    mpv_render_context_render(mpv_gl, params);
}

void mpv_flip() {
    mpv_render_context_report_swap(mpv_gl);
}

int mpv_destroy() {
    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.
    if (mpv_gl) {
        mpv_render_context_free(mpv_gl);
        mpv_gl = nullptr;
    }

    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = nullptr;
    }
    return 0;
}
