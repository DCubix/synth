#include "keyboard.h"
#include "../../log/log.h"

constexpr u32 WhiteKeys = 52;
constexpr u32 BlackKeys = 36;

static const u32 KEYS[] = {
	SDLK_z, SDLK_s, SDLK_x, SDLK_d, SDLK_c, SDLK_v, SDLK_g, SDLK_b, SDLK_h, SDLK_n, SDLK_j, SDLK_m,
	SDLK_q, SDLK_2, SDLK_w, SDLK_3, SDLK_e, SDLK_r, SDLK_5, SDLK_t, SDLK_6, SDLK_y, SDLK_7, SDLK_u, SDLK_i
};

static u32 po2(u32 v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

Keyboard::Keyboard() {
	m_keys.fill(false);
}

void Keyboard::onDraw(Renderer& renderer) {
	auto b = realBounds();
	u32 WhiteKeyWidth = b.width / WhiteKeys;
	u32 BlackKeyWidth = (WhiteKeyWidth / 2);
	while (BlackKeyWidth % 2 != 0) BlackKeyWidth++;

	renderer.panel(b.x, b.y, b.width, b.height);
	renderer.pushClipping(b.x + 1, b.y + 1, b.width - 2, b.height - 2);

	// draw white keys
	u32 offx = (WhiteKeyWidth * (WhiteKeys));
	for (i32 k = m_keys.size() - 1; k >= 0; k--) {
		u32 ki = k % 12;
		switch (ki) {
			case 10:
			case 8:
			case 7:
			case 5:
			case 3:
			case 2:
			case 0:
				offx -= WhiteKeyWidth;
				break;
			default: continue;
		}

		u32 kx = offx + (b.x + 1);
		u32 ktx = !m_keys[k] ? 64 : 80;
		renderer.patch(kx, b.y + 1, WhiteKeyWidth, b.height - 2, ktx, 176, 16, 32);
		if (ki == 3) {
			u32 yoff = m_keys[k] ? 3 : 0;
			u32 oct = k / 12;
			renderer.textSmall(
				kx + (WhiteKeyWidth / 2 - 3),
				b.y + yoff + b.height - 26,
				"C", 0, 0, 0, 100
			);
			renderer.textSmall(
				kx + (WhiteKeyWidth / 2 - 3),
				b.y + yoff + b.height - 16,
				std::to_string(oct), 0, 0, 0, 100
			);
		}
	}

	offx = (WhiteKeyWidth / 2) + (BlackKeyWidth / 2);

	// draw black keys
	for (i32 k = 0; k < m_keys.size(); k++) {
		u32 ki = k % 12;
		u32 ox = offx;
		switch (ki) {
			case 11: offx += WhiteKeyWidth; break;
			case 9:  offx += WhiteKeyWidth; break;
			case 6:  offx += WhiteKeyWidth * 2; break;
			case 4:  offx += WhiteKeyWidth; break;
			case 1:  offx += WhiteKeyWidth * 2; break;
			default: continue;
		}

		u32 kx = ox + (b.x + 1);
		u32 ktx = !m_keys[k] ? 96 : 112;
		renderer.patch(kx, b.y + 1, BlackKeyWidth, b.height / 6 + b.height / 2 - 2, ktx, 176, 16, 32);
	}
	renderer.popClipping();

	bounds().width = (WhiteKeyWidth * WhiteKeys) + 2;
}

void Keyboard::onPress(u8 button, i32 x, i32 y) {
	if (button != SDL_BUTTON_LEFT) return;
	handleClick(x, y);
	m_drag = true;
}

void Keyboard::onRelease(u8 button, i32 x, i32 y) {
	for (u32 k = 0; k < m_keys.size(); k++) {
		if (m_keys[k]) {
			m_keys[k] = false;
			if (m_onNoteOff) m_onNoteOff(k + 21, 0.0f);
		}
	}
	m_currentKey = 0;
	m_drag = false;
	invalidate();
}

void Keyboard::onMove(i32 x, i32 y) {
	if (m_drag) {
		handleClick(x, y);
	}
}

void Keyboard::onExit() {
	onRelease(0, 0, 0);
}

void Keyboard::onKeyTap(u32 key, u32 mod) {
	u32 i = 48;
	for (u32 k : KEYS) {
		if (key == k) {
			m_keys[i - 21] = true;
			if (m_onNoteOn) m_onNoteOn(i, 0.5f);
			break;
		}
		i++;
	}
	invalidate();
}

void Keyboard::onKeyRelease(u32 key, u32 mod) {
	u32 i = 48;
	for (u32 k : KEYS) {
		if (key == k) {
			m_keys[i - 21] = false;
			if (m_onNoteOff) m_onNoteOff(i, 0.0f);
			break;
		}
		i++;
	}
	invalidate();
}

void Keyboard::handleClick(i32 x, i32 y) {
	auto b = realBounds();
	u32 WhiteKeyWidth = b.width / WhiteKeys;
	u32 BlackKeyWidth = (WhiteKeyWidth / 2);
	while (BlackKeyWidth % 2 != 0) BlackKeyWidth++;

	u32 offx = (WhiteKeyWidth / 2) + (BlackKeyWidth / 2);

	bool handled = false;
	// test black keys
	for (i32 k = 0; k < m_keys.size(); k++) {
		u32 ki = k % 12;
		u32 ox = offx;
		switch (ki) {
			case 11: offx += WhiteKeyWidth; break;
			case 9:  offx += WhiteKeyWidth; break;
			case 6:  offx += WhiteKeyWidth * 2; break;
			case 4:  offx += WhiteKeyWidth; break;
			case 1:  offx += WhiteKeyWidth * 2; break;
			default: continue;
		}

		u32 kx = ox + 1;
		u32 km = k + 21;
		u32 h = b.height / 6 + b.height / 2 - 2;
		m_keys[k] = util::hits(x, y, kx, 1, BlackKeyWidth, h);
		if (m_keys[k] && m_currentKey != km) {
			handled = true;
			if (m_onNoteOn) m_onNoteOn(km, f32(y) / h);
			m_currentKey = km;
		} else if (m_currentKey != km) {
			if (m_onNoteOff) m_onNoteOff(km, 0.0f);
		}
	}

	if (!handled) {
		// test white keys
		offx = (WhiteKeyWidth * (WhiteKeys));
		for (i32 k = m_keys.size() - 1; k >= 0; k--) {
			u32 ki = k % 12;
			switch (ki) {
				case 10:
				case 8:
				case 7:
				case 5:
				case 3:
				case 2:
				case 0:
					offx -= WhiteKeyWidth;
					break;
				default: continue;
			}

			u32 kx = offx + 1;
			u32 km = k + 21;
			m_keys[k] = util::hits(x, y, kx, 1, WhiteKeyWidth, b.height - 2);
			if (m_keys[k] && m_currentKey != km) {
				if (m_onNoteOn) m_onNoteOn(km, f32(y) / (b.height - 2));
				m_currentKey = km;
			} else if (m_currentKey != km) {
				if (m_onNoteOff) m_onNoteOff(km, 0.0f);
			}
		}
	} else {
		for (i32 k = 0; k < m_keys.size(); k++) {
			u32 ki = k % 12;
			switch (ki) {
				case 10:
				case 8:
				case 7:
				case 5:
				case 3:
				case 2:
				case 0:
					offx -= WhiteKeyWidth;
					break;
				default: continue;
			}
			m_keys[k] = false;
		}
	}
	invalidate();
}
