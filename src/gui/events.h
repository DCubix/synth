#ifndef SYG_EVENTS_H
#define SYG_EVENTS_H

#include <vector>
#include <functional>

#include "../common.h"
#include "../sdl2.h"

#define SY__LAZY_CB(name, sig) void on##name(const std::function<void sig>& cb) { m_on##name = cb; }

class EventHandler;
class Element {
	friend class EventHandler;
public:
	struct Rect {
		i32 x{ 0 }, y{ 0 }, width{ 100 }, height{ 100 };
	};

	virtual void onMove(i32 x, i32 y) { if (m_onMove) m_onMove(x, y); }
	virtual void onClick(u8 button, i32 x, i32 y) { if (m_onClick) m_onClick(button, x, y); }
	virtual void onDoubleClick(u8 button, i32 x, i32 y) { if (m_onDoubleClick) m_onDoubleClick(button, x, y); }
	virtual void onPress(u8 button, i32 x, i32 y) { if (m_onPress) m_onPress(button, x, y); }
	virtual void onRelease(u8 button, i32 x, i32 y) { if (m_onRelease) m_onRelease(button, x, y); }
	virtual void onScroll(i8 direction) { if (m_onScroll) m_onScroll(direction); }
	virtual void onEnter() { if (m_onEnter) m_onEnter(); }
	virtual void onExit() { if (m_onExit) m_onExit(); }

	virtual void onType(char chr) { if (m_onType) m_onType(chr); }
	virtual void onKeyTap(u32 key, u32 mod) { if (m_onKeyTap) m_onKeyTap(key, mod); }
	virtual void onKeyPress(u32 key, u32 mod) { if (m_onKeyPress) m_onKeyPress(key, mod); }
	virtual void onKeyRelease(u32 key, u32 mod) { if (m_onKeyRelease) m_onKeyRelease(key, mod); }

	virtual void onFocus() { if (m_onFocus) m_onFocus(); }
	virtual void onBlur() { if (m_onBlur) m_onBlur(); }

	Rect& bounds() { return m_bounds; }

	bool enabled() const { return m_enabled; }
	void enabled(bool v) { m_enabled = v; }

	SY__LAZY_CB(Move, (i32, i32))
	SY__LAZY_CB(Click, (u8, i32, i32))
	SY__LAZY_CB(DoubleClick, (u8, i32, i32))
	SY__LAZY_CB(Press, (u8, i32, i32))
	SY__LAZY_CB(Release, (u8, i32, i32))
	SY__LAZY_CB(Scroll, (i8))
	SY__LAZY_CB(Enter, ())
	SY__LAZY_CB(Exit, ())
	SY__LAZY_CB(Focus, ())
	SY__LAZY_CB(Blur, ())
	SY__LAZY_CB(Type, (char))
	SY__LAZY_CB(KeyPress, (u32, u32))
	SY__LAZY_CB(KeyRelease, (u32, u32))

protected:
	std::function<void(i32, i32)> m_onMove;
	std::function<void(u8, i32, i32)> m_onClick, m_onDoubleClick, m_onPress, m_onRelease;
	std::function<void(i8)> m_onScroll;
	std::function<void()> m_onEnter, m_onExit, m_onFocus, m_onBlur;
	std::function<void(char)> m_onType;
	std::function<void(u32, u32)> m_onKeyPress, m_onKeyRelease, m_onKeyTap;

	bool m_clicked{ false }, m_hovered{ false }, m_focused{ false }, m_enabled{ true };
	Rect m_bounds{};

	bool hits(i32 x, i32 y);
};

class EventHandler {
public:
	enum Status {
		Running = 0,
		Quit,
		Resize
	};

	void subscribe(Element* element);
	void unsubscribe(Element* element);
	Status poll();

private:
	SDL_Event m_event;
	std::vector<Element*> m_elements;
	Element* m_focused{ nullptr };

	bool m_elementsChanged{ false };
};

#endif // SYG_EVENTS_H