#include "widget.h"

void Widget::configure(u32 row, u32 col, u32 colSpan, u32 rowSpan) {
	m_gridRow = row;
	m_gridColumn = col;
	m_columnSpan = colSpan;
	m_rowSpan = rowSpan;
}

Element::Rect Widget::realBounds() const {
	Element::Rect rb{ m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height };
	if (m_parent != nullptr) {
		auto b = m_parent->realBounds();
		rb.x += b.x;
		rb.y += b.y;
	}
	return rb;
}
