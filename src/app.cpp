
// this needs to be first
#define GL3_PROTOTYPES 1
#include <GL/glew.h>

#include "app.hpp"
#include <iostream>
#include <algorithm>    // std::min
#include <cmath>        // std::isnan
#include <chrono>
#ifdef __MINGW32__
    #include "lib/mingw.thread.hpp"
#else
    #include <thread>   // std::this_thread::sleep_for
#endif

#include <soundio/soundio.h>
//#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_ttf.h>
#include "util/fpscounter.hpp"
#include "util/glutils.hpp"
#include "util/glprogram.hpp"
#include "util/glframebuffer.hpp"
#include "mpv.hpp"
#include "song.hpp"
#include "font.hpp"
#include "musicvisualizer.hpp"

//#ifndef _WIN32
    #define SDL_AUDIO_OUTPUT 1
//#endif

const int MAX_FRAMES_PER_SECOND = 60;

const int ANALYSIS_BUFFER_LENGTH = 2048;
const int ANALYSIS_HOP_SIZE = ANALYSIS_BUFFER_LENGTH / 4;

static struct SoundIoRingBuffer *ring_buffer = nullptr;

//RingBuffer<float> ring(ANALYSIS_BUFFER_LENGTH + 1);
static float currentNote1 = 0.0;
static float currentNote2 = 0.0;
static float currentConfidence1 = 0.0;
static float currentConfidence2 = 0.0;
// static std::mutex audioMutex;

static struct SoundIo *soundio;
static struct SoundIoDevice *in_device;
static struct SoundIoInStream *instream;
static struct SoundIoDevice *out_device_sio;
static struct SoundIoOutStream *outstream = nullptr;
static SDL_AudioDeviceID out_device_sdl = 0;
static MPV* mpv = nullptr;
static std::vector<float> mpv_audio_buffer;
static MusicVisualizer mviz;

