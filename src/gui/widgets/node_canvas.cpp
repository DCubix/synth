#include "node_canvas.h"

#include "../../log/log.h"

constexpr u32 NodeWidth = 68;
constexpr u32 NodeHeight = 54;
constexpr u32 textPad = 3;

NodeCanvas::NodeCanvas() {
	m_system = std::make_unique<NodeSystem>();
	m_gnodes[0].x = 0;
	m_gnodes[0].y = 0;
	m_gnodes[0].node = 0;
}

static u32 inY(u32 index) {
	return (10 * index) + (NodeHeight / 4) + 3;
}

void NodeCanvas::onDraw(Renderer& renderer) {
	auto b = realBounds();
	renderer.panel(b.x, b.y, b.width, b.height);
	renderer.rect(b.x, b.y, b.width, b.height, 0, 0, 0, 70, true);

	renderer.pushClipping(b.x + 1, b.y + 1, b.width - 2, b.height - 2);

	auto nodes = m_system->nodes();
	std::sort(nodes.begin(), nodes.end(), [&](u32 a, u32 b) {
		return m_gnodes[a].selected < m_gnodes[b].selected;
	});

	m_gnodes[0].x = b.width - 12;
	m_gnodes[0].y = b.height / 2 - NodeHeight / 2;

	// Draw connections
	for (u32 cid : m_system->connections()) {
		auto conn = m_system->getConnection(cid);
		GNode src = m_gnodes[conn->src];
		GNode dest = m_gnodes[conn->dest];

		i32 connSrcX = (src.x + b.x) + NodeWidth - (textPad + 4);
		i32 connSrcY = (src.y + b.y) + inY(0) + textPad + 6;

		i32 connDestX = (dest.x + b.x) + textPad + 4;
		i32 connDestY = (dest.y + b.y) + inY(conn->destParam) + textPad + 6;

		connSrcY -= 2;
		connDestY -= 2;

		if (conn->src != conn->dest) {
			if (connSrcX > connDestX) {
				i32 mid = (connDestY - connSrcY) / 2;
				renderer.curve(
					connSrcX, connSrcY,
					connSrcX + 50, connSrcY + mid,
					connDestX - 50, connDestY - mid,
					connDestX, connDestY,
					196, 212, 209
				);
			} else {
				i32 mid = (connDestX - connSrcX) / 2;
				renderer.curve(
					connSrcX, connSrcY,
					connSrcX + mid, connSrcY,
					connSrcX + mid, connDestY,
					connDestX, connDestY,
					196, 212, 209
				);
			}

			i32 mx = (connDestX + connSrcX) / 2;
			i32 my = (connDestY + connSrcY) / 2;
			renderer.roundRect(mx - 3, my - 3, 6, 6, 6, 196, 212, 209);
		} else {
			const i32 rad = 10;
			const i32 w = rad * 3;
			const i32 nw = std::abs(connDestX - connSrcX) + w - 1;
			renderer.roundRect(
				connDestX - w / 2, connDestY,
				nw,	NodeHeight - 16, rad,
				196, 212, 209
			);
			i32 mx = (connDestX + connSrcX) / 2;
			i32 my = connDestY + NodeHeight - 16;
			renderer.roundRect(mx - 3, my - 3, 6, 6, 6, 196, 212, 209);
		}
	}

	for (u32 nid : nodes) {
		GNode gnode = m_gnodes[nid];
		Node* node = m_system->get<Node>(nid);
		i32 nx = gnode.x + b.x;
		i32 ny = gnode.y + b.y;

		if (node->type() != NodeType::Out) {
			renderer.flatPanel(nx, ny, NodeWidth, NodeHeight, 0, 3, 0.8f);
			renderer.pushClipping(nx + 1, ny + 1, NodeWidth - 2, NodeHeight - 2);
			renderer.rect(nx + 1, ny + 1, NodeWidth - 2, 16, 0, 0, 0, 80, true);

			std::string txt = "";
			switch (node->type()) {
				default: break;
				case NodeType::SineWave: txt = "Sine"; break;
				case NodeType::Value: txt = "Value"; break;
				case NodeType::LFO: txt = "LFO"; break;
				case NodeType::ADSR: txt = "ADSR"; break;
				case NodeType::Map: txt = "Map"; break;
				case NodeType::Reader: txt = "Reader"; break;
				case NodeType::Writer: txt = "Writer"; break;
			}
			renderer.text(nx + 5, ny + 5, txt, 0, 0, 0, 128);
			renderer.text(nx + 4, ny + 4, txt, 255, 255, 255, 180);

			for (u32 i = 0; i < node->paramCount(); i++) {
				i32 py = inY(i) + ny;
				renderer.textSmall(nx + textPad, py + textPad, "\x9", 0, 0, 0, 200);
				renderer.textSmall(nx + textPad + 8, py + textPad, node->paramName(i), 0, 0, 0, 200);
			}
			renderer.popClipping();

			i32 py = inY(0) + ny;
			renderer.textSmall(nx + NodeWidth - (textPad + 8), py + textPad, "\x9", 0, 0, 0, 200);

			if (gnode.selected) {
				renderer.rect(nx + 2, ny + 2, NodeWidth - 4, NodeHeight - 4, 255, 255, 255, 50, true);
			}

			//renderer.textSmall(nx + 4, py + 4, std::to_string(node->level()), 255, 0, 0, 255);
		} else {
			renderer.flatPanel(nx, ny, NodeWidth, NodeHeight, 0, 2, 0.2f);
			i32 py = inY(0) + ny;
			renderer.textSmall(nx + textPad, py + textPad, "\x9", 0, 0, 0, 200);
		}
	}

	// Draw dots
	for (u32 cid : m_system->connections()) {
		auto conn = m_system->getConnection(cid);
		GNode src = m_gnodes[conn->src];
		GNode dest = m_gnodes[conn->dest];

		i32 connSrcX = (src.x + b.x) + NodeWidth - (textPad + 4);
		i32 connSrcY = (src.y + b.y) + inY(0) + textPad + 6;

		i32 connDestX = (dest.x + b.x) + textPad + 4;
		i32 connDestY = (dest.y + b.y) + inY(conn->destParam) + textPad + 6;

		renderer.textSmall(connSrcX - 4, connSrcY - 6, "\x7", 196, 212, 209);
		renderer.textSmall(connDestX - 4, connDestY - 6, "\x7", 196, 212, 209);
	}

	if (m_state == Linking) {
		GNode gnode = m_gnodes[m_link.src];
		i32 nx = gnode.x + b.x;
		i32 ny = gnode.y + b.y;
		
		i32 connSrcX = nx + NodeWidth - (textPad + 4);
		i32 connSrcY = ny + inY(0) + textPad + 4;
		i32 mid = ((m_px + b.x) + connSrcX) / 2;

		renderer.textSmall(connSrcX - 4, connSrcY - 4, "\x7", 196, 212, 209);
		renderer.curve(
			connSrcX, connSrcY,
			mid, connSrcY,
			mid, m_py + b.y,
			m_px + b.x, m_py + b.y,
			196, 212, 209
		);
	}

	renderer.popClipping();
	m_time += 0.01f;
}

