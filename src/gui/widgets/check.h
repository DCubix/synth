#ifndef SYG_CHECK_H
#define SYG_CHECK_H

#include "widget.h"

class Check : public Widget {
public:

	virtual void onClick(u8 button, i32 x, i32 y) override;
	virtual void onDraw(Renderer& renderer) override;

	std::string text() const { return m_text; }
	void text(const std::string& v) { m_text = v; }

	bool checked() const { return m_checked; }
	void checked(bool v) { m_checked = v; }

private:
	std::string m_text{ "Check" };
	bool m_checked;
};

#endif // SYG_CHECK_H