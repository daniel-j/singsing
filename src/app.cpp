
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include "app.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>    // std::min
// #include <portaudio.h>
#include <soundio/soundio.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "base/ringbuffer.hpp"
#include "util/fpscounter.hpp"
#include "mpv.hpp"
#include "util/glutils.hpp"
#include "util/glprogram.hpp"
#include "util/glframebuffer.hpp"

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

    fvec_t out{
        1,
        pitch
    };
    fvec_t buf{
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

    int advance_bytes = write_frames * instream->bytes_per_frame;
    soundio_ring_buffer_advance_write_ptr(ring_buffer, advance_bytes);

    //std::cout << write_frames << " " << soundio_ring_buffer_fill_count(ring_buffer) / instream->bytes_per_frame << ">=" << ANALYSIS_HOP_SIZE << " ";

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
    instream->sample_rate = soundio_device_nearest_sample_rate(in_device, 44100);
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

    int capacity = instream->bytes_per_frame * ANALYSIS_BUFFER_LENGTH;
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
}

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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);


    mainWindow = SDL_CreateWindow("singsing",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 360,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE/*|SDL_WINDOW_ALLOW_HIGHDPI*/);
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

    std::cout << "GL_VENDOR " << glGetString(GL_VENDOR) << std::endl <<
                 "GL_VERSION " << glGetString(GL_VERSION) << std::endl <<
                 "GL_RENDERER " << glGetString(GL_RENDERER) << std::endl <<
                 "GLSL_VERSION " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glDepthMask(GL_FALSE));
    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GLCall(glClearColor(0.0, 0.0, 0.0, 0.0));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    SDL_GL_SwapWindow(mainWindow);
    GLCall(glClearColor(0.0, 0.0, 0.0, 0.0));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    return 0;
}

int App::launch() {

    /*
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
    */

#ifdef __linux__
    auto gFont = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 18);
    if (!gFont) {
        gFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
    }
#elif __APPLE__
    auto gFont = TTF_OpenFont("/Library/Fonts/Arial.ttf", 18);
#elif _WIN32
    auto gFont = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 18);
#endif

    if (!gFont) {
        std::cerr << "Font not found" << std::endl;
        return 1;
    }

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

    init_mpv();
    mpv_play("https://www.youtube.com/watch?v=ywjyeaMUibM");

    SDL_Color textColor{255, 255, 255, 255};
    GLuint fpsTexture;
    GLCall(glGenTextures(1, &fpsTexture));
    SDL_Rect textureSize;

    // This will identify our vertex buffer
    GLuint vertexbuffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    GLCall(glGenBuffers(1, &vertexbuffer));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW));

    Program square;
    square.load("../shaders/vert.glsl", "../shaders/frag.glsl");

    fpsinit();
    bool isrunning = true;
    bool firstFrame = true;
    auto last_time = SDL_GetTicks();
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
        auto now_time = SDL_GetTicks();
        int winWidth, winHeight;
        SDL_GL_GetDrawableSize(mainWindow, &winWidth, &winHeight);

        if (firstFrame || SDL_TICKS_PASSED(now_time, last_time + 500)) {
            std::string note = "";
            if (!isnan(currentNote1)) {
              note = tones[(int)(currentNote1) % 12];
            }
            textureSize = TextToTexture(fpsTexture, gFont, textColor, (std::string("FPS: ") + std::to_string((int)framespersecond)).c_str());
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
        GLCall(mpv_render(winWidth, winHeight, mpv_fbo.getHandle(), 0));
        GLCall(glEnable(GL_CULL_FACE));

        // restore state
        GLCall(glViewport(0, 0, winWidth, winHeight));
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        square.use();
        square.setUniformi("texture", 0);
        // GLCall(glActiveTexture(GL_TEXTURE0 + 0));
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
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

        square.setUniform("textureOpacity", 0.0);

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

        // macOS bug workaround
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        SDL_GL_SwapWindow(mainWindow);
        // SDL_Delay(1);
        mpv_flip();
        firstFrame = false;
    }

    GLCall(glDeleteBuffers(1, &vertexbuffer));
    GLCall(glDeleteTextures(1, &fpsTexture));

    TTF_CloseFont(gFont);

    return EXIT_SUCCESS;
}
