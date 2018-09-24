
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include "app.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>    // std::min
// #include <portaudio.h>
#include <soundio/soundio.h>
#include "shader.hpp"
#include <glm/glm.hpp>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "base/ringbuffer.hpp"
#include "util/fpscounter.hpp"
#include "mpv.hpp"

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

static std::string tones[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
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

    fvec_t out = {
        1,
        pitch
    };
    fvec_t buf = {
        ANALYSIS_HOP_SIZE,
        (float*)&channelBuffer
    };
    aubio_pitch_do(aubioPitchChannels[channel], &buf, &out);
    *probability = aubio_pitch_get_confidence(aubioPitchChannels[channel]);
    //std::cout << "A: " << *pitch << " " << *probability << std::endl;

    // *pitch = yinChannels[channel].getPitch(buffer, 0);
    // *probability = yinChannels[channel].getProbability();
    //std::cout << "B: " << *pitch << " " << *probability << std::endl;
}

void checkRingbuffer(App* app, int bytes_per_frame) {
    float pitch1, probability1, note1;
    float pitch2, probability2, note2;
    int tone;
    float threshold = 0.03f;

    float *read_ptr = (float*)soundio_ring_buffer_read_ptr(ring_buffer);
    // int fill_bytes = soundio_ring_buffer_fill_count(ring_buffer);

    // std::cout << fill_bytes << std::endl;

    // app->analyzeAudio(0, ring.buffer(), &pitch, &probability, false);
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

void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max) {
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

    int advance_bytes = write_frames * instream->bytes_per_sample;
    soundio_ring_buffer_advance_write_ptr(ring_buffer, advance_bytes);

    //std::cout << advance_bytes / instream->bytes_per_frame << " " << soundio_ring_buffer_fill_count(ring_buffer) / instream->bytes_per_frame << ">=" << ANALYSIS_HOP_SIZE << " ";

    while (soundio_ring_buffer_fill_count(ring_buffer) >= ANALYSIS_HOP_SIZE * instream->bytes_per_frame) {
        //std::cout << "analyze ";
        checkRingbuffer(app, instream->bytes_per_frame);
    }
    //std::cout << std::endl;
}

void overflow_callback(struct SoundIoInStream *instream) {
    static int count = 0;
    fprintf(stderr, "overflow %d\n", ++count);
}

