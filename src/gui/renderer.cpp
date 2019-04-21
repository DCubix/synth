#include "renderer.h"

#include <algorithm>

#include "skin.h"
#include "font.h"
#include "font_small.h"
#include "stb_image.h"

#define MAX_SHADOW 32.0f

Renderer::Renderer(SDL_Renderer* ren)
	: m_ren(ren)
{
	int w, h, comp;
	u8* pixels = stbi_load_from_memory(skin_data, skin_size, &w, &h, &comp, 4);
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

	SDL_SetRenderDrawBlendMode(m_ren, SDL_BLENDMODE_BLEND);
}

Renderer::~Renderer() {
	/*if (m_skin) {
		SDL_DestroyTexture(m_skin);
		m_skin = nullptr;
	}*/
}

void Renderer::line(i32 x1, i32 y1, i32 x2, i32 y2, u8 r, u8 g, u8 b, u8 a) {
	SDL_SetRenderDrawColor(m_ren, r, g, b, a);
	SDL_RenderDrawLine(m_ren, x1, y1, x2, y2);
}

static f32 deCasteljau(f32 a, f32 b, f32 c, f32 d, f32 t) {
	return std::powf(1.0f - t, 3)* a +
		3.0f * std::powf(1.0f - t, 2) * t * b +
		3.0f * (1.0f - t) * std::powf(t, 2) * c +
		std::powf(t, 3) * d;
}

void Renderer::curve(
	i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, i32 x4, i32 y4,
	u8 r, u8 g, u8 b, u8 a
) {
	const u32 steps = 24;

	i32 px = i32(deCasteljau(x1, x2, x3, x4, 0));
	i32 py = i32(deCasteljau(y1, y2, y3, y4, 0));
	for (u32 i = 1; i <= steps; i++) {
		f32 t = f32(i) / f32(steps);
		i32 x = i32(deCasteljau(x1, x2, x3, x4, t));
		i32 y = i32(deCasteljau(y1, y2, y3, y4, t));
		line(px, py, x, y, r, g, b, a);
		px = x;
		py = y;
	}
}

void Renderer::rect(i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, u8 a, bool fill) {
	SDL_SetRenderDrawColor(m_ren, r, g, b, a);
	SDL_Rect rec = { x, y, w, h };
	if (fill) SDL_RenderFillRect(m_ren, &rec);
	else      SDL_RenderDrawRect(m_ren, &rec);
}

void Renderer::panel(i32 x, i32 y, i32 w, i32 h, i32 sx, i32 sy, f32 shadow) {
	const u32 TS = 64;
	if (shadow > 0.0f) {
		const u32 sz = u32(shadow * MAX_SHADOW);
		const u32 tsz = sz * 2 < 32 ? sz * 2 : sz;
		patch((x - sz/2) + sx, (y - sz/2) + sy, w + sz, h + sz, 0, 0, 96, 96, sz, tsz);
	}
	patch(x, y, w, h, 0, 160, TS, TS);
}

void Renderer::button(i32 x, i32 y, i32 w, i32 h, u32 state) {
	const u32 TS = 64;
	u32 ox = state * TS;

	////Shadow
	//const u32 sz = 4;
	//const u32 tsz = sz * 2 < 32 ? sz * 2 : sz;
	//patch((x - sz / 2), (y - sz / 2) + 1, w + sz, h + sz, 0, 0, 96, 96, sz, tsz);

	patch(x, y, w, h, ox, 96, TS, TS, 4);
}

void Renderer::check(i32 x, i32 y, bool state) {
	u32 ox = u32(state) * 16;
	skin(x, y, 16, 16, 64 + ox, 160, 16, 16);
}

void Renderer::flatPanel(i32 x, i32 y, i32 w, i32 h, i32 sx, i32 sy, f32 shadow) {
	const u32 TS = 64;
	if (shadow > 0.0f) {
		const u32 sz = u32(shadow * MAX_SHADOW);
		const u32 tsz = sz * 2 < 32 ? sz * 2 : sz;
		patch((x - sz / 2) + sx, (y - sz / 2) + sy, w + sz, h + sz, 0, 0, 96, 96, sz, tsz);
	}
	patch(x, y, w, h, 128, 160, TS, TS);
}