static std::string tones[]{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

/*
int recordCallback(const void *inputBuffer, void *outputBuffer,
                   unsigned long framesPerBuffer,
                   const PaStreamCallbackTimeInfo* timeInfo,
                   PaStreamCallbackFlags statusFlags,
                   void *userData) {
    auto app = (App*)userData;
    float threshold = 0.05f;

    for (int channel = 0; channel < 1; ++channel) {
        float *channelBuffer = ((float **) inputBuffer)[channel];
        for (int i = 0; i < framesPerBuffer; ++i) {
            ring.put(channelBuffer[i]);
        }
        if (ring.length() < ANALYSIS_BUFFER_LENGTH) continue;
        float amplitude = 0.0f;
        for (int i = 0; i < framesPerBuffer; ++i) {
            float volume = abs(channelBuffer[i]);
            if (volume > amplitude) {
                amplitude = volume;
            }
        }
        float pitch, probability, note, toneAccuracy;
        int tone;

        app->analyzeAudio(channel, channelBuffer, &pitch, &probability);
        static auto LOG2 = log(2.0);

        if (pitch > 0.0 && probability > 0.0 && amplitude >= threshold) {
            note = 12.0 * (log(pitch / 440.0) / LOG2) + 69.0;

            tone = (int)round(fmod(note, 12.0)) % 12;
            toneAccuracy = round(fmod(note, 12.0)) - fmod(note, 12.0);

            // std::cout << pitch << " " << tone << " " << toneOff << " " << tones[tone] << " " << probability << std::endl;
            std::cout << "Channel " << channel + 1 << ": " <<
                       round(pitch) << "\t" <<
                       tones[tone] << "\t";
            for (int i = 0; i < 12; ++i) {
              if (i != tone) {
                  std::cout << "-";
              } else {
                  std::cout << "#";
              }
            }
            std::cout << "\t";
            int a = round(-toneAccuracy * 10.0);
            for (int i = -5; i < 5; ++i) {
              if (i != a) {
                  std::cout << "-";
              } else if (i == a && i != 0) {
                  std::cout << "#";
              } else {
                  std::cout << "|";
              }
            }
            std::cout << " " << -toneAccuracy << "\t";
            a = ceil(probability * 10.0);
            for (int i = 0; i < 10; ++i) {
              if (i >= a) {
                  std::cout << "-";
              } else {
                  std::cout << "#";
              }
            }
            std::cout << " " << probability * 100 << std::endl;
            {
              // std::lock_guard<std::mutex> lock(audioMutex);
              currentNote = fmod(note, 12.0);
              currentConfidence = probability;
            }
        } else {
            currentNote = 100000;
        }



        auto HalftoneBase = 1.05946309436; // 2^(1/12) -> HalftoneBase^12 = 2 (one octave)
        auto BaseToneFreq = 65.4064;
        int NumHalfTones = 46;

        bool ToneValid = false;
        int Tone = -1;
        int ToneAbs = -1;

        // check if signal has an acceptable volume (ignore background-noise)
        if (amplitude >= threshold) {
          // analyse the current voice pitch
          float MaxWeight = -1.0;
          int MaxTone = 0;
          for (int ToneIndex = 0; ToneIndex < NumHalfTones; ++ToneIndex) {
            float CurFreq = BaseToneFreq * pow(HalftoneBase, ToneIndex);
            int SampleIndex = 0;
            int SamplesPerPeriod = round(44100/CurFreq);
            int CorrelatingSampleIndex = SampleIndex + SamplesPerPeriod;
            float AccumDist = 0.0f;
            while (CorrelatingSampleIndex < framesPerBuffer) {
                auto Dist = abs(channelBuffer[SampleIndex] - channelBuffer[CorrelatingSampleIndex]);
                AccumDist += Dist;
                SampleIndex++;
                CorrelatingSampleIndex++;
            }
            float CurWeight = 1.0 - AccumDist / (float)framesPerBuffer;
            if (CurWeight > MaxWeight) {
              // this frequency has a higher weight
              MaxWeight = CurWeight;
              MaxTone = ToneIndex;
            }
            // auto CurWeight = AnalyzeAutocorrelationFreq(CurFreq);
          }
          ToneAbs = MaxTone;
          Tone = MaxTone % 12;
          currentNote2 = Tone;
          // std::cout << (ToneAbs) << " " << tones[Tone] << std::endl;
        } else {
            currentNote2 = 10000;
        }
    }

    return paContinue;
}
*/
void App::analyzeAudio(const int& channel, const float* const buffer, float* const pitch, float* const probability) {
    float channelBuffer[ANALYSIS_HOP_SIZE];
    for (int i = 0; i < ANALYSIS_HOP_SIZE; ++i) {
        channelBuffer[i] = buffer[i * 2 + channel];
    }

    fvec_t buf{ ANALYSIS_HOP_SIZE, (float*)&channelBuffer };
    fvec_t out{ 1, pitch };
    aubio_pitch_do(aubioPitchChannels[channel], &buf, &out);
    *probability = aubio_pitch_get_confidence(aubioPitchChannels[channel]);
}

void checkRingbuffer(App* app, int bytes_per_frame) {
    float pitch1, probability1, note1;
    float pitch2, probability2, note2;

    float *read_ptr = (float*)soundio_ring_buffer_read_ptr(ring_buffer);

    app->analyzeAudio(0, read_ptr, &pitch1, &probability1);
    app->analyzeAudio(1, read_ptr, &pitch2, &probability2);

    soundio_ring_buffer_advance_read_ptr(ring_buffer, ANALYSIS_HOP_SIZE * bytes_per_frame);

    static auto LOG2 = log(2.0);
    note1 = 12.0 * (log(pitch1 / 440.0) / LOG2) + 69.0;
    currentNote1 = fmod(note1, 12.0);
    currentConfidence1 = probability1;

    note2 = 12.0 * (log(pitch2 / 440.0) / LOG2) + 69.0;
    currentNote2 = fmod(note2, 12.0);
    currentConfidence2 = probability2;

    if (pitch1 > 0.0 && probability1 > 0.1/* && amplitude >= threshold*/) {
        // tone = (int)round(fmod(note1, 12.0)) % 12;
        // toneAccuracy = round(fmod(note1, 12.0)) - fmod(note1, 12.0);

        //std::cout << pitch << " " << tone << " " << tones[tone] << " " << probability << std::endl;
    } else {
        //printf("%f\n", amplitude);
    }

    //fflush(stdout);
}

static void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max) {
    auto app = (App*)instream->userdata;
    struct SoundIoChannelArea *areas;

    char *write_ptr = soundio_ring_buffer_write_ptr(ring_buffer);
    int free_bytes = soundio_ring_buffer_free_count(ring_buffer);
    int free_count = free_bytes / instream->bytes_per_frame;

    if (frame_count_min > free_count) {
        std::cerr << "Ring buffer overflow " << frame_count_min << " " << free_count << std::endl;
    }

    int write_frames = std::min(free_count, frame_count_max);
    int frames_left = write_frames;

    // std::cout << frame_count_max << std::endl;

    // int frames_left = frame_count_max;

    for (;;) {
        int frame_count = frames_left;
        int err;

        if ((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
            fprintf(stderr, "begin read error: %s\n", soundio_strerror(err));
            return;
        }

        // printf("%d %d %d \n", frame_count, frame_count_min, frame_count_max);

        if (!frame_count) {
            break;
        }

        if (!areas) {
            memset(write_ptr, 0, frame_count * instream->bytes_per_frame);
            fprintf(stderr, "Dropped %d frames due to internal overflow\n", frame_count);
        } else {
            for (int frame = 0; frame < frame_count; frame += 1) {
                for (int ch = 0; ch < instream->layout.channel_count; ch++) {
                    memcpy(write_ptr, areas[ch].ptr, instream->bytes_per_sample);
                    areas[ch].ptr += areas[ch].step;
                    write_ptr += instream->bytes_per_sample;
                }
            }
        }

        if ((err = soundio_instream_end_read(instream))) {
            fprintf(stderr, "end read error: %s", soundio_strerror(err));
            return;
        }


        frames_left -= frame_count;
        if (frames_left <= 0) {
            break;
        }
    }

    int advance_bytes = write_frames * instream->bytes_per_frame;
    soundio_ring_buffer_advance_write_ptr(ring_buffer, advance_bytes);

    //std::cout << write_frames << " " << soundio_ring_buffer_fill_count(ring_buffer) / instream->bytes_per_frame << ">=" << ANALYSIS_HOP_SIZE << " ";

    while (soundio_ring_buffer_fill_count(ring_buffer) >= ANALYSIS_HOP_SIZE * instream->bytes_per_frame) {
        //std::cout << "analyze ";
        checkRingbuffer(app, instream->bytes_per_frame);
    }
    //std::cout << std::endl;
}

static void write_audio_callback(void*, Uint8* stream, int len) {
    SDL_memset(stream, 0, len);
    int frames_written = 0;
    if (mpv) frames_written = mpv->readAudioBuffer((void*)stream, len);
    // std::cout << len << " " << frames_written << std::endl;
    // std::cout << SDL_GetQueuedAudioSize(out_device_sdl) << std::endl;

    float volume = 0.0;
    const float* buf = (float*)stream;
    float monoBuffer[2048]{0};
    for (size_t i = 0; i < len / sizeof(float); i++) {
        if (abs(buf[i]) > volume) {
            volume = abs(buf[i]);
        }
        monoBuffer[i / 2] += buf[i] / 2.0;
    }
    mviz.do_fft(monoBuffer);
}

static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    struct SoundIoChannelArea *areas;
    int err;
    const struct SoundIoChannelLayout *layout = &outstream->layout;

    int frames_left = frame_count_max;
    int chunk_size = 1024;
    float chunk_buffer[chunk_size * 2];

    while (frames_left > 0) {
        int frame_count = fmax(frame_count_min, fmin(chunk_size, frames_left));
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "unrecoverable begin_write stream error: %s %d\n", soundio_strerror(err), frame_count);
            exit(1);
        }

        write_audio_callback(NULL, (Uint8*)&chunk_buffer, frame_count * sizeof(float) * layout->channel_count);

        for (int frame = 0; frame < frame_count; frame += 1) {
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                float sample = chunk_buffer[frame * layout->channel_count + channel];
                float *buf = (float*)areas[channel].ptr;
                *buf = sample;
                areas[channel].ptr += areas[channel].step;
            }
        }

        if ((err = soundio_outstream_end_write(outstream))) {
            if (err == SoundIoErrorUnderflow) {
                return;
            }
            fprintf(stderr, "unrecoverable end_write stream error: %s\n", soundio_strerror(err));
            exit(1);
        }
        frames_left -= frame_count;
    }

    return;

    mpv_audio_buffer.resize(frames_left * outstream->layout.channel_count);

    int mpv_frames = 0;

    if (mpv) mpv_frames = mpv->readAudioBuffer((void*)mpv_audio_buffer.data(), frames_left * outstream->layout.channel_count * 4);

    std::cout << mpv_frames << ", " << frame_count_min << " to " << frame_count_max << std::endl;

    if (mpv_frames > 0) {
        frames_left = fmin(frames_left, mpv_frames);
        std::cout << frames_left << std::endl;
    } else {

    }

    for (;;) {
        int frame_count = frames_left;
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }

        std::cout << frame_count << std::endl;

        if (!frame_count) {
            break;
        }

        const struct SoundIoChannelLayout *layout = &outstream->layout;

        for (int frame = 0; frame < frame_count; frame += 1) {
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                float sample = mpv_audio_buffer[frame * layout->channel_count + channel];
                float *buf = (float*)areas[channel].ptr;
                *buf = sample;
                areas[channel].ptr += areas[channel].step;
            }
        }

        if ((err = soundio_outstream_end_write(outstream))) {
            if (err == SoundIoErrorUnderflow)
                return;
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
        if (frames_left <= 0) {
            break;
        }
    }
}

