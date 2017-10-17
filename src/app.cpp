
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include "app.hpp"
#include <iostream>
#include <portaudio.h>
#include "shader.hpp"
#include <glm/glm.hpp>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "base/ringbuffer.hpp"
#include "util/fpscounter.hpp"

const int ANALYSIS_BUFFER_LENGTH = 2048;

RingBuffer<float> ring(ANALYSIS_BUFFER_LENGTH);
float currentNote = 0.0;
float currentNote2 = 0.0;
float currentConfidence = 0.0;
std::mutex audioMutex;

std::string tones[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

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

void App::analyzeAudio(const int& channel, const float* const buffer, float* const pitch, float* const probability) {
    /*fvec_t out = {
        1,
        pitch
    };
    fvec_t buf = {
        ANALYSIS_BUFFER_LENGTH,
        (float*)buffer
    };*/
    //aubio_pitch_do (aubioPitchChannels[channel], &buf, &out);
    //*probability = aubio_pitch_get_confidence(aubioPitchChannels[channel]);

    // std::cout << "A: " << *pitch << " " << *probability << std::endl;


    *pitch = yinChannels[channel].getPitch(buffer, 0);
    *probability = yinChannels[channel].getProbability();

    for (int i = 0; i < ANALYSIS_BUFFER_LENGTH; ++i) {
        ring.get();
    }
    // std::cout << "B: " << *pitch << " " << *probability << std::endl;
}

App::App() {

}
App::~App() {
    if (glctx) SDL_GL_DeleteContext(glctx);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (mainWindow) SDL_DestroyWindow(mainWindow);
    TTF_Quit();
    SDL_Quit();
    Pa_Terminate();
    if (yinChannels) delete[] yinChannels;
    if (aubioPitchChannels) {
        del_aubio_pitch(aubioPitchChannels[0]);
        del_aubio_pitch(aubioPitchChannels[1]);
    }
}

int App::init() {

    initAudio();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    mainWindow = SDL_CreateWindow("OpenGL",
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

    return 0;
}

void App::initAudio() {

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
}

SDL_Rect TextToTexture( GLuint tex, TTF_Font* font, uint8_t r, uint8_t g, uint8_t b, const char* text ) {
    SDL_Color fg = { r, g, b };
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
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, s1->w, s1->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s1->pixels );

    SDL_FreeSurface( s1 );
    glBindTexture(GL_TEXTURE_2D, 0);
}


int App::launch() {

    Shader square;
    square.init("../shaders/vert.glsl", "../shaders/frag.glsl");

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

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
    // The following commands will talk about our 'vertexbuffer' buffer
    // glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    // glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_DYNAMIC_DRAW);

    auto gFont = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 48);

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
        }
        if (!isrunning) break;
        float framespersecond = fpsupdate();
        int winWidth, winHeight;
        SDL_GetWindowSize(mainWindow, &winWidth, &winHeight);
        glViewport(0, 0, winWidth, winHeight);

        auto textureSize = TextToTexture(fpsTexture, gFont, 255, 255, 255, (std::string("FPS: ") + std::to_string(framespersecond)).c_str());
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

        square.use();

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
        glEnableVertexAttribArray(square.attribute("position"));
        glEnableVertexAttribArray(square.attribute("texcoord"));
        glUniform2f(square.uniform("viewportSize"), winWidth, winHeight);
        glUniform2f(square.uniform("viewOffset"), 0, currentNote * 20);
        glUniform4f(square.uniform("bgColor"), 1.0, 0.0, 1.0, currentConfidence);
        glUniform1f(square.uniform("textureOpacity"), 0.0);
        glBindTexture(GL_TEXTURE_2D, fpsTexture);

        glDrawArrays(GL_TRIANGLES, 0, 3*2);

        glUniform4f(square.uniform("bgColor"), 1.0, 0.0, 0.0, 0.5);
        glUniform2f(square.uniform("viewOffset"), 250, currentNote2 * 20);
        glDrawArrays(GL_TRIANGLES, 0, 3*2);
        glDisableVertexAttribArray(square.attribute("position"));
        glDisableVertexAttribArray(square.attribute("texcoord"));

        SDL_GL_SwapWindow(mainWindow);
    }

    TTF_CloseFont(gFont);

    return EXIT_SUCCESS;
}
