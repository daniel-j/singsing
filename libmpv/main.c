// Build with: gcc -o main main.c fpscounter.c `pkg-config --libs --cflags mpv sdl2` -std=c99

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include "fpscounter.h"

static Uint32 wakeup_on_mpv_redraw, wakeup_on_mpv_events;

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
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

int main(int argc, char *argv[]) {
    char *videoFile = "";
    char *audioFile = "";
    if (argc < 2) {
        printf("usage: <audio/video file/url> [audio file/url]\n");
        videoFile = "https://www.youtube.com/watch?v=JqrQ6lSOYxI";
    } else {
        videoFile = argv[1];
    }

    mpv_handle *mpv = mpv_create();
    if (!mpv) {
        die("context init failed");
    }

    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "msg-level", "all=v");
    mpv_set_option_string(mpv, "hwdec", "auto");
    // mpv_set_option_string(mpv, "video-sync", "display-resample");
    mpv_set_option_string(mpv, "video-timing-offset", "0.0");

    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0) {
        die("mpv init failed");
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        die("SDL init failed");
    }

    SDL_Window *window =
        SDL_CreateWindow("hi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         1000, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                                    SDL_WINDOW_RESIZABLE);
    if (!window) {
        die("failed to create SDL window");
    }

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    if (!glcontext) {
        die("failed to create SDL GL context");
    }

    // vsync
    SDL_GL_SetSwapInterval(1);

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &(mpv_opengl_init_params){
            .get_proc_address = get_proc_address_mpv,
        }},
        {0}
    };

    // This makes mpv use the currently set GL context. It will use the callback
    // (passed via params) to resolve GL builtin functions, as well as extensions.
    mpv_render_context *mpv_gl;
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
        die("could not register events");
    }

    // When normal mpv events are available.
    mpv_set_wakeup_callback(mpv, on_mpv_events, NULL);

    // When a new frame should be drawn with mpv_opengl_cb_draw().
    // (Separate from the normal event handling mechanism for the sake of
    //  users which run OpenGL on a different thread.)
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_redraw, NULL);
    
    // Play this file. Note that this starts playback asynchronously.
    mpv_command(mpv, (const char *[]){"loadfile", videoFile, NULL});

    if (argc >= 3) {
        audioFile = argv[2];
        mpv_set_option_string(mpv, "audio-files", audioFile);
    }

    SDL_Event event;
    fpsinit();

    while (1) {
        int redraw = 0;
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
            case SDL_QUIT:
                goto done;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
                    redraw = 1;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_SPACE)
                    mpv_command_string(mpv, "cycle pause");
                break;
            default:
                // Happens when a new video frame should be rendered, or if the
                // current frame has to be redrawn e.g. due to OSD changes.
                if (event.type == wakeup_on_mpv_redraw) {
                    redraw = 1;
                }
                // Happens when at least 1 new event is in the mpv event queue.
                else if (event.type == wakeup_on_mpv_events) {
                    // Handle all remaining mpv events.
                    while (1) {
                        mpv_event *mp_event = mpv_wait_event(mpv, 0);
                        if (mp_event->event_id == MPV_EVENT_NONE)
                            break;
                        printf("event: %s\n", mpv_event_name(mp_event->event_id));
                        if (strcmp(mpv_event_name(mp_event->event_id), "end-file") == 0) {
                            goto done;
                        }
                    }
                }
            }
        }

        redraw = 1;

        if (redraw) {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            mpv_render_param params[] = {
                // Specify the default framebuffer (0) as target. This will
                // render onto the entire screen. If you want to show the video
                // in a smaller rectangle or apply fancy transformations, you'll
                // need to render into a separate FBO and draw it manually.
                {MPV_RENDER_PARAM_OPENGL_FBO, &(mpv_opengl_fbo){
                    .fbo = 0,
                    .w = w,
                    .h = h,
                }},
                // Flip rendering (needed due to flipped GL coordinate system).
                {MPV_RENDER_PARAM_FLIP_Y, &(int){1}},
                // {MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, &(int){0}},
                {0}
            };
            // See render_gl.h on what OpenGL environment mpv expects, and
            // other API details.
            
            mpv_render_context_render(mpv_gl, params);
        }
        SDL_GL_SwapWindow(window);
        float framespersecond = fpsupdate();
        // printf("fps %d\n", (int)framespersecond);
    }
done:

    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.
    mpv_render_context_free(mpv_gl);

    mpv_terminate_destroy(mpv);
    return 0;
}
