#ifndef SYG_NODE_CANVAS_H
#define SYG_NODE_CANVAS_H

#include "widget.h"
#include "../../engine/node_logic.h"

class GNodeGUI {
public:
	virtual void build(GUI* gui) = 0;
	virtual void destroy(GUI* gui) = 0;
};

struct GNode {
	i32 x{ 0 }, y{ 0 };
	u32 node{ 0 };
	bool selected{ false };
};

struct Link {
	u32 src, dest, param;
	bool active{ false };
};

class NodeCanvas : public Widget {
public:
	NodeCanvas();

	virtual void onDraw(Renderer& renderer) override;

	virtual void onClick(u8 button, i32 x, i32 y) override;
	virtual void onMove(i32 x, i32 y) override;
	virtual void onPress(u8 button, i32 x, i32 y) override;
	virtual void onRelease(u8 button, i32 x, i32 y) override;
	virtual void onKeyPress(u32 key, u32 mod) override;
	virtual void onKeyRelease(u32 key, u32 mod) override;

	NodeSystem* system() { return m_system.get(); }
	std::vector<u32> selected() { return m_selected; }

	void onSelect(const std::function<void(Node*)>& cb) { m_onSelect = cb; }

private:
	enum State {
		None = 0,
		Selecting,
		Moving,
		Linking
	};

	std::function<void(Node*)> m_onSelect;

	std::array<GNode, SynMaxNodes> m_gnodes;
	std::unique_ptr<NodeSystem> m_system;

	bool m_dragging{ false },
		m_multiSelect{ false };
	State m_state{ None };

	std::vector<u32> m_selected;

	i32 m_px{ 0 }, m_py{ 0 }, m_sx{ 0 }, m_sy{ 0 },
		m_sw{ 0 }, m_sh{ 0 };

	Link m_link;
};

#endif // SYG_NODE_CANVAS_H
