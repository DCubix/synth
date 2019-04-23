#ifndef SYG_KEYBOARD_H
#define SYG_KEYBOARD_H

#include <array>

#include "widget.h"

class Keyboard : public Widget {
	using NoteCallback = std::function<void(u32, f32)>;
public:
	Keyboard();

	virtual void onDraw(Renderer& renderer) override;

	virtual void onPress(u8 button, i32 x, i32 y) override;
	virtual void onRelease(u8 button, i32 x, i32 y) override;
	virtual void onMove(i32 x, i32 y) override;
	virtual void onExit() override;

	virtual void onKeyTap(u32 key, u32 mod) override;
	virtual void onKeyRelease(u32 key, u32 mod) override;

	void onNoteOn(const NoteCallback& cb) { m_onNoteOn = cb; }
	void onNoteOff(const NoteCallback& cb) { m_onNoteOff = cb; }

private:
	NoteCallback m_onNoteOn, m_onNoteOff;
	bool m_drag{ false };
	std::array<bool, 88> m_keys;
	u32 m_currentKey{ 0 };

	void handleClick(i32 x, i32 y);
};

#endif // SYG_KEYBOARD_H