#include "renderer.h"

#include <algorithm>
#include <cmath>

#include "skin.h"
#include "font.h"
#include "font_small.h"
#include "stb_image.h"

#define MAX_SHADOW 32.0f

Renderer::Renderer(SDL_Renderer* ren)
	: m_ren(ren)
{
	int w, h, comp;
	unsigned char* pixels = stbi_load_from_memory(skin_data, skin_size, &w, &h, &comp, 4);
	if (pixels) {
		m_skin = SDL_CreateTexture(
			m_ren,
			SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STATIC,
			w, h
		);
		SDL_UpdateTexture(m_skin, nullptr, (void*)pixels, w * 4);
		stbi_image_free(pixels);

		SDL_SetTextureBlendMode(m_skin, SDL_BLENDMODE_BLEND);
	}

	pixels = stbi_load_from_memory(font_data, font_size, &w, &h, &comp, 4);
	if (pixels) {
		m_fontWidth = w;
		m_fontHeight = h;
		m_font = SDL_CreateTexture(
			m_ren,
			SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STATIC,
			w, h
		);
		SDL_UpdateTexture(m_font, nullptr, (void*)pixels, w * 4);
		stbi_image_free(pixels);

		SDL_SetTextureBlendMode(m_font, SDL_BLENDMODE_BLEND);
	}

	pixels = stbi_load_from_memory(font_small_data, font_small_size, &w, &h, &comp, 4);
	if (pixels) {
		m_fontSmallWidth = w;
		m_fontSmallHeight = h;
		m_fontSmall = SDL_CreateTexture(
			m_ren,
			SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STATIC,
			w, h
		);
		SDL_UpdateTexture(m_fontSmall, nullptr, (void*)pixels, w * 4);
		stbi_image_free(pixels);

		SDL_SetTextureBlendMode(m_fontSmall, SDL_BLENDMODE_BLEND);
	}
}

Renderer::~Renderer() {
}

void Renderer::line(int x1, int y1, int x2, int y2, int r, int g, int b, int a, int width) {
	if (width == 1) {
		aalineRGBA(m_ren, x1, y1, x2, y2, r, g, b, a);
	} else {
		thickLineRGBA(m_ren, x1, y1, x2, y2, width, r, g, b, a);
	}
}

void Renderer::curve(
	int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
	int r, int g, int b, int a
) {
	Sint16 vx[] = { x1, x2, x3, x4 };
	Sint16 vy[] = { y1, y2, y3, y4 };
	bezierRGBA(m_ren, vx, vy, 4, 16, r, g, b, a);
}

void Renderer::multiCurve(const std::vector<int>& positions, int r, int g, int b, int a) {
	std::vector<Sint16> vx, vy;
	vx.reserve(positions.size()/2);
	vy.reserve(positions.size()/2);

	for (int i = 0; i < positions.size(); i += 2) {
		vx.push_back(positions[i]);
		vy.push_back(positions[i + 1]);
	}

	bezierRGBA(m_ren, vx.data(), vy.data(), vx.size(), 16, r, g, b, a);
}

void Renderer::roundRect(int x, int y, int w, int h, int rad, int r, int g, int b, int a) {
	roundedRectangleRGBA(m_ren, x, y, x + w, y + h, rad, r, g, b, a);
}

void Renderer::arc(int x, int y, int rad, int start, int end, int r, int g, int b, int a) {
	arcRGBA(m_ren, x, y, rad, start, end, r, g, b, a);
}

void Renderer::rect(int x, int y, int w, int h, int r, int g, int b, int a, bool fill) {
	if (fill) boxRGBA(m_ren, x, y, x + w, y + h - 1, r, g, b, a);
	else      rectangleRGBA(m_ren, x, y, x + w, y + h - 1, r, g, b, a);
}

void Renderer::triangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a) {
	filledTrigonRGBA(m_ren, x1, y1, x2, y2, x3, y3, r, g, b, a);
}

void Renderer::arrow(int x, int y, int w, int h, float angle, int r, int g, int b, int a) {
	const float c = std::cos(angle), s = std::sin(angle);
	float x1 = w, y1 = -h/2;
	float x2 = 0, y2 = 0;
	float x3 = w, y3 = h/2;

	float _x1 = x1 * c - y1 * s,
		_y1 = x1 * s + y1 * c;
	float _x2 = x2 * c - y2 * s,
		_y2 = x2 * s + y2 * c;
	float _x3 = x3 * c - y3 * s,
		_y3 = x3 * s + y3 * c;
	
	triangle(
		x + int(_x1), y + int(_y1),
		x + int(_x2), y + int(_y2),
		x + int(_x3), y + int(_y3),
		r, g, b, a
	);
}

void Renderer::panel(int x, int y, int w, int h, int sx, int sy, float shadow) {
	const int TS = 64;
	if (shadow > 0.0f) {
		const int sz = int(shadow * MAX_SHADOW);
		const int tsz = sz * 2 < 32 ? sz * 2 : sz;
		patch((x - sz/2) + sx, (y - sz/2) + sy, w + sz, h + sz, 0, 0, 96, 96, sz, tsz);
	}
	patch(x, y, w, h, 0, 160, TS, TS);
}

void Renderer::button(int x, int y, int w, int h, int state) {
	const int TS = 64;
	int ox = state * TS;
	patch(x, y, w, h, ox, 96, TS, TS, 4);
}

void Renderer::check(int x, int y, bool state) {
	int ox = int(state) * 16;
	skin(x, y, 16, 16, 64 + ox, 160, 16, 16);
}

