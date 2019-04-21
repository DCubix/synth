#include "widget.h"

void Widget::configure(u32 row, u32 col, u32 colSpan, u32 rowSpan) {
	m_gridRow = row;
	m_gridColumn = col;
	m_columnSpan = colSpan;
	m_rowSpan = rowSpan;
}