static void overflow_callback(struct SoundIoInStream*) {
    static int count = 0;
    fprintf(stderr, "overflow %d\n", ++count);
}
static void underflow_callback(struct SoundIoOutStream*) {
    static int count = 0;
    fprintf(stderr, "underflow %d\n", ++count);
}

void App::initAudio() {
    // Workaround to fix bad audio from input devices
    // I have an issue with my ZOOM H1 USB mic together with
    // soundio, which doesn't seem to init properly on pulse.
    // This wakes up every pulse input device before soundio
    // starts which seems to do the trick. Another fix is to
    // keep pavucontrol open in the background.
    std::string sdlAudioDriver = SDL_GetCurrentAudioDriver();
    if (sdlAudioDriver == "pulseaudio" && false) {
        int count = SDL_GetNumAudioDevices(1);
        for (int i = 0; i < count; ++i) {
            auto name = SDL_GetAudioDeviceName(i, 1);
            std::cout << "Waking up audio input device " << name << std::endl;
            SDL_AudioSpec want, have;
            SDL_zero(want);
            want.format = AUDIO_F32;
            want.samples = 2048;
            auto dev = SDL_OpenAudioDevice(name, 1, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);
            if (dev) {
                SDL_PauseAudioDevice(dev, false);
                SDL_Delay(1000);
                SDL_PauseAudioDevice(dev, true);
                SDL_ClearQueuedAudio(dev);
                SDL_CloseAudioDevice(dev);
            }
        }
    }

    // soundio
    soundio = soundio_create();
    soundio->app_name = "singsing";
    int err = soundio_connect(soundio);
    if (err) {
        fprintf(stderr, "could not connect: %s\n", soundio_strerror(err));
        return;
    }
    soundio_flush_events(soundio);

#ifdef SDL_AUDIO_OUTPUT
    // SDL2 audio output
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 48000;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 2048;
    want.callback = write_audio_callback;
    out_device_sdl = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!out_device_sdl) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(out_device_sdl, false); // start audio output
