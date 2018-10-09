#pragma once

#include <GL/glew.h>
#include <libprojectM/projectM.hpp>


class ProjectMRenderer : projectM {
public:
    ProjectMRenderer(std::string config_file, int flags) :
        projectM(config_file, flags)
    {
        m_width = getWindowWidth();
        m_height = getWindowHeight();
    };

    void init() {
        projectM_resetGL(m_width, m_height);
        uint buildErrors = 0;
        std::cout << "playlist size: " << this->getPlaylistSize() << std::endl;
        for(int i = 0; i < this->getPlaylistSize(); i++) {
            std::cout << i << "\t" << this->getPresetName(i) << std::endl;
            this->selectPreset(i);
            if (this->getErrorLoadingCurrentPreset()) {
                buildErrors++;
            }
        }
    }

    void resize(unsigned int width, unsigned int height) {
        if (width == m_width && height == m_height) return;
        m_width = width;
        m_height = height;
        projectM_resetGL(width, height);
    };

    void renderFrame() {
        // glClearColor(0.0, 0.0, 0.0, 0.0);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        projectM::renderFrame();
    }

    void processAudio(float* stream, int samples) {
        pcm()->addPCMfloat((float *)stream, samples/sizeof(float));
    }
private:
    unsigned int m_width, m_height;
};