void Renderer::pushClipping(i32 x, i32 y, i32 w, i32 h) {
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
		f32 minX = std::max(parent.x, x);
		f32 maxX = std::min(parent.x + parent.w, x + w);
		if (maxX - minX < 1) return;

		f32 minY = std::max(parent.y, y);
		f32 maxY = std::min(parent.y + parent.h, y + h);
		if (maxY - minY < 1) return;

		x = minX;
		y = minY;
		w = i32(maxX - minX);
		h = i32(std::max(1.0f, maxY - minY));
	}

	SDL_Rect cr = { x, y, w, h };
	m_clipRects.push(cr);
	SDL_RenderSetClipRect(m_ren, &cr);
}

void Renderer::popClipping() {
	m_clipRects.top(); m_clipRects.pop();
	if (m_clipRects.empty()) {
		SDL_RenderSetClipRect(m_ren, nullptr);
	} else {
		SDL_Rect cr = m_clipRects.top();
		SDL_RenderSetClipRect(m_ren, &cr);
	}
}

void Renderer::text(i32 x, i32 y, const std::string& str, u8 r, u8 g, u8 b, u8 a) {
	textGen(m_font, m_fontWidth, m_fontHeight, x, y, str, r, g, b, a);
}

void Renderer::textSmall(i32 x, i32 y, const std::string& str, u8 r, u8 g, u8 b, u8 a) {
	textGen(m_fontSmall, m_fontSmallWidth, m_fontSmallHeight, x, y, str, r, g, b, a);
}

void Renderer::skin(i32 x, i32 y, i32 w, i32 h, i32 sx, i32 sy, i32 sw, i32 sh) {
	SDL_Rect dstRect = { x, y, w, h };
	if (sx < 0 || sy < 0 || sw < 0 || sh < 0) {
		SDL_RenderCopy(m_ren, m_skin, nullptr, &dstRect);
	} else {
		SDL_Rect srcRect = { sx, sy, sw, sh };
		SDL_RenderCopy(m_ren, m_skin, &srcRect, &dstRect);
	}
	//rect(x, y, w, h, 128, 100, 200);
}

void Renderer::patch(i32 x, i32 y, i32 w, i32 h, i32 sx, i32 sy, i32 sw, i32 sh, u32 pad, i32 tpad) {
	tpad = tpad < 0 ? pad : tpad;

	// Left
	skin(x, y, pad, pad, sx, sy, tpad, tpad); // T
	skin(x, y + pad, pad, h - pad*2, sx, sy + tpad, tpad, sh - tpad *2); // M
	skin(x, y + (h - pad), pad, pad, sx, sy + (sh - tpad), tpad, tpad); // B
	
	// Center
	i32 cw = w - pad * 2;
	i32 tw = sw - tpad * 2;
	skin(x + pad, y, cw, pad, sx + tpad, sy, tw, tpad); // T
	skin(x + pad, y + pad, cw, h - pad * 2, sx + tpad, sy + tpad, tw, sh - tpad * 2); // M
	skin(x + pad, y + (h - pad), cw, pad, sx + tpad, sy + (sh - tpad), tw, tpad); // B

	// Right
	skin(x + (w - pad), y, pad, pad, sx + (sw - tpad), sy, tpad, tpad); // T
	skin(x + (w - pad), y + pad, pad, h - pad * 2, sx + (sw - tpad), sy + tpad, tpad, sh - tpad * 2); // M
	skin(x + (w - pad), y + (h - pad), pad, pad, sx + (sw - tpad), sy + (sh - tpad), tpad, tpad); // B
}

void Renderer::textGen(SDL_Texture* font, i32 fw, i32 fh, i32 x, i32 y, const std::string& str, u8 r, u8 g, u8 b, u8 a) {
	i32 tx = x, ty = y;
	for (u32 i = 0; i < str.size(); i++) {
		u8 c = str[i];
		if (c == '\n') {
			tx = x;
			ty += 12;
		} else {
			if (!::isspace(c)) putChar(font, fw, fh, tx, ty, c, r, g, b, a);
		}
		tx += 8;
	}
}

void Renderer::putChar(SDL_Texture* font, i32 fw, i32 fh, i32 x, i32 y, char c, u8 r, u8 g, u8 b, u8 a) {
	const u32 cw = fw / 16;
	const u32 ch = fh / 16;

	const u32 sx = i32(c & 0x7F) % 16 * cw;
	const u32 sy = i32(c & 0x7F) / 16 * ch;
	SDL_Rect dstRect = { x, y, cw, ch };
	SDL_Rect srcRect = { sx, sy, cw, ch };
	
	SDL_SetTextureColorMod(font, r, g, b);
	SDL_SetTextureAlphaMod(font, a);
	SDL_RenderCopy(m_ren, font, &srcRect, &dstRect);
}
