#include "label.h"

void Label::onDraw(Renderer& renderer) {
	bounds().height = 22;
	auto b = realBounds();	
	i32 tw = renderer.textWidth(m_text)+1;
	i32 tx = 0;
	switch (m_textAlign) {
		case Alignment::Center: tx = b.width / 2 - tw / 2; break;
		case Alignment::Right: tx = b.width - tw; break;
		default: break;
	}

	renderer.pushClipping(b.x, b.y, b.width, b.height);

	if (m_parent != nullptr) {
		auto pb = m_parent->realBounds();
		renderer.panel(pb.x, pb.y, pb.width, pb.height);
	}

	renderer.text(b.x + 1 + tx, b.y + (b.height / 2 - 6) + 1, m_text, 0, 0, 0, 128);
	renderer.text(b.x + tx, b.y + (b.height / 2 - 6), m_text, 255, 255, 255, 180);

	renderer.popClipping();
}
