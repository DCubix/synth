#include "panel.h"

#include "../gui.h"
#include "../../log/log.h"

void Panel::onDraw(Renderer& renderer) {
	performLayout();

	auto b = realBounds();
	renderer.panel(b.x, b.y, b.width, b.height);
	renderer.pushClipping(
		b.x + m_padding, b.y + m_padding, 
		b.width - m_padding * 2, b.height - m_padding * 2
	);
	for (auto&& w : m_children) {
		if (!w) continue;
		if (!w->visible()) continue;
		if (w->dirty() || w->alwaysRedraw()) {
			w->onDraw(renderer);
			w->m_dirty = false;
		}
	}
	renderer.popClipping();
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
		invalidate();
	}
}

void Panel::removeAll() {
	for (Widget* w : m_children) {
		w->m_parent = nullptr;
		m_gui->events()->unsubscribe(w);
	}
	m_children.clear();
	invalidate();
}

void Panel::invalidate() {
	for (auto&& w : m_children) {
		if (!w) continue;
		w->invalidate();
	}
	m_dirty = true;
}

void Panel::performLayout() {
	auto b = realBounds();
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
		w->bounds().x += b.x;
		w->bounds().y += b.y;
	}
}