void App::initAudio() {

    // soundio
    soundio = soundio_create();
    soundio->app_name = "singsing";
    int err = soundio_connect(soundio);
    if (err) {
        fprintf(stderr, "could not connect: %s\n", soundio_strerror(err));
        return;
    }
    soundio_flush_events(soundio);

    int input_count = soundio_input_device_count(soundio);
    int default_input = soundio_default_input_device_index(soundio);

    printf("Input devices: %d\n", input_count);

    in_device = soundio_get_input_device(soundio, default_input);

    printf("Input device: %s\n", in_device->name);

    soundio_device_sort_channel_layouts(in_device);

    instream = soundio_instream_create(in_device);

    instream->name = "Input #1";
    instream->sample_rate = soundio_device_nearest_sample_rate(in_device, 48000);
    instream->format = SoundIoFormatFloat32NE;
    instream->read_callback = read_callback;
    instream->overflow_callback = overflow_callback;
    instream->userdata = this;
    instream->software_latency = 0.010; // 10 ms

    std::cout << "Software latency: " << instream->software_latency << std::endl;

    if ((err = soundio_instream_open(instream))) {
        fprintf(stderr, "unable to open input stream: %s", soundio_strerror(err));
        return;
    }

    // single channel
    int capacity = instream->sample_rate * instream->bytes_per_frame * 2 * instream->software_latency;
    ring_buffer = soundio_ring_buffer_create(soundio, capacity);
    if (!ring_buffer) {
        std::cerr << "Unable to create ring buffer: Out of memory" << std::endl;
        return;
    }

    aubioPitchChannels = (aubio_pitch_t **)malloc(sizeof(aubio_pitch_t *) * instream->layout.channel_count);
    // yinChannels = new Yin[instream->layout.channel_count];
    for (int channel = 0; channel < instream->layout.channel_count; ++channel) {
        aubioPitchChannels[channel] = new_aubio_pitch((char*)"yin", ANALYSIS_BUFFER_LENGTH, ANALYSIS_HOP_SIZE, instream->sample_rate);
        // yinChannels[channel].initialize(instream->sample_rate, ANALYSIS_BUFFER_LENGTH, 1.0);
    }

    if ((err = soundio_instream_start(instream))) {
        fprintf(stderr, "unable to start input device: %s\n", soundio_strerror(err));
    }

    fprintf(stderr, "%s %d Hz %s interleaved\n",
                instream->layout.name, instream->sample_rate, soundio_format_string(instream->format));

    return;

#if SDL_AUDIO_DRIVER_ALSAA
    if (SDL_AudioInit("alsa")) {
        std::cerr << "Failed to initialize ALSA" << std::endl;
        SDL_AudioInit(NULL);
    }
#endif

    const char* driver_name = SDL_GetCurrentAudioDriver();

    if (driver_name) {
        printf("Audio subsystem initialized; driver = %s.\n", driver_name);
    } else {
        printf("Audio subsystem not initialized.\n");
    }

    /*for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
        printf("Audio driver %d: %s\n", i, SDL_GetAudioDriver(i));
    }*/

    int i, count = SDL_GetNumAudioDevices(1);
    std::cout << "Number of devices: " << count << std::endl;
    for (i = 0; i < count; ++i) {
        printf("Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 1));
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 256;
    want.callback = NULL;

    auto dev = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 1), 0, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (dev == 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
    } else {
        if (have.format != want.format) { /* we let this one thing change. */
            SDL_Log("We didn't get Float32 audio format.");
        }
        SDL_PauseAudioDevice(dev, 0); /* start audio playing. */
        // SDL_Delay(5000); /* let the audio callback play some sound for 5 seconds. */
        SDL_CloseAudioDevice(dev);
    }

    /*
    PaError err = paNoError;
    PaStreamParameters inputParameters, outputParameters;
    err = Pa_Initialize();
    if( err != paNoError ) {
        std::cerr << "Unable to initialize PortAudio" << std::endl;
        return;
    }
    std::cout << "PortAudio version: " << Pa_GetVersionText() << "" << std::endl;

    auto numDevices = Pa_GetDeviceCount();
    std::cout << "Number of devices = " << numDevices << std::endl;

    auto defaultInput = Pa_GetDefaultInputDevice();
    auto defaultOutput = Pa_GetDefaultOutputDevice();

    for (int i = 0; i < numDevices; i++) {
        auto deviceInfo = Pa_GetDeviceInfo( i );
        std::cout << "Name: " << deviceInfo->name;
        if (i == defaultInput) {
            std::cout << " [Default Input]";
        } else if( i == Pa_GetHostApiInfo( deviceInfo->hostApi )->defaultInputDevice ) {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
            std::cout << " [Default " << hostInfo->name << " Input]";
        }
        if (i == defaultOutput) {
            std::cout << " [Default Output]";
        } else if (i == Pa_GetHostApiInfo( deviceInfo->hostApi )->defaultOutputDevice) {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
            std::cout << " [Default " << hostInfo->name << " Output]";
        }
        std::cout << std::endl;
        printf( "Host API: %s\n",  Pa_GetHostApiInfo( deviceInfo->hostApi )->name );
        printf( "Max input/output channels: %d/%d\n", deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels );

        printf( "Default low input latency   = %8.4f\n", deviceInfo->defaultLowInputLatency  );
        printf( "Default low output latency  = %8.4f\n", deviceInfo->defaultLowOutputLatency  );
        printf( "Default high input latency  = %8.4f\n", deviceInfo->defaultHighInputLatency  );
        printf( "Default high output latency = %8.4f\n", deviceInfo->defaultHighOutputLatency  );

        printf( "Default sample rate:  %8.2f\n", deviceInfo->defaultSampleRate );
    }

    PaStream* stream;
    PaStreamParameters inputParams;
    inputParams.device = defaultInput; // default input device
    auto deviceInfo = Pa_GetDeviceInfo( inputParams.device );
    inputParams.channelCount = 2; // stereo input
    inputParams.sampleFormat = paFloat32|paNonInterleaved; // float
    inputParams.suggestedLatency = (deviceInfo->defaultSampleRate/(float)ANALYSIS_BUFFER_LENGTH)/1000.0 + 0.01;//Pa_GetDeviceInfo( inputParams.device )->defaultHighInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        &inputParams,
        NULL,
        deviceInfo->defaultSampleRate,
        ANALYSIS_BUFFER_LENGTH,
        paNoFlag,
        recordCallback,
        this);
    if ( err != paNoError ) {
        std::cerr << "Unable to open input stream" << std::endl;
    }
    yinChannels = new Yin[inputParams.channelCount];
    aubioPitchChannels = (aubio_pitch_t **)malloc(sizeof(aubio_pitch_t *) * inputParams.channelCount);
    for (int channel = 0; channel < inputParams.channelCount; ++channel) {
        yinChannels[channel].initialize(deviceInfo->defaultSampleRate, ANALYSIS_BUFFER_LENGTH, 0.9);
        aubioPitchChannels[channel] = new_aubio_pitch((char*)"yin", ANALYSIS_BUFFER_LENGTH, ANALYSIS_BUFFER_LENGTH / 2, deviceInfo->defaultSampleRate);
    }
    err = Pa_StartStream(stream);
    if ( err != paNoError ) {
        std::cerr << "Unable to start input stream" << std::endl;
    }
    */
}

SDL_Rect TextToTexture( GLuint tex, TTF_Font* font, uint8_t r, uint8_t g, uint8_t b, const char* text ) {
    SDL_Color fg = { r, g, b, 255 };
    auto s1 = TTF_RenderText_Blended(font, text, fg);
    auto s2 = SDL_ConvertSurfaceFormat(s1, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface( s1 );

    glBindTexture( GL_TEXTURE_2D, tex );

    // disable mipmapping on the new texture
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, s2->w, s2->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s2->pixels );

    SDL_FreeSurface( s2 );
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_Rect rect = {0, 0, s2->w, s2->h};
    return rect;
}

void ImageToTexture( GLuint tex, const char* imgpath ) {
    SDL_Surface* s1 = IMG_Load( imgpath );
    auto s2 = SDL_ConvertSurfaceFormat(s1, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface( s1 );
    glBindTexture( GL_TEXTURE_2D, tex );

    // disable mipmapping on the new texture
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, s2->w, s2->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s2->pixels );

    SDL_FreeSurface( s2 );
    glBindTexture(GL_TEXTURE_2D, 0);
}

App::App() {

}
App::~App() {
    mpv_destroy();
    if (glctx) SDL_GL_DeleteContext(glctx);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (mainWindow) SDL_DestroyWindow(mainWindow);
    TTF_Quit();
    SDL_Quit();
    // Pa_Terminate();
    // soundio_ring_buffer_destroy(ring_buffer);
    soundio_instream_destroy(instream);
    soundio_device_unref(in_device);
    soundio_destroy(soundio);
    if (yinChannels) delete[] yinChannels;
    if (aubioPitchChannels) {
        del_aubio_pitch(aubioPitchChannels[0]);
        del_aubio_pitch(aubioPitchChannels[1]);
    }
}

int App::init() {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    initAudio();

    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    mainWindow = SDL_CreateWindow("singsing",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
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
    glewInit();

    std::cout << "GL_VERSION " << glGetString(GL_VERSION) << std::endl <<
                 "GL_RENDERER " << glGetString(GL_RENDERER) << std::endl;

    glDepthMask( GL_FALSE );
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapWindow(mainWindow);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    init_mpv();

    return 0;
}

int App::launch() {

    auto path = "../notes.txt";

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

            std::cout << identifier << " " << value << std::endl;
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
                    printf("type: %c, startBeat: %d, length: %d, pitch: %d, text: \"%s\"\n", tag, startBeat, length, pitch, text.c_str());
            }
        }
    }
    if (!finishedHeaders) {
        std::cerr << "Lyrics/Notes missing" << std::endl;
        return 0;
    }

    Shader square;
    square.init("../shaders/vert.glsl", "../shaders/frag.glsl");

    GLfloat g_vertex_buffer_data[] = {
         0.0f,   0.0f,    0.0f, 0.0f,
         0.0f,  50.0f,    0.0f, 1.0f,
       200.0f,   0.0f,    1.0f, 0.0f,

       -1.0f,  1.0f,    0.0f, 1.0f,
        1.0f, -1.0f,    1.0f, 0.0f,
       -1.0f, -1.0f,    1.0f, 1.0f,
    };

    // This will identify our vertex buffer
    GLuint vertexbuffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
        square.attribute("position"),
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        4 * sizeof(GLfloat),// stride
        0                   // array buffer offset
    );
    glVertexAttribPointer(
        square.attribute("texcoord"),
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        4 * sizeof(GLfloat),// stride
        (GLvoid*)(2 * sizeof(GLfloat)) // array buffer offset
    );

