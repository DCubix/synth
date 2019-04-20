#include "renderer.h"

#include "skin.h"
#include "font.h"
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

void Renderer::enableClipping(i32 x, i32 y, i32 w, i32 h) {
	SDL_Rect cr = { x, y, w, h };
	SDL_RenderSetClipRect(m_ren, &cr);
}

void Renderer::disableClipping() {
	SDL_RenderSetClipRect(m_ren, nullptr);
}

void Renderer::text(i32 x, i32 y, const std::string& str, u8 r, u8 g, u8 b, u8 a) {
	i32 tx = x, ty = y;
	for (u32 i = 0; i < str.size(); i++) {
		u8 c = str[i];
		if (c == '\n') {
			tx = x;
			ty += 12;
		} else {
			if (!::isspace(c)) putChar(tx, ty, c, r, g, b, a);
		}
		tx += 8;
	}
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

void Renderer::putChar(i32 x, i32 y, char c, u8 r, u8 g, u8 b, u8 a) {
	const u32 sx = i32(c & 0x7F) % 16 * 8;
	const u32 sy = i32(c & 0x7F) / 16 * 12;
	SDL_Rect dstRect = { x, y, 8, 12 };
	SDL_Rect srcRect = { sx, sy, 8, 12 };
	
	SDL_SetTextureColorMod(m_font, r, g, b);
	SDL_SetTextureAlphaMod(m_font, a);
	SDL_RenderCopy(m_ren, m_font, &srcRect, &dstRect);
}
