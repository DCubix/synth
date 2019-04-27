#ifndef SYG_RENDERER_H
#define SYG_RENDERER_H

#include <string>
#include <stack>
#include <vector>

#include "../common.h"
#include "../sdl2.h"

class Renderer {
public:
	Renderer(SDL_Renderer* ren);
	~Renderer();

	void line(i32 x1, i32 y1, i32 x2, i32 y2, u8 r, u8 g, u8 b, u8 a = 255, u8 width = 1);
	void curve(
		i32 x1, i32 y1,
		i32 x2, i32 y2,
		i32 x3, i32 y3,
		i32 x4, i32 y4,
		u8 r, u8 g, u8 b, u8 a = 255
	);
	void multiCurve(
		const std::vector<i32>& positions,
		u8 r, u8 g, u8 b, u8 a = 255
	);
	void roundRect(
		i32 x, i32 y, i32 w, i32 h, i32 rad, u8 r, u8 g, u8 b, u8 a = 255
	);
	void arc(i32 x, i32 y, i32 rad, i32 start, i32 end, u8 r, u8 g, u8 b, u8 a = 255);

	void rect(i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, u8 a = 255, bool fill = false);
	void triangle(i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, u8 r, u8 g, u8 b, u8 a = 255);
	void arrow(i32 x, i32 y, i32 w, i32 h, f32 angle, u8 r, u8 g, u8 b, u8 a = 255);

	void panel(i32 x, i32 y, i32 w, i32 h, i32 sx = 0, i32 sy = 0, f32 shadow = 0.0f);
	void button(i32 x, i32 y, i32 w, i32 h, u32 state = 0);
	void check(i32 x, i32 y, bool state);
	void flatPanel(i32 x, i32 y, i32 w, i32 h, i32 sx = 0, i32 sy = 0, f32 shadow = 0.0f);

	void pushClipping(i32 x, i32 y, i32 w, i32 h);
	void popClipping();

	void text(i32 x, i32 y, const std::string& str, u8 r, u8 g, u8 b, u8 a = 255);
	void textSmall(i32 x, i32 y, const std::string& str, u8 r, u8 g, u8 b, u8 a = 255);
	u32 textWidth(const std::string& str) const { return str.size() * 8; }

	void patch(i32 x, i32 y, i32 w, i32 h, i32 sx, i32 sy, i32 sw, i32 sh, u32 pad = 5, i32 tpad = -1);

	SDL_Renderer* sdlRenderer() { return m_ren; }

private:
	SDL_Renderer* m_ren;
	SDL_Texture *m_skin, *m_font, *m_fontSmall;
	u32 m_fontWidth, m_fontHeight;
	u32 m_fontSmallWidth, m_fontSmallHeight;

	std::stack<SDL_Rect> m_clipRects;

	void skin(
		i32 x, i32 y, i32 w, i32 h,
		i32 sx, i32 sy, i32 sw, i32 sh
	);

	void textGen(SDL_Texture* font, i32 fw, i32 fh, i32 x, i32 y, const std::string& str, u8 r, u8 g, u8 b, u8 a);
	void putChar(SDL_Texture* font, i32 fw, i32 fh, i32 x, i32 y, char c, u8 r, u8 g, u8 b, u8 a = 0xFF);
};

#endif // SYG_RENDERER_H