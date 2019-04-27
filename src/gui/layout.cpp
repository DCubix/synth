#include "layout.h"

#include "widgets/widget.h"
#include "widgets/panel.h"

void BorderLayout::apply(Panel* panel, std::vector<Widget*> widgets) {
	auto b = panel->realBounds();
	i32 top = panel->padding();
	i32 bottom = b.height - panel->padding();
	i32 left = panel->padding();
	i32 right = b.width - panel->padding();

	for (Widget* w : widgets) {
		BorderLayoutPosition param = BorderLayoutPosition(w->layoutParam());
		if (!slots[param]) {
			slots[param] = w;
			m_slotCount++;
		}
	}

	for (Widget* w : slots) {
		if (!w) continue;
		auto& wb = w->bounds();
		BorderLayoutPosition param = BorderLayoutPosition(w->layoutParam());
		switch (param) {
			case BorderLayoutPosition::Top: {
				wb.x = left; wb.y = top;
				wb.width = right - left;
				top += wb.height + panel->spacing();
			} break;
			case BorderLayoutPosition::Bottom: {
				wb.x = left; wb.y = bottom - wb.height;
				wb.width = right - left;
				bottom -= wb.height + panel->spacing();
			} break;
			case BorderLayoutPosition::Left: {
				wb.x = left; wb.y = top;
				wb.height = bottom - top;
				left += wb.width + panel->spacing();
			} break;
			case BorderLayoutPosition::Right: {
				wb.x = right - wb.width; wb.y = top;
				wb.height = bottom - top;
				right -= wb.width + panel->spacing();
			} break;
			case BorderLayoutPosition::Center: {
				wb.x = left; wb.y = top;
				wb.width = right - left; wb.height = bottom - top;
			} break;
		}
	}
}

void GridLayout::apply(Panel* panel, std::vector<Widget*> widgets) {
	auto b = panel->realBounds();
	const u32 spacingWidth = (panel->gridWidth() - 1) * panel->spacing();
	const u32 spacingHeight = (panel->gridHeight() - 1) * panel->spacing();
	const u32 width = b.width - (spacingWidth + panel->padding() * 2);
	const u32 height = b.height - (spacingHeight + panel->padding() * 2);
	const u32 cellWidth = width / panel->gridWidth();
	const u32 cellHeight = height / panel->gridHeight();

	for (Widget* w : widgets) {
		w->bounds().x = w->gridColumn() * (cellWidth + panel->spacing()) + panel->padding();
		w->bounds().y = w->gridRow() * (cellHeight + panel->spacing()) + panel->padding();
		w->bounds().width = (cellWidth * w->columnSpan() + panel->spacing() * (w->columnSpan() - 1));
		w->bounds().height = (cellHeight * w->rowSpan() + panel->spacing() * (w->rowSpan() - 1));
	}
}

void StackLayout::apply(Panel* panel, std::vector<Widget*> widgets) {
	i32 y = panel->padding();
	for (Widget* w : widgets) {
		w->bounds().x = panel->padding();
		w->bounds().y = y;
		w->bounds().width = panel->bounds().width - panel->padding() * 2;
		y += w->bounds().height + panel->spacing();
	}
}

void FlowLayout::apply(Panel* panel, std::vector<Widget*> widgets) {
	i32 x = panel->padding();
	for (Widget* w : widgets) {
		w->bounds().x = x;
		w->bounds().y = panel->padding();
		w->bounds().height = panel->bounds().height - panel->padding() * 2;
		x += w->bounds().width + panel->spacing();
	}
}