void Renderer::flatPanel(int x, int y, int w, int h, int sx, int sy, float shadow) {
	const int TS = 64;
	if (shadow > 0.0f) {
		const int sz = int(shadow * MAX_SHADOW);
		const int tsz = sz * 2 < 32 ? sz * 2 : sz;
		patch((x - sz / 2) + sx, (y - sz / 2) + sy, w + sz, h + sz, 0, 0, 96, 96, sz, tsz);
	}
	patch(x, y, w, h, 128, 160, TS, TS);
}

void Renderer::pushClipping(int x, int y, int w, int h) {
	// Fix cliprect
	if (w < 0) {
		w = -w;
		x -= w;
	}
	if (h < 0) {
		h = -h;
		y -= h;
	}

	if (m_clipRects.empty()) {
		if (w < 1 || h < 1) return;
	} else {
		// Merge
		SDL_Rect parent = m_clipRects.top();
		float minX = std::max(parent.x, x);
		float maxX = std::min(parent.x + parent.w, x + w);
		if (maxX - minX < 1) return;

		float minY = std::max(parent.y, y);
		float maxY = std::min(parent.y + parent.h, y + h);
		if (maxY - minY < 1) return;

		x = minX;
		y = minY;
		w = int(maxX - minX);
		h = int(std::max(1.0f, maxY - minY));
	}

	SDL_Rect cr = { x, y, w, h };
	m_clipRects.push(cr);

	SDL_SetRenderDrawBlendMode(m_ren, SDL_BLENDMODE_BLEND);
	SDL_RenderSetClipRect(m_ren, &cr);
}

void Renderer::popClipping() {
	if (!m_clipRects.empty()) {
		m_clipRects.pop();
	}
	if (m_clipRects.empty()) {
		SDL_RenderSetClipRect(m_ren, nullptr);
	} else {
		SDL_Rect cr = m_clipRects.top();
		SDL_RenderSetClipRect(m_ren, &cr);
	}
}

void Renderer::text(int x, int y, const std::string& str, int r, int g, int b, int a) {
	textGen(m_font, m_fontWidth, m_fontHeight, x, y, str, r, g, b, a);
}

void Renderer::textSmall(int x, int y, const std::string& str, int r, int g, int b, int a) {
	textGen(m_fontSmall, m_fontSmallWidth, m_fontSmallHeight, x, y, str, r, g, b, a);
}

void Renderer::skin(int x, int y, int w, int h, int sx, int sy, int sw, int sh) {
	SDL_Rect dstRect = { x, y, w, h };
	if (sx < 0 || sy < 0 || sw < 0 || sh < 0) {
		SDL_RenderCopy(m_ren, m_skin, nullptr, &dstRect);
	} else {
		SDL_Rect srcRect = { sx, sy, sw, sh };
		SDL_RenderCopy(m_ren, m_skin, &srcRect, &dstRect);
	}
	//rect(x, y, w, h, 128, 100, 200);
}

void Renderer::patch(int x, int y, int w, int h, int sx, int sy, int sw, int sh, int pad, int tpad) {
	tpad = tpad < 0 ? pad : tpad;

	// Left
	skin(x, y, pad, pad, sx, sy, tpad, tpad); // T
	skin(x, y + pad, pad, h - pad*2, sx, sy + tpad, tpad, sh - tpad *2); // M
	skin(x, y + (h - pad), pad, pad, sx, sy + (sh - tpad), tpad, tpad); // B
	
	// Center
	int cw = w - pad * 2;
	int tw = sw - tpad * 2;
	skin(x + pad, y, cw, pad, sx + tpad, sy, tw, tpad); // T
	skin(x + pad, y + pad, cw, h - pad * 2, sx + tpad, sy + tpad, tw, sh - tpad * 2); // M
	skin(x + pad, y + (h - pad), cw, pad, sx + tpad, sy + (sh - tpad), tw, tpad); // B

	// Right
	skin(x + (w - pad), y, pad, pad, sx + (sw - tpad), sy, tpad, tpad); // T
	skin(x + (w - pad), y + pad, pad, h - pad * 2, sx + (sw - tpad), sy + tpad, tpad, sh - tpad * 2); // M
	skin(x + (w - pad), y + (h - pad), pad, pad, sx + (sw - tpad), sy + (sh - tpad), tpad, tpad); // B

	//rect(x, y, w, h, 255, 0, 0);
}

void Renderer::textGen(SDL_Texture* font, int fw, int fh, int x, int y, const std::string& str, int r, int g, int b, int a) {
	int tx = x, ty = y;
	for (int i = 0; i < str.size(); i++) {
		int c = str[i];
		if (c == '\n') {
			tx = x;
			ty += 12;
		} else {
			if (c != ' ') putChar(font, fw, fh, tx, ty, c, r, g, b, a);
		}
		tx += 8;
	}
}

void Renderer::putChar(SDL_Texture* font, int fw, int fh, int x, int y, char c, int r, int g, int b, int a) {
	const int cw = fw / 16;
	const int ch = fh / 16;

	const int sx = int(c & 0x7F) % 16 * cw;
	const int sy = int(c & 0x7F) / 16 * ch;
	SDL_Rect dstRect = { x, y, cw, ch };
	SDL_Rect srcRect = { sx, sy, cw, ch };
	
	SDL_SetTextureColorMod(font, r, g, b);
	SDL_SetTextureAlphaMod(font, a);
	SDL_RenderCopy(m_ren, font, &srcRect, &dstRect);
}
