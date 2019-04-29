#ifndef SYG_RENDERER_H
#define SYG_RENDERER_H

#include <string>
#include <stack>
#include <vector>

#include "sdl_headers.h"

class Renderer {
public:
	Renderer(SDL_Renderer* ren);
	~Renderer();

	void line(int x1, int y1, int x2, int y2, int r, int g, int b, int a = 255, int width = 1);
	void curve(
		int x1, int y1,
		int x2, int y2,
		int x3, int y3,
		int x4, int y4,
		int r, int g, int b, int a = 255
	);
	void multiCurve(
		const std::vector<int>& positions,
		int r, int g, int b, int a = 255
	);
	void roundRect(
		int x, int y, int w, int h, int rad, int r, int g, int b, int a = 255
	);
	void arc(int x, int y, int rad, int start, int end, int r, int g, int b, int a = 255);

	void rect(int x, int y, int w, int h, int r, int g, int b, int a = 255, bool fill = false);
	void triangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a = 255);
	void arrow(int x, int y, int w, int h, float angle, int r, int g, int b, int a = 255);

	void panel(int x, int y, int w, int h, int sx = 0, int sy = 0, float shadow = 0.0f);
	void button(int x, int y, int w, int h, int state = 0);
	void check(int x, int y, bool state);
	void flatPanel(int x, int y, int w, int h, int sx = 0, int sy = 0, float shadow = 0.0f);

	void pushClipping(int x, int y, int w, int h);
	void popClipping();

	void text(int x, int y, const std::string& str, int r, int g, int b, int a = 255);
	void textSmall(int x, int y, const std::string& str, int r, int g, int b, int a = 255);
	int textWidth(const std::string& str) const { return str.size() * 8; }

	void patch(int x, int y, int w, int h, int sx, int sy, int sw, int sh, int pad = 5, int tpad = -1);

	SDL_Renderer* sdlRenderer() { return m_ren; }

private:
	SDL_Renderer* m_ren;
	SDL_Texture *m_skin, *m_font, *m_fontSmall;
	int m_fontWidth, m_fontHeight;
	int m_fontSmallWidth, m_fontSmallHeight;

	std::stack<SDL_Rect> m_clipRects;

	void skin(
		int x, int y, int w, int h,
		int sx, int sy, int sw, int sh
	);

	void textGen(SDL_Texture* font, int fw, int fh, int x, int y, const std::string& str, int r, int g, int b, int a);
	void putChar(SDL_Texture* font, int fw, int fh, int x, int y, char c, int r, int g, int b, int a = 0xFF);
};

#endif // SYG_RENDERER_H