#ifdef __linux__
    auto gFont = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 48);
    if (!gFont) {
        gFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 48);
    }
#elif __APPLE__
    auto gFont = TTF_OpenFont("/Library/Fonts/Arial.ttf", 48);
#elif _WIN32
    auto gFont = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 48);
#endif

    if (!gFont) {
        std::cerr << "Font not found" << std::endl;
        return 1;
    }

    mpv_play("https://www.youtube.com/watch?v=ywjyeaMUibM");

    GLuint fpsTexture;
    glGenTextures( 1, &fpsTexture );

    fpsinit();
    bool isrunning = true;
    while (isrunning) {
        SDL_Event e;
        while (SDL_PollEvent(&e) && isrunning) {
              if (e.type == SDL_QUIT) {
                isrunning = false;
                break;
              }
              mpv_process_sdl_event(&e);
        }
        if (!isrunning) break;
        float framespersecond = fpsupdate();
        int winWidth, winHeight;
        SDL_GetWindowSize(mainWindow, &winWidth, &winHeight);

        glViewport(0, 0, winWidth, winHeight);

        std::string note = "";
        if (!isnan(currentNote1)) {
          note = tones[(int)(currentNote1) % 12];
        }

        auto textureSize = TextToTexture(fpsTexture, gFont, 255, 255, 255, (std::string("FPS: ") + std::to_string((int)framespersecond) + " " + note).c_str());
        g_vertex_buffer_data[0+0*4] = 0.0f;
        g_vertex_buffer_data[1+0*4] = 0.0f;
        g_vertex_buffer_data[0+1*4] = 0.0f;
        g_vertex_buffer_data[1+1*4] = textureSize.h;
        g_vertex_buffer_data[0+2*4] = textureSize.w;
        g_vertex_buffer_data[1+2*4] = 0.0f;

        g_vertex_buffer_data[0+3*4] = 0.0f;
        g_vertex_buffer_data[1+3*4] = textureSize.h;
        g_vertex_buffer_data[0+4*4] = textureSize.w;
        g_vertex_buffer_data[1+4*4] = 0.0f;
        g_vertex_buffer_data[0+5*4] = textureSize.w;
        g_vertex_buffer_data[1+5*4] = textureSize.h;

        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_DYNAMIC_DRAW);

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mpv_render(winWidth, winHeight);

        glEnable(GL_BLEND);

        square.use();

        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

        glEnableVertexAttribArray(square.attribute("position"));
        glEnableVertexAttribArray(square.attribute("texcoord"));
        glUniform2f(square.uniform("viewportSize"), winWidth, winHeight);
        glUniform2f(square.uniform("viewOffset"), 0, 0);
        glUniform4f(square.uniform("bgColor"), 1.0, 0.0, 1.0, 0.5);
        glUniform1f(square.uniform("textureOpacity"), 1.0);
        glBindTexture(GL_TEXTURE_2D, fpsTexture);

        glDrawArrays(GL_TRIANGLES, 0, 3*2);

        glUniform4f(square.uniform("bgColor"), 0.0, 0.5, 1.0, currentConfidence1);
        glUniform2f(square.uniform("viewOffset"), 300, winHeight - currentNote1 * 40 + 50);
        glUniform1f(square.uniform("textureOpacity"), 0.0);
        glDrawArrays(GL_TRIANGLES, 0, 3*2);
        glUniform4f(square.uniform("bgColor"), 1.0, 0.2, 0.0, currentConfidence2);
        glUniform2f(square.uniform("viewOffset"), 450, winHeight - currentNote2 * 40 + 50);
        glDrawArrays(GL_TRIANGLES, 0, 3*2);
        glDisableVertexAttribArray(square.attribute("position"));
        glDisableVertexAttribArray(square.attribute("texcoord"));

        SDL_GL_SwapWindow(mainWindow);
        SDL_Delay(1);
    }

    TTF_CloseFont(gFont);

    return EXIT_SUCCESS;
}
