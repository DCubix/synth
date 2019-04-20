#include "label.h"

void Label::onDraw(Renderer& renderer) {
	auto b = bounds();	
	i32 tw = renderer.textWidth(m_text)+1;
	i32 tx = 0;
	switch (m_textAlign) {
		case Alignment::Center: tx = b.width / 2 - tw / 2; break;
		case Alignment::Right: tx = b.width - tw; break;
		default: break;
	}

	renderer.enableClipping(b.x, b.y, b.width, b.height);

	renderer.text(b.x + 1, b.y + (b.height / 2 - 6) + 1, m_text, 0, 0, 0, 128);
	renderer.text(b.x, b.y + (b.height / 2 - 6), m_text, 255, 255, 255, 180);

	renderer.disableClipping();
}
