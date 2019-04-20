#include "spinner.h"

#include <algorithm>

constexpr i32 buttonW = 16;

static bool hitsR(i32 x, i32 y,  i32 bx, i32 by, i32 bw, i32 bh) {
	return x >= bx &&
		x <= bx + bw &&
		y >= by &&
		y <= by + bh;
}

Spinner::Spinner() {
	bounds().width = 100;
	bounds().height = 24;
}

void Spinner::onDraw(Renderer& renderer) {
	auto b = bounds();

	f32 vnorm = (m_value - m_min) / (m_max - m_min);

	i32 mainW = b.width - buttonW;
	i32 barW = i32((mainW - 4) * vnorm);

	renderer.panel(b.x, b.y, mainW, b.height);
	renderer.rect(b.x + 2, b.y + 2, barW, b.height - 4, 0, 0, 0, 80, true);

	i32 halfH = b.height / 2;
	renderer.button(b.x + mainW, b.y, buttonW, halfH, m_incState);
	renderer.button(b.x + mainW, b.y + halfH, buttonW, halfH, m_decState);
	renderer.text(b.x + mainW + 4, b.y + (halfH / 2 - 6), "\x1E", 0, 0, 0, 180);
	renderer.text(b.x + mainW + 4, b.y + halfH + (halfH / 2 - 6), "\x1F", 0, 0, 0, 180);

	renderer.enableClipping(b.x, b.y, mainW, b.height);
	if (!m_editing) {
		auto txt = util::to_string_prec(m_value, 3) + m_suffix;
		i32 tw = renderer.textWidth(txt);
		renderer.text(b.x + (mainW / 2 - tw / 2) + 1, b.y + (b.height / 2 - 6) + 1, txt, 0, 0, 0, 128);
		renderer.text(b.x + (mainW / 2 - tw / 2), b.y + (b.height / 2 - 6), txt, 255, 255, 255, 128);
	} else {
		renderer.text(b.x + 3, b.y + (b.height / 2 - 6) + 1, m_valText, 0, 0, 0, 128);
		renderer.text(b.x + 2, b.y + (b.height / 2 - 6), m_valText, 255, 255, 255, 128);

		u32 cx = m_valText.size() * 8;
		renderer.text(b.x + cx + 3, b.y + (b.height / 2 - 6) + 1, "|", 0, 0, 0, 128);
		renderer.text(b.x + cx + 2, b.y + (b.height / 2 - 6), "|", 255, 255, 255, 128);
	}
	renderer.disableClipping();
}

void Spinner::onMove(i32 x, i32 y) {
	auto b = bounds();
	i32 halfH = b.height / 2;
	i32 mainW = b.width - buttonW;
	if (hitsR(x, y, mainW, 0, buttonW, halfH)) { // INC
		m_incState = 1;
	} else {
		m_incState = 0;
	}

	if (hitsR(x, y, mainW, halfH, buttonW, halfH)) { // DEC
		m_decState = 1;
	} else {
		m_decState = 0;
	}

	if (m_decState == 0 && m_incState == 0 && m_clicked) {
		f32 xnorm = f32(x) / (mainW - 2);

		f32 val = m_min + xnorm * (m_max - m_min);
		val = std::clamp(val, m_min, m_max);
		val = std::floor(val / m_step) * m_step;
		value(val);
	}

	Widget::onMove(x, y);
}

void Spinner::onClick(u8 button, i32 x, i32 y) {
	Widget::onClick(button, x, y);
}

void Spinner::onDoubleClick(u8 button, i32 x, i32 y) {
	Widget::onDoubleClick(button, x, y);

	auto b = bounds();
	i32 mainW = b.width - buttonW;
	if (hitsR(x, y, 0, 0, mainW, b.height)) {
		m_editing = true;
		m_valText = "";
	}
}

void Spinner::onPress(u8 button, i32 x, i32 y) {
	Widget::onPress(button, x, y);

	if (!m_editing) {
		auto b = bounds();
		i32 halfH = b.height / 2;
		i32 mainW = b.width - buttonW;
		if (hitsR(x, y, mainW, 0, buttonW, halfH)) { // INC
			value(value() + step());
			m_incState = 2;
		}
		else {
			m_incState = 1;
		}

		if (hitsR(x, y, mainW, halfH, buttonW, halfH)) { // DEC
			value(value() - step());
			m_decState = 2;
		}
		else {
			m_decState = 1;
		}
		m_value = std::clamp(m_value, m_min, m_max);

		if (m_clicked && hitsR(x, y, 0, 0, mainW, b.height)) {
			f32 xnorm = f32(x) / (mainW - 2);

			f32 val = m_min + xnorm * (m_max - m_min);
			val = std::clamp(val, m_min, m_max);
			val = std::floor(val / m_step) * m_step;
			value(val);
		}
	} else {
		onBlur();
	}
}

void Spinner::onRelease(u8 button, i32 x, i32 y) {
	Widget::onRelease(button, x, y);
	m_incState = 0;
	m_decState = 0;
}

void Spinner::onScroll(i8 direction) {
	Widget::onScroll(direction);
}

void Spinner::onType(char chr) {
	if (m_editing) {
		m_valText.push_back(chr);
	}
}

void Spinner::onEnter() {
	Widget::onEnter();
	m_incState = 0;
	m_decState = 0;
}

void Spinner::onExit() {
	Widget::onExit();
	m_incState = 0;
	m_decState = 0;
}

void Spinner::onBlur() {
	Widget::onBlur();
	if (m_editing) {
		m_editing = false;
		if (!m_valText.empty() &&
			std::find_if(
				m_valText.begin(), m_valText.end(),
				[](char c) { return !std::isdigit(c & 0xFF) && c != '.' && c != '-'; }
			) == m_valText.end()
			) {
			value(std::clamp(std::stof(m_valText), m_min, m_max));
		}
	}
}

void Spinner::onKeyPress(u32 key, u32 mod) {
	if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
		onBlur();
	}

	if (m_editing && key == SDLK_BACKSPACE) {
		if (!m_valText.empty()) m_valText.pop_back();
	}
}

void Spinner::value(f32 v) {
	if (m_onChange && m_value != v) m_onChange();
	m_value = v;
}
