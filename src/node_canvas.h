#ifndef SYG_NODE_CANVAS_H
#define SYG_NODE_CANVAS_H

#ifndef SYN_PERF_TESTS
#include "gui/widgets/widget.h"
#include "engine/node_logic.h"

struct GNode {
	int x{ 0 }, y{ 0 }, height{ 16 };
	int node{ 0 };
	bool selected{ false };
};

struct Link {
	int src, dest, param;
	bool active{ false };
};

class NodeCanvas : public Widget {
public:
	NodeCanvas();
	virtual ~NodeCanvas() = default;

	template <typename T>
	int create() {
		int id = m_system->create<T>();
		m_gnodes[id].x = 20;
		m_gnodes[id].y = 20;
		m_gnodes[id].node = id;
		invalidate();
		return id;
	}

	virtual void onDraw(Renderer& renderer) override;

	virtual void onClick(int button, int x, int y) override;
	virtual void onMove(int x, int y) override;
	virtual void onPress(int button, int x, int y) override;
	virtual void onRelease(int button, int x, int y) override;
	virtual void onKeyPress(int key, int mod) override;
	virtual void onKeyRelease(int key, int mod) override;

	NodeSystem* system() { return m_system.get(); }
	std::vector<int> selected() { return m_selected; }

	void onSelect(const std::function<void(Node*)>& cb) { m_onSelect = cb; }
	void onChange(const std::function<void()>& cb) { m_onChange = cb; }
	void onConnect(const std::function<void()>& cb) { m_onConnect = cb; }
	void deselect();

	Node* current() { return m_selected.empty() ? nullptr : m_system->get<Node>(m_selected[0]); }

	void load(Json json);
	void save(Json& json);

private:
	enum State {
		None = 0,
		Selecting,
		Moving,
		Linking
	};

	std::function<void(Node*)> m_onSelect;
	std::function<void()> m_onConnect, m_onChange;

	std::array<GNode, SynMaxNodes> m_gnodes;
	std::unique_ptr<NodeSystem> m_system;

	bool m_dragging{ false },
		m_multiSelect{ false };
	State m_state{ None };

	std::vector<int> m_selected;

	int m_px{ 0 }, m_py{ 0 }, m_sx{ 0 }, m_sy{ 0 },
		m_sw{ 0 }, m_sh{ 0 };

	Link m_link;
	float m_time{ 0.0f };
};
#endif

#endif // SYG_NODE_CANVAS_H
