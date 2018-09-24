#pragma once

int init_mpv();
int mpv_process_sdl_event(SDL_Event* event);
void mpv_play(std::string videoFile = "https://www.youtube.com/watch?v=JqrQ6lSOYxI", std::string audioFile = "");
void mpv_render(int width, int height);
int mpv_destroy();
