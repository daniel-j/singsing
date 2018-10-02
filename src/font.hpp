#pragma once

#include <string>
#include <freetype-gl/freetype-gl.h>


class Font {
private:
	ftgl::texture_atlas_t* m_texatlas;
	ftgl::texture_font_t* m_texfont;
public:
	Font(const std::string& name, const std::string& filename, float size) {
		m_texatlas = ftgl::texture_atlas_new(512, 512, 2);
		m_texfont = ftgl::texture_font_new_from_file(m_texatlas, size, filename.c_str());
	}
	~Font() {
		delete m_texatlas;
		m_texatlas = nullptr;
		delete m_texfont;
		m_texfont = nullptr;
	}
};