void NodeCanvas::onClick(u8 button, i32 x, i32 y) {

}

void NodeCanvas::onMove(i32 x, i32 y) {
	switch (m_state) {
		case None: break;
		case Moving: {
			i32 dx = x - m_px;
			i32 dy = y - m_py;

			for (auto&& nid : m_selected) {
				GNode& gn = m_gnodes[nid];
				gn.x += dx;
				gn.y += dy;
			}

			m_px = x;
			m_py = y;

			invalidate();
		} break;
		case Selecting: break;
		case Linking: {
			m_px = x;
			m_py = y;
			invalidate();
		} break;
	}
}

void NodeCanvas::onPress(u8 button, i32 x, i32 y) {
	auto b = bounds();
	if (button == SDL_BUTTON_LEFT) {
		bool clickConnector = false;
		bool hitSomething = false;
		u32 hit = UINT32_MAX;

		for (u32 nid : m_system->nodes()) {
			GNode& gnode = m_gnodes[nid];
			Node* node = m_system->get<Node>(nid);
			i32 nx = gnode.x;
			i32 ny = gnode.y;
			i32 py = inY(0) + ny;
			i32 px = nx + NodeWidth - (textPad + 8);

			if (util::hits(x, y, px, py, 8, 12)) {
				m_link.src = nid;
				m_link.active = true;
				clickConnector = true;
				break;
			} else if (util::hits(x, y, nx, ny, NodeWidth, NodeHeight)) {
				if (!m_multiSelect) {
					for (auto&& nid : m_selected) {
						m_gnodes[nid].selected = false;
					}
					m_selected.clear();
				}
				auto it = std::find(m_selected.begin(), m_selected.end(), nid);
				if (it == m_selected.end()) {
					gnode.selected = true;
					m_selected.push_back(nid);
					if (m_onSelect) m_onSelect(node);
				}
				hit = nid;
				hitSomething = true;
				invalidate();
			}
		}

		if (!clickConnector) {
			if (!hitSomething) {
				deselect();
				m_state = None;

				// Handle disconnections
				for (u32 cid : m_system->connections()) {
					auto conn = m_system->getConnection(cid);
					GNode src = m_gnodes[conn->src];
					GNode dest = m_gnodes[conn->dest];

					i32 connSrcX = src.x + NodeWidth - (textPad + 4);
					i32 connSrcY = src.y + inY(0) + textPad + 6;

					i32 connDestX = dest.x + textPad + 4;
					i32 connDestY = dest.y + inY(conn->destParam) + textPad + 6;

					connSrcY -= 2;
					connDestY -= 2;

					i32 mx = 0, my = 0;
					if (conn->src != conn->dest) {
						mx = (connDestX + connSrcX) / 2;
						my = (connDestY + connSrcY) / 2;
					} else {
						mx = (connDestX + connSrcX) / 2;
						my = connDestY + NodeHeight - 16;
					}

					if (util::hits(x, y, mx - 5, my - 5, 10, 10)) {
						m_system->disconnect(cid);
						if (m_onConnect) m_onConnect();
						invalidate();
						break;
					}
				}
			} else {
				m_state = Moving;
			}
		} else {
			m_state = Linking;
		}
		m_px = x;
		m_py = y;
	}

	Widget::onPress(button, x, y);
}