#else
    auto output_count = soundio_output_device_count(soundio);
    auto default_output = soundio_default_output_device_index(soundio);
    for (int i = 0; i < output_count; i += 1) {
        auto device = soundio_get_output_device(soundio, i);
        // std::cout << device->id << " " << device->is_raw << std::endl;
        if (strcmp(device->id, "void") == 0) {
            // default_output = i;
        }
        soundio_device_unref(device);
    }

    out_device_sio = soundio_get_output_device(soundio, default_output);
    printf("Output device: %s (%s)\n", out_device_sio->name, out_device_sio->id);

    if (out_device_sio->probe_error) {
        fprintf(stderr, "Cannot probe device: %s\n", soundio_strerror(out_device_sio->probe_error));
        return;
    }

    soundio_device_sort_channel_layouts(out_device_sio);

    if (!soundio_device_supports_format(out_device_sio, SoundIoFormatFloat32NE)) {
        std::cerr << "Output device does not support float32 audio" << std::endl;
        soundio_device_unref(out_device_sio);
        out_device_sio = nullptr;
        return;
    }

    if(!soundio_device_supports_sample_rate(out_device_sio, 48000)) {
        std::cerr << "Sample rate not supported" << std::endl;
        return;
    }

    std::cout << "Current output sample rate: " << out_device_sio->sample_rate_current << std::endl;

    outstream = soundio_outstream_create(out_device_sio);

    if (soundio->current_backend != SoundIoBackendWasapi) {
        // this crashes wasapi
        outstream->name = soundio->app_name;
    }
    outstream->name = NULL;
    outstream->format = SoundIoFormatFloat32NE;
    outstream->sample_rate = soundio_device_nearest_sample_rate(out_device_sio, 48000);
    outstream->layout = *soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdStereo);
    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->userdata = this;
    // pulseaudio likes a high latency
    outstream->software_latency = 0.050; // 50 ms

    if ((err = soundio_outstream_open(outstream))) {
        fprintf(stderr, "Unable to open output stream: %s\n", soundio_strerror(err));
        soundio_outstream_destroy(outstream);
        outstream = nullptr;
        soundio_device_unref(out_device_sio);
        out_device_sio = nullptr;
        exit(1);
        return;
    }

    if (outstream && (err = soundio_outstream_start(outstream))) {
        fprintf(stderr, "unable to start output stream: %s\n", soundio_strerror(err));
        return;
    }
