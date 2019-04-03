#pragma once

#include <vector>
#include <glad/glad.h>
#include "util/glutils.hpp"
#include "util/glprogram.hpp"
#include <aubio/aubio.h>

class MusicVisualizer {
public:
    MusicVisualizer(uint_t window_size = 2048) {
        m_window_size = window_size;
        m_audiosamples = new_fvec(m_window_size); // input audio samples
        m_fftgrain = new_cvec(m_window_size); // fft norm and phase
        m_audiofreqs = new_fvec(m_window_size); // output buffer of frequencies
        m_fft = new_aubio_fft(m_window_size);

        m_gl_program = new Program();
    }
    ~MusicVisualizer() {
        del_aubio_fft(m_fft);
        del_fvec(m_audiofreqs);
        del_cvec(m_fftgrain);
        del_fvec(m_audiosamples);

        if (initialized) {
            if (m_gl_program) delete m_gl_program;
            glDeleteVertexArrays(1, &m_gl_vao);
        }
    }

    void init() {
        if (initialized) return;
        // Required by gl but unused.
        GLCall(glGenVertexArrays(1, &m_gl_vao));
        GLCall(glBindVertexArray(m_gl_vao));
        for (size_t i = 0; i < (sizeof(m_gl_audio_textures)/sizeof(*m_gl_audio_textures)); ++i) {
            GLuint tex;
            GLCall(glGenTextures(1, &tex));
            GLCall(glActiveTexture(GL_TEXTURE0 + i));
            GLCall(glBindTexture(GL_TEXTURE_1D, tex));
            GLCall(glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, m_window_size, 0, GL_RED, GL_FLOAT, nullptr));
            GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT));
            GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT));
            GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            m_gl_audio_textures[i] = tex;
        }

        // m_gl_program->load("shaders/mviz.vert.glsl", "shaders/mviz.frag.glsl"/*, "shaders/mviz.geom.glsl"*/);

        initialized = true;
    }


    void write_audio_samples(float* buffer, int channels) {
        // downmix interleaved buffer to mono
        for (size_t i = 0; i < m_window_size; ++i) {
            float value = 0.0;
            for (size_t j = 0; j < channels; ++j) {
                value += buffer[i * channels + j];
            }
            value /= (float)channels;
            m_audiosamples->data[i] = value;
        }
        aubio_fft_do(m_fft, m_audiosamples, m_fftgrain);
        aubio_fft_rdo(m_fft, m_fftgrain, m_audiofreqs);
        audio_updated = true;
    }

    // call this from gl thread
    void update_textures() {
        if (initialized && audio_updated) {
            GLCall(glActiveTexture(GL_TEXTURE0 + 0));
            GLCall(glTexSubImage1D(GL_TEXTURE_1D, 0, 0, m_window_size, GL_RED, GL_FLOAT, m_audiosamples->data));
            GLCall(glActiveTexture(GL_TEXTURE0 + 1));
            GLCall(glTexSubImage1D(GL_TEXTURE_1D, 0, 0, m_window_size, GL_RED, GL_FLOAT, m_audiofreqs->data));
            audio_updated = false;
        }
    }

    void render() {
        m_gl_program->use();
    }

private:
    bool initialized = false;
    bool audio_updated = false;

    // aubio stuff
    uint_t m_window_size;
    fvec_t* m_audiosamples = nullptr;
    cvec_t* m_fftgrain = nullptr;
    fvec_t* m_audiofreqs = nullptr;
    aubio_fft_t* m_fft = nullptr;

    // opengl stuff
    Program* m_gl_program = nullptr;
    GLuint m_gl_vao{0};
    GLuint m_gl_audio_textures[2]{0};
};
