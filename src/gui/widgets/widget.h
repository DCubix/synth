#ifndef SYG_WIDGET_H
#define SYG_WIDGET_H

#include "../events.h"
#include "../renderer.h"

class Widget : public Element {
public:
	virtual void onDraw(Renderer& renderer) {}

	bool visible() const { return m_visible; }
	void visible(bool v) { m_visible = v; }

protected:
	bool m_visible;
};

#endif // SYG_WIDGET_H