#endif

    // int input_count = soundio_input_device_count(soundio);
    int default_input = soundio_default_input_device_index(soundio);

    // printf("Input devices: %d\n", input_count);

    in_device = soundio_get_input_device(soundio, default_input);

    printf("Input device: %s (%s)\n", in_device->name, in_device->id);

    soundio_device_sort_channel_layouts(in_device);

    instream = soundio_instream_create(in_device);

    if (soundio->current_backend != SoundIoBackendWasapi) {
        // this crashes with wasapi
        instream->name = "Input #1";
    }
    instream->sample_rate = soundio_device_nearest_sample_rate(in_device, 48000);
    instream->format = SoundIoFormatFloat32NE;
    instream->read_callback = read_callback;
    instream->overflow_callback = overflow_callback;
    instream->userdata = this;
    instream->software_latency = 0.010; // 10 ms

    std::cout << "Software latency: " << instream->software_latency << std::endl;

    if ((err = soundio_instream_open(instream))) {
        fprintf(stderr, "unable to open input stream: %s\n", soundio_strerror(err));
        exit(1);
        return;
    }

    int capacity = instream->bytes_per_frame * ANALYSIS_BUFFER_LENGTH;
    ring_buffer = soundio_ring_buffer_create(soundio, capacity);
    if (!ring_buffer) {
        std::cerr << "Unable to create ring buffer: Out of memory" << std::endl;
        return;
    }

    aubioPitchChannels = (aubio_pitch_t **)malloc(sizeof(aubio_pitch_t *) * instream->layout.channel_count);
    // yinChannels = new Yin[instream->layout.channel_count];
    for (int channel = 0; channel < instream->layout.channel_count; ++channel) {
        aubioPitchChannels[channel] = new_aubio_pitch((char*)"yinfast", ANALYSIS_BUFFER_LENGTH, ANALYSIS_HOP_SIZE, instream->sample_rate);
        if (!aubioPitchChannels[channel]) {
            std::cerr << "Aubio algorithm not found, falling back to yin" << std::endl;
            aubioPitchChannels[channel] = new_aubio_pitch((char*)"yin", ANALYSIS_BUFFER_LENGTH, ANALYSIS_HOP_SIZE, instream->sample_rate);
        }
        // yinChannels[channel].initialize(instream->sample_rate, ANALYSIS_BUFFER_LENGTH, 1.0);
    }

    if ((err = soundio_instream_start(instream))) {
        fprintf(stderr, "unable to start input device: %s\n", soundio_strerror(err));
    }

    fprintf(stderr, "%s %d Hz %s interleaved\n",
                instream->layout.name, instream->sample_rate, soundio_format_string(instream->format));

}

