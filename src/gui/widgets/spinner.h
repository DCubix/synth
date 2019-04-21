#ifndef SYG_SPINNER_H
#define SYG_SPINNER_H

#include "widget.h"

class Spinner : public Widget {
public:
	Spinner();

	virtual void onDraw(Renderer& renderer) override;
	virtual void onMove(i32 x, i32 y) override;
	virtual void onClick(u8 button, i32 x, i32 y) override;
	virtual void onDoubleClick(u8 button, i32 x, i32 y) override;
	virtual void onPress(u8 button, i32 x, i32 y) override;
	virtual void onRelease(u8 button, i32 x, i32 y) override;
	virtual void onScroll(i8 direction) override;
	virtual void onType(char chr) override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void onBlur() override;
	virtual void onKeyPress(u32 key, u32 mod) override;

	f32 value() const { return m_value; }
	void value(f32 v);

	f32 minimum() const { return m_min; }
	void minimum(f32 v) { m_min = v; }

	f32 maximum() const { return m_max; }
	void maximum(f32 v) { m_max = v; }

	f32 step() const { return m_step; }
	void step(f32 v) { m_step = v; }

	std::string suffix() const { return m_suffix; }
	void suffix(const std::string& v) { m_suffix = v; }

	void onChange(const std::function<void()>& cb) { m_onChange = cb; }

private:
	std::function<void()> m_onChange;

	f32 m_value{ 0.0f }, m_min{ 0.0f }, m_max{ 1.0f }, m_step{ 0.1f };
	u8 m_decState{ 0 }, m_incState{ 0 };

	std::string m_suffix{ };
	i32 m_mx{ 0 };

	bool m_editing{ false };
	std::string m_valText{};
};

#endif // SYG_SPINNER_H