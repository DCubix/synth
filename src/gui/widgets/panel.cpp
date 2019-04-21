#include "panel.h"

#include "../gui.h"

void Panel::onDraw(Renderer& renderer) {
	performLayout();

	auto b = bounds();
	renderer.panel(b.x, b.y, b.width, b.height, 0, 4, 0.3f);
	renderer.enableClipping(
		b.x + m_padding, b.y + m_padding, 
		b.width - m_padding * 2, b.height - m_padding * 2
	);
	for (auto&& w : m_children) {
		if (!w) continue;
		if (!w->visible()) continue;
		w->onDraw(renderer);
	}
	renderer.disableClipping();
}

void Panel::add(Widget* widget) {
	widget->m_parent = this;
	m_children.push_back(widget);
}

void Panel::remove(Widget* widget) {
	auto it = std::find(m_children.begin(), m_children.end(), widget);
	if (it != m_children.end()) {
		(*it)->m_parent = nullptr;
		m_gui->events()->unsubscribe(*it);
		m_children.erase(it);
		for (auto&& w : m_children) {
			if (!w) continue;
			w->invalidate();
		}
		invalidate();
	}
}

void Panel::performLayout() {
	auto b = bounds();
	const u32 spacingWidth = (m_gridWidth - 1) * m_spacing;
	const u32 spacingHeight = (m_gridHeight - 1) * m_spacing;
	const u32 width = b.width - (spacingWidth + m_padding * 2);
	const u32 height = b.height - (spacingHeight + m_padding * 2);
	const u32 cellWidth = width / m_gridWidth;
	const u32 cellHeight = height / m_gridHeight;

	for (Widget* w : m_children) {
		w->bounds().x = w->gridColumn() * (cellWidth + m_spacing) + m_padding;
		w->bounds().y = w->gridRow() * (cellHeight + m_spacing) + m_padding;
		w->bounds().width = (cellWidth * w->columnSpan() + m_spacing * (w->columnSpan() - 1));
		w->bounds().height = (cellHeight * w->rowSpan() + m_spacing * (w->rowSpan() - 1));
	}
}