/*
SDL_Rect TextToTexture( GLuint tex, TTF_Font* font, SDL_Color color, const std::string &text ) {
    auto s1 = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    auto s2 = SDL_ConvertSurfaceFormat(s1, SDL_PIXELFORMAT_RGBA32, 0);

    glBindTexture( GL_TEXTURE_2D, tex );
    // disable mipmapping on the new texture
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, s2->w, s2->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s2->pixels );

    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_Rect rect{0, 0, s2->w, s2->h};
    SDL_FreeSurface(s1);
    SDL_FreeSurface(s2);
    return rect;
}

void ImageToTexture( GLuint tex, const char* imgpath ) {
    SDL_Surface* s1 = IMG_Load( imgpath );
    auto s2 = SDL_ConvertSurfaceFormat(s1, SDL_PIXELFORMAT_RGBA32, 0);

    glBindTexture( GL_TEXTURE_2D, tex );
    // disable mipmapping on the new texture
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, s2->w, s2->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s2->pixels );

    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface( s1 );
    SDL_FreeSurface( s2 );
}
*/

App::App() {

}
App::~App() {
    if (instream) soundio_instream_pause(instream, true);
    if (outstream) soundio_outstream_pause(outstream, true);
    if (out_device_sdl) {
        SDL_ClearQueuedAudio(out_device_sdl);
        SDL_CloseAudioDevice(out_device_sdl);
    }
    if (mpv) delete mpv;
    if (glctx) SDL_GL_DeleteContext(glctx);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (mainWindow) SDL_DestroyWindow(mainWindow);
    // TTF_Quit();
    SDL_Quit();
    // Pa_Terminate();
    // soundio_ring_buffer_destroy(ring_buffer);
    if (instream) soundio_instream_destroy(instream);
    if (in_device) soundio_device_unref(in_device);
    if (outstream) soundio_outstream_destroy(outstream);
    if (out_device_sio) soundio_device_unref(out_device_sio);
    if (soundio) soundio_destroy(soundio);
    // if (yinChannels) delete[] yinChannels;
    if (aubioPitchChannels) {
        del_aubio_pitch(aubioPitchChannels[0]);
        del_aubio_pitch(aubioPitchChannels[1]);
    }
}

