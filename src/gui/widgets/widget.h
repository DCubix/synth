#ifndef SYG_WIDGET_H
#define SYG_WIDGET_H

#include "../events.h"
#include "../renderer.h"

class GUI;
class Widget : public Element {
	friend class GUI;
	friend class Panel;
public:
	virtual void onDraw(Renderer& renderer) {}

	bool visible() const { return m_visible; }
	void visible(bool v) { m_visible = v; invalidate(); }

	void configure(u32 row, u32 col, u32 colSpan = 1, u32 rowSpan = 1);

	u32 gridRow() const { return m_gridRow; }
	u32 gridColumn() const { return m_gridColumn; }

	u32 rowSpan() const { return m_rowSpan; }
	u32 columnSpan() const { return m_columnSpan; }

	virtual void invalidate() { m_dirty = true; }

	Widget* parent() { return m_parent; }
	virtual bool dirty() const { return m_dirty; }

	bool alwaysRedraw() const { return m_alwaysRedraw; }
	void alwaysRedraw(bool v) { m_alwaysRedraw = v; }

	Element::Rect realBounds() const;

protected:
	GUI* m_gui;
	Widget* m_parent{ nullptr };

	bool m_visible{ true },
		m_dirty{ true },
		m_alwaysRedraw{ false };
	u32 m_gridRow{ 0 },
		m_gridColumn{ 0 },
		m_rowSpan{ 1 },
		m_columnSpan{ 1 };
};

#endif // SYG_WIDGET_H