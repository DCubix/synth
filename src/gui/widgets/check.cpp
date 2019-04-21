#include "check.h"

void Check::onClick(u8 button, i32 x, i32 y) {
	Widget::onClick(button, x, y);
	m_checked = !m_checked;
	invalidate();
}

void Check::onDraw(Renderer& renderer) {
	auto b = bounds();
	renderer.pushClipping(b.x, b.y, b.width, b.height);
	renderer.check(b.x, b.y + (b.height / 2 - 8), m_checked);
	renderer.text(b.x + 19, b.y + (b.height / 2 - 6) + 1, m_text, 0, 0, 0, 128);
	renderer.text(b.x + 18, b.y + (b.height / 2 - 6), m_text, 255, 255, 255, 180);
	renderer.popClipping();
}