int App::init() {

#ifdef _WIN32
    // wasapi is broken
    SDL_setenv("SDL_AUDIODRIVER", "directsound", false);
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (!SDL_VERSION_ATLEAST(2, 0, 8)) {
        SDL_Log("SDL version 2.0.8 or greater is required. You have %i.%i.%i", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
        return 1;
    }

    std::cout << "SDL video driver: " << SDL_GetCurrentVideoDriver() << std::endl;
    std::cout << "SDL audio driver: " << SDL_GetCurrentAudioDriver() << std::endl;

    initAudio();

    /*if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return 1;
    }*/

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

    // Don't disable desktop compositing (e.g. KDE Plasma)
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    mainWindow = SDL_CreateWindow("singsing",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 360,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN/*|SDL_WINDOW_ALLOW_HIGHDPI*/);
    if (!mainWindow) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    glctx = SDL_GL_CreateContext(mainWindow);
    if (!glctx) {
        std::cerr << "Error: OpenGL context could not be created! " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (GLenum err = glewInit() != GLEW_OK) {
        std::cerr << "GLEW Error: " << glewGetErrorString(err) << std::endl;
        return 1;
    }

    std::cout << std::endl <<
        "GL_VENDOR " << glGetString(GL_VENDOR) << std::endl <<
        "GL_VERSION " << glGetString(GL_VERSION) << std::endl <<
        "GL_RENDERER " << glGetString(GL_RENDERER) << std::endl <<
        "GLSL_VERSION " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl << std::endl;

    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glDepthMask(GL_FALSE));
    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GLCall(glClearColor(0.0, 0.0, 0.0, 0.0));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    SDL_GL_SwapWindow(mainWindow);
    GLCall(glClearColor(0.0, 0.0, 0.0, 0.0));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    SDL_PumpEvents();
    return 0;
}

int App::launch() {

    Song song;
    song.parse("deps/cold-notes.txt");
    std::cout << "Parsed song: " << song.getTitle() << " by " << song.getArtist() << std::endl;

    /*
#ifdef __linux__
    auto fontFile = "/usr/share/fonts/TTF/DejaVuSans.ttf";
    auto gFont = TTF_OpenFont(fontFile, 18);
    if (!gFont) {
        fontFile = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
        gFont = TTF_OpenFont(fontFile, 18);
    }
#elif __APPLE__
    auto fontFile = "/Library/Fonts/Arial.ttf";
    auto gFont = TTF_OpenFont(fontFile, 18);
#elif _WIN32
    auto fontFile = "C:\\Windows\\Fonts\\arial.ttf";
    auto gFont = TTF_OpenFont(fontFile, 18);
#endif

    if (!gFont) {
        std::cerr << "Font not found" << std::endl;
        return 1;
    }
    */

    GLfloat g_vertex_buffer_data[]{
        // vertices    // uv
        0.0f, 0.0f,    0.0f, 0.0f,
        0.0f, 1.0f,    0.0f, 1.0f,
        1.0f, 0.0f,    1.0f, 0.0f,

        0.0f, 1.0f,    0.0f, 1.0f,
        1.0f, 1.0f,    1.0f, 1.0f,
        1.0f, 0.0f,    1.0f, 0.0f
    };

    Framebuffer mpv_fbo;

    mpv = new MPV;

    mpv->init(out_device_sdl || outstream);

    mpv->play(
        "deps/cold-video.mp4",
        "deps/cold-song.mp3"
    );

    SDL_Color textColor{255, 255, 255, 255};
    GLuint fpsTexture;
    GLCall(glGenTextures(1, &fpsTexture));
    SDL_Rect textureSize;

    GLuint vao;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    // This will identify our vertex buffer
    GLuint vertexbuffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    GLCall(glGenBuffers(1, &vertexbuffer));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW));

    Program square;
    square.load("shaders/vert.330.glsl", "shaders/frag.330.glsl");

    fpsinit();
    bool isrunning = true;
    bool firstFrame = true;
    auto last_time = SDL_GetTicks();
    std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point b = std::chrono::system_clock::now();
    while (isrunning) {
        // SDL_PumpEvents();
        SDL_Event e;
        while (SDL_PollEvent(&e) && isrunning) {
            if (e.type == SDL_QUIT) {
                isrunning = false;
                break;
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_EXPOSED) {

                }
            }
            mpv->processSDLEvent(&e);
        }
        if (!isrunning) break;
        float framespersecond = fpsupdate();
        auto now_time = SDL_GetTicks();
        int winWidth, winHeight;
        SDL_GL_GetDrawableSize(mainWindow, &winWidth, &winHeight);

        if (firstFrame || SDL_TICKS_PASSED(now_time, last_time + 500)) {
            std::string note = "";
            if (!std::isnan(currentNote1)) {
              note = tones[(int)(currentNote1) % 12];
            }
            // textureSize = TextToTexture(fpsTexture, gFont, textColor, (std::string("FPS: ") + std::to_string((int)round(framespersecond))).c_str());
            last_time = now_time;
            // std::cout << textureSize.w << std::endl;
        }

        // clean state
        GLCall(glViewport(0, 0, winWidth, winHeight));
        GLCall(glClearColor(0.0, 0.0, 0.0, 0.0));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        // render mpv video frame to framebuffer
        mpv_fbo.resize(winWidth, winHeight);
        GLCall(glDisable(GL_CULL_FACE));
        GLCall(mpv->render(winWidth, winHeight, mpv_fbo.getHandle(), 0));
        GLCall(glEnable(GL_CULL_FACE));

        // restore state
        GLCall(glViewport(0, 0, winWidth, winHeight));
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        square.use();
        square.setUniformi("u_texture", 0);
        GLCall(glActiveTexture(GL_TEXTURE0 + 0));
        GLCall(glBindVertexArray(vao));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer));
        GLCall(glEnableVertexAttribArray(square.attribute("position")));
        GLCall(glEnableVertexAttribArray(square.attribute("texcoord")));
        GLCall(glVertexAttribPointer(
            square.attribute("position"),
            2,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            4 * sizeof(GLfloat),// stride
            0                   // array buffer offset
        ));
        GLCall(glVertexAttribPointer(
            square.attribute("texcoord"),
            2,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            4 * sizeof(GLfloat),// stride
            (GLvoid*)(2 * sizeof(GLfloat)) // array buffer offset
        ));

        // draw mpv video
        GLCall(glBindTexture(GL_TEXTURE_2D, mpv_fbo.getTexture()));
        square.setUniform("viewportSize", winWidth, winHeight);
        square.setUniform("viewOffset", 0, 0);
        square.setUniform("viewSize", winWidth, winHeight);
        square.setUniform("bgColor", 0.0, 0.0, 0.0, 0.0);
        square.setUniform("textureOpacity", 1.0);
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

        // fps counter
        GLCall(glBindTexture(GL_TEXTURE_2D, fpsTexture));
        square.setUniform("viewSize", textureSize.w, textureSize.h);
        square.setUniform("bgColor", 0.0, 0.0, 0.0, 0.3);
        // GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

        square.setUniform("textureOpacity", 0.0);

        auto progress = mpv->getProgressPercent();
        square.setUniform("viewSize", winWidth, 15);
        square.setUniform("viewOffset", 0, winHeight - 15);
        square.setUniform("bgColor", 0.0, 0.0, 0.0, 0.3);
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
        square.setUniform("viewSize", winWidth * progress, 15);
        square.setUniform("bgColor", 1.0, 1.0, 1.0, 0.8);
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

        // bars background
        square.setUniform("viewSize", 100, 12 * 15 + 30);
        square.setUniform("bgColor", 0.0, 0.1, 0.2, 0.7);
        square.setUniform("viewOffset", 50, 50);
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
        square.setUniform("bgColor", 0.2, 0.04, 0.0, 0.7);
        square.setUniform("viewOffset", 150, 50);
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

        // render note bars
        square.setUniform("viewSize", 100, 30);
        if (currentConfidence1 >= 0.5) {
            square.setUniform("bgColor", 0.0, 0.5, 1.0, currentConfidence1 * 0.5);
            square.setUniform("viewOffset", 50, (12 - currentNote1) * 15 + 50);
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
        }
        if (currentConfidence2 >= 0.5) {
            square.setUniform("bgColor", 1.0, 0.2, 0.0, currentConfidence2 * 0.5);
            square.setUniform("viewOffset", 150, (12 - currentNote2) * 15 + 50);
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
        }

        GLCall(glDisableVertexAttribArray(square.attribute("position")));
        GLCall(glDisableVertexAttribArray(square.attribute("texcoord")));

        // GLCall(glBindVertexArray(0));
        // macOS bug workaround, make sure no framebuffer is active
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        SDL_GL_SwapWindow(mainWindow);
        mpv->reportSwap();

        // On macOS Mojave, moving/resizing window disables vsync
        // This is required to limit the fps to MAX_FRAMES_PER_SECOND
        if (MAX_FRAMES_PER_SECOND > 0) {
            a = std::chrono::system_clock::now();
            std::chrono::duration<double, std::milli> work_time = a - b;

            if (work_time.count() < 1000.0/MAX_FRAMES_PER_SECOND) {
                std::chrono::duration<double, std::milli> delta_ms(1000.0/MAX_FRAMES_PER_SECOND - work_time.count());
                auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
            }

            b = std::chrono::system_clock::now();
            std::chrono::duration<double, std::milli> sleep_time = b - a;
        }

        if (firstFrame) {
            // Fix for macOS Mojave. Show window after
            // SDL2 has processed events once. Otherwise
            // shows black window until moved.
            SDL_ShowWindow(mainWindow);
            SDL_RaiseWindow(mainWindow);
        }
        firstFrame = false;
    }

    GLCall(glDeleteBuffers(1, &vertexbuffer));
    GLCall(glDeleteTextures(1, &fpsTexture));

    // TTF_CloseFont(gFont);

    return EXIT_SUCCESS;
}
