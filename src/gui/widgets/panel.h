#ifndef SYG_PANEL_H
#define SYG_PANEL_H

#include <memory>

#include "widget.h"

class Panel : public Widget {
public:
	virtual void onDraw(Renderer& renderer) override;

	void add(Widget* widget);
	void remove(Widget* widget);

	u32 gridWidth() const { return m_gridWidth; }
	void gridWidth(u32 v) { m_gridWidth = v; }

	u32 gridHeight() const { return m_gridHeight; }
	void gridHeight(u32 v) { m_gridHeight = v; }

	u32 padding() const { return m_padding; }
	void padding(u32 v) { m_padding = v; }

	u32 spacing() const { return m_spacing; }
	void spacing(u32 v) { m_spacing = v; }

	void removeAll();
	virtual void invalidate() override;

private:
	std::vector<Widget*> m_children;
	u32 m_gridWidth{ 2 }, // # of columns
		m_gridHeight{ 2 }, // # of rows
		m_padding{ 5 }, // Spacing around the edges
		m_spacing{ 4 }; // Spacing between grid cells

	void performLayout();
};

#endif // SYG_PANEL_H