void NodeCanvas::onRelease(u8 button, i32 x, i32 y) {
	for (u32 nid : m_system->nodes()) {
		GNode& gnode = m_gnodes[nid];
		Node* node = m_system->get<Node>(nid);
		i32 nx = gnode.x;
		i32 ny = gnode.y;

		for (u32 i = 0; i < node->paramCount(); i++) {
			i32 py = inY(i) + ny;
			if (util::hits(x, y, nx + textPad, py + textPad, 8, 6) /*&& nid != m_link.src*/ && m_link.active) {
				if (m_system->connect(m_link.src, nid, i) != UINT32_MAX) {
					m_link.src = 0;
					m_link.active = false;
					if (m_onConnect) m_onConnect();
				}
				break;
			}
		}
	}

	m_state = None;
	m_sw = 0;
	m_sh = 0;
	invalidate();
}

void NodeCanvas::onKeyPress(u32 key, u32 mod) {
	if (key == SDLK_LCTRL || key == SDLK_RCTRL) m_multiSelect = true;
}

void NodeCanvas::onKeyRelease(u32 key, u32 mod) {
	if (key == SDLK_LCTRL || key == SDLK_RCTRL) m_multiSelect = false;
}

void NodeCanvas::deselect() {
	for (auto&& nid : m_selected) {
		m_gnodes[nid].selected = false;
	}
	m_selected.clear();
	if (m_onSelect) m_onSelect(nullptr);
	invalidate();
}
