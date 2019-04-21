#include "node_canvas.h"

#include "../../log/log.h"

constexpr u32 NodeWidth = 68;
constexpr u32 NodeHeight = 54;

NodeCanvas::NodeCanvas() {
	m_system = std::make_unique<NodeSystem>();
	m_gnodes[0].x = 0;
	m_gnodes[0].y = 0;
	m_gnodes[0].node = 0;
}

static u32 inY(u32 index) {
	return (10 * index) + (NodeHeight / 4) + 2;
}

void NodeCanvas::onDraw(Renderer& renderer) {
	auto b = bounds();
	renderer.panel(b.x, b.y, b.width, b.height);
	renderer.rect(b.x, b.y, b.width, b.height, 0, 0, 0, 70, true);

	renderer.pushClipping(b.x + 1, b.y + 1, b.width - 2, b.height - 2);

	auto nodes = m_system->nodes();
	std::sort(nodes.begin(), nodes.end(), [&](u32 a, u32 b) {
		return m_gnodes[a].selected < m_gnodes[b].selected;
	});

	m_gnodes[0].x = b.width - 12;
	m_gnodes[0].y = b.height / 2 - NodeHeight / 2;

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
			}
			renderer.text(nx + 5, ny + 5, txt, 0, 0, 0, 128);
			renderer.text(nx + 4, ny + 4, txt, 255, 255, 255, 180);

			for (u32 i = 0; i < node->paramCount(); i++) {
				i32 py = inY(i) + ny;
				renderer.textSmall(nx + 4, py + 4, node->paramName(i), 0, 0, 0, 200);
			}
			renderer.popClipping();

			for (u32 i = 0; i < node->paramCount(); i++) {
				i32 py = inY(i) + ny;
				i32 ly = py + 7;
				renderer.line(nx, ly, nx - 8, ly, 255, 255, 255, 180);
			}

			i32 py = inY(0) + ny;
			i32 ly = py + 7;
			renderer.line(nx + NodeWidth, ly, nx + NodeWidth + 8, ly, 255, 255, 255, 180);

			if (gnode.selected) {
				renderer.rect(nx + 2, ny + 2, NodeWidth - 4, NodeHeight - 4, 255, 255, 255, 50, true);
			}

			//renderer.textSmall(nx + 4, py + 4, std::to_string(node->level()), 255, 0, 0, 255);
		} else {
			renderer.flatPanel(nx, ny, NodeWidth, NodeHeight, 0, 3, 0.8f);
			i32 py = inY(0) + ny;
			i32 ly = py + 7;
			renderer.line(nx, ly, nx - 8, ly, 255, 255, 255, 180);
		}
	}

	// Draw connections
	for (u32 cid : m_system->connections()) {
		auto conn = m_system->getConnection(cid);
		GNode src = m_gnodes[conn->src];
		GNode dest = m_gnodes[conn->dest];

		i32 sy = inY(0) + src.y + 2;
		i32 lsy = sy + 7;
		i32 dy = inY(conn->destParam) + dest.y + 2;
		i32 ldy = dy + 7;

		i32 srcX = src.x + NodeWidth + 11;
		i32 destX = dest.x - 6;
		
		if (srcX > destX) {
			i32 mid = (ldy - lsy) / 2;
			renderer.curve(
				srcX, lsy,
				srcX + 50, lsy + mid,
				destX - 50, ldy - mid,
				destX, ldy,
				255, 255, 255, 180
			);
		} else {
			i32 mid = (destX - srcX) / 2;
			renderer.curve(
				srcX, lsy,
				srcX + mid, lsy,
				srcX + mid, ldy,
				destX, ldy,
				255, 255, 255, 180
			);
		}
	}
	
	if (m_state == Linking) {
		GNode gnode = m_gnodes[m_link.src];
		i32 nx = gnode.x + b.x;
		i32 ny = gnode.y + b.y;
		i32 py = inY(0) + ny;
		i32 ly = py + 7;
		renderer.curve(
			nx + NodeWidth + 8, ly,
			nx + NodeWidth + 8 + 50, ly,
			nx + NodeWidth + 8 + 50, m_py,
			m_px, m_py,
			255, 255, 255, 180
		);
	}

	renderer.popClipping();
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
		bool hitSomething = false;
		u32 hit = UINT32_MAX;
		for (u32 nid : m_system->nodes()) {
			GNode& gnode = m_gnodes[nid];
			Node* node = m_system->get<Node>(nid);
			i32 nx = gnode.x;
			i32 ny = gnode.y;

			if (util::hits(x, y, nx, ny, NodeWidth, NodeHeight)) {
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
				//break;
			} else {
				i32 py = inY(0) + ny;
				i32 ly = py + 7;
				if (util::hits(x, y, nx + NodeWidth, ly - 4, 8, 8)) {
					m_link.src = nid;
					m_link.active = true;
					break;
				} else {
					for (u32 i = 0; i < node->paramCount(); i++) {
						i32 py = inY(i) + ny;
						i32 ly = py + 7;
						if (util::hits(x, y, nx - 8, ly - 3, 8, 6)) {
							if (node->param(i).connected) {
								m_system->disconnect(m_system->getConnection(nid, i));
								invalidate();
							}
							break;
						}
					}
				}
			}
		}

		if (!hitSomething) {
			deselect();
			m_state = m_link.active ? Linking : Selecting;
		} else {
			m_state = Moving;
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
			i32 ly = py + 7;
			if (util::hits(x, y, nx - 8, ly - 4, 8, 8) && nid != m_link.src && m_link.active) {
				m_system->connect(m_link.src, nid, i);
				m_link.src = 0;
				m_link.active = false;
				if (m_onConnect) m_onConnect();
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
