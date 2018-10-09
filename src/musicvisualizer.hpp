#pragma once

#include <aubio/aubio.h>

class MusicVisualizer {
public:
    MusicVisualizer(uint_t window_size = 2048) {
        m_window_size = window_size;
        m_fftgrain = new_cvec(m_window_size); // fft norm and phase
        m_frequencies = new_fvec(m_window_size); // output buffer
        m_fft = new_aubio_fft(m_window_size);
    }
    ~MusicVisualizer() {
        del_aubio_fft(m_fft);
        del_fvec(m_frequencies);
        del_cvec(m_fftgrain);
    }

    void do_fft(float* buffer) {
        fvec_t buf{ m_window_size, buffer };
        aubio_fft_do(m_fft, &buf, m_fftgrain);
        aubio_fft_rdo(m_fft, m_fftgrain, m_frequencies);
    }

private:
    uint_t m_window_size;
    cvec_t* m_fftgrain = nullptr;
    fvec_t* m_frequencies = nullptr;
    aubio_fft_t* m_fft = nullptr;
};
