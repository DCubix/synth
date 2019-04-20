#ifndef SYG_RENDERER_H
#define SYG_RENDERER_H

#include <string>

#include "../common.h"
#include "../sdl2.h"

class Renderer {
public:
	Renderer(SDL_Renderer* ren);
	~Renderer();

	void line(i32 x1, i32 y1, i32 x2, i32 y2, u8 r, u8 g, u8 b, u8 a = 0xFF);
	void rect(i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, u8 a = 0xFF, bool fill = false);

	void panel(i32 x, i32 y, i32 w, i32 h, i32 sx = 0, i32 sy = 0, f32 shadow = 0.0f);
	void button(i32 x, i32 y, i32 w, i32 h, u32 state = 0);
	void check(i32 x, i32 y, bool state);
	void flatPanel(i32 x, i32 y, i32 w, i32 h, i32 sx = 0, i32 sy = 0, f32 shadow = 0.0f);

	void enableClipping(i32 x, i32 y, i32 w, i32 h);
	void disableClipping();

	void text(i32 x, i32 y, const std::string& str, u8 r, u8 g, u8 b, u8 a = 0xFF);
	u32 textWidth(const std::string& str) const { return str.size() * 8; }

private:
	SDL_Renderer* m_ren;
	SDL_Texture *m_skin, *m_font;

	void skin(
		i32 x, i32 y, i32 w, i32 h,
		i32 sx, i32 sy, i32 sw, i32 sh
	);

	void patch(i32 x, i32 y, i32 w, i32 h, i32 sx, i32 sy, i32 sw, i32 sh, u32 pad=5, i32 tpad=-1);

	void putChar(i32 x, i32 y, char c, u8 r, u8 g, u8 b, u8 a = 0xFF);
};

#endif // SYG_RENDERER_H