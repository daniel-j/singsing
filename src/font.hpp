#pragma once

#include <string>
#include <GL/glew.h>
#include <freetype-gl/freetype-gl.h>
#include <freetype-gl/vertex-buffer.h>
#include <SDL2/SDL_pixels.h> // SDL_Color
#include "util/glprogram.hpp"
#include "util/glutils.hpp"


class Font {
private:
	ftgl::texture_atlas_t* m_texatlas;
	ftgl::texture_font_t* m_texfont;
	ftgl::vertex_buffer_t* m_buffer;
	typedef struct {
	    float x, y;
	    float u, v;
	    float r, g, b, a;
	} vertex_t;
	Program m_program;
	std::string m_name;
	int m_viewwidth, m_viewheight;
public:
	Font(const std::string& name, const std::string& filename, float size) {
		m_name = name;

		m_texatlas = ftgl::texture_atlas_new(1024, 1024, 1);
		m_texfont = ftgl::texture_font_new_from_file(m_texatlas, size, filename.c_str());
		if (!m_texfont) {
			std::cerr << "Font file " << filename << " not found" << std::endl;
		}

		m_buffer = ftgl::vertex_buffer_new( "vertex:2f,texcoord:2f,color:4f" );

		GLCall(glGenTextures( 1, &m_texatlas->id ));
		GLCall(glBindTexture( GL_TEXTURE_2D, m_texatlas->id ));
		GLCall(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ));
	    GLCall(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ));
	    GLCall(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ));
	    GLCall(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ));
	    GLCall(glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, m_texatlas->width, m_texatlas->height, 0, GL_RED, GL_UNSIGNED_BYTE, m_texatlas->data ));

	    m_program.load("shaders/ftgl.vert.330.glsl", "shaders/ftgl.frag.330.glsl");
	}
	~Font() {
		ftgl::vertex_buffer_delete(m_buffer);
		if (m_texfont) ftgl::texture_font_delete(m_texfont);
		GLCall(glDeleteTextures( 1, &m_texatlas->id ));
		m_texatlas->id = 0;
		ftgl::texture_atlas_delete(m_texatlas);
		m_texatlas = nullptr;
		m_texfont = nullptr;
	}

	void setViewport(int width, int height) {
		m_viewwidth = width;
		m_viewheight = height;
	}

	void draw(float x = 0.0, float y = 0.0) {
		m_program.use();
		m_program.setUniform("viewportSize", m_viewwidth, m_viewheight);
		m_program.setUniform("viewOffset", x, y);
		GLCall(glBindTexture( GL_TEXTURE_2D, m_texatlas->id ));
		GLCall(ftgl::vertex_buffer_render( m_buffer, GL_TRIANGLES ));
	}

	void add_text(const std::string& text, const SDL_Color& color, float x = 0.0, float y = 0.0) {
		if (!m_texfont) return;
		const char* ctext = text.c_str();
		float r = color.r / 255;
		float g = color.g / 255;
		float b = color.b / 255;
		float a = color.a / 255;
		size_t i;
	    for( i = 0; i < strlen(ctext); ++i ) {
	        ftgl::texture_glyph_t *glyph = ftgl::texture_font_get_glyph( m_texfont, ctext + i );
	        if ( glyph != NULL ) {
	            float kerning = 0.0f;
	            if ( i > 0) {
	                kerning = ftgl::texture_glyph_get_kerning( glyph, ctext + i - 1 );
	            }
	            x += kerning;
	            float x0  = ( x + glyph->offset_x );
	            float y0  = ( y - glyph->offset_y );
	            float x1  = ( x0 + glyph->width );
	            float y1  = ( y0 + glyph->height );
	            float s0 = glyph->s0;
	            float t0 = glyph->t0;
	            float s1 = glyph->s1;
	            float t1 = glyph->t1;
	            vertex_t vertices[]{
	            	// xy     uv      rgba
	            	{ x0,y0,  s0,t0,  r,g,b,a },
	                { x0,y1,  s0,t1,  r,g,b,a },
	                { x1,y0,  s1,t0,  r,g,b,a },

	                { x0,y1,  s0,t1,  r,g,b,a },
	                { x1,y1,  s1,t1,  r,g,b,a },
	                { x1,y0,  s1,t0,  r,g,b,a }
	            };
	            ftgl::vertex_buffer_push_back( m_buffer, vertices, 6, NULL, 0);
	            x += glyph->advance_x;
	        } else {
	        	std::cerr << "glyph " << ctext[i] << " not found" << std::endl;
	        }
		}
		GLCall(glBindTexture( GL_TEXTURE_2D, m_texatlas->id ));
		GLCall(glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, m_texatlas->width, m_texatlas->height, 0, GL_RED, GL_UNSIGNED_BYTE, m_texatlas->data ));
	}

	void clear_text() {
		ftgl::vertex_buffer_clear(m_buffer);
	}
};
