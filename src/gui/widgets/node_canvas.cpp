#include "node_canvas.h"

#include "../../log/log.h"
#include "../../engine/sine_wave.hpp"
#include "../../engine/lfo.hpp"
#include "../../engine/adsr_node.hpp"
#include "../../engine/remap.hpp"

constexpr u32 NodeWidth = 72;
constexpr u32 textPad = 3;
constexpr u32 GridSize = 8;

NodeCanvas::NodeCanvas() {
	m_system = std::make_unique<NodeSystem>();
	m_gnodes[0].x = 0;
	m_gnodes[0].y = 0;
	m_gnodes[0].node = 0;
}

static u32 inY(u32 index) {
	return (10 * index) + 17;
}

void NodeCanvas::onDraw(Renderer& renderer) {
	auto b = realBounds();
	renderer.panel(b.x, b.y, b.width, b.height);
	renderer.pushClipping(b.x + 1, b.y + 1, b.width - 2, b.height - 2);
	
	// Grid
	for (u32 y = b.y; y < b.y + b.height; y += GridSize) {
		renderer.line(b.x, y, b.x + b.width, y, 196, 212, 209, 80);
	}
	for (u32 x = b.x; x < b.x + b.width; x += GridSize) {
		renderer.line(x, b.y, x, b.y + b.height, 196, 212, 209, 80);
	}
	renderer.rect(b.x, b.y, b.width, b.height, 196, 212, 209, 80);
	//

	renderer.rect(b.x, b.y, b.width, b.height, 0, 0, 0, 80, true);

	auto nodes = m_system->nodes();
	std::sort(nodes.begin(), nodes.end(), [&](u32 a, u32 b) {
		return m_gnodes[a].selected < m_gnodes[b].selected;
	});

	m_gnodes[0].x = b.width - 12;
	m_gnodes[0].y = b.height / 2 - m_gnodes[0].height / 2;

	for (u32 nid : nodes) {
		GNode& gnode = m_gnodes[nid];
		Node* node = m_system->get<Node>(nid);
		u32 count = node->paramCount() == 0 ? 1 : node->paramCount();
		gnode.height = 20 + (count * 10);
	}

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
				nw, dest.height - 16, rad,
				196, 212, 209
			);
			i32 mx = (connDestX + connSrcX) / 2;
			i32 my = connDestY + dest.height - 16;
			renderer.roundRect(mx - 3, my - 3, 6, 6, 6, 196, 212, 209);
		}
	}

	for (u32 nid : nodes) {
		GNode& gnode = m_gnodes[nid];
		Node* node = m_system->get<Node>(nid);
		i32 nx = gnode.x + b.x;
		i32 ny = gnode.y + b.y;

		i32 ngx = ::floor(gnode.x / GridSize) * GridSize + b.x;
		i32 ngy = ::floor(gnode.y / GridSize) * GridSize + b.y;

		if (node->type() != NodeType::Out) {
			if (gnode.selected && m_state == Moving) {
				renderer.rect(ngx, ngy, NodeWidth, gnode.height, 255, 255, 255, 50, true);
			}
			renderer.flatPanel(nx, ny, NodeWidth, gnode.height, 0, 3, 0.8f);
			renderer.pushClipping(nx + 1, ny + 1, NodeWidth - 2, gnode.height - 2);
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
				case NodeType::Mul: txt = "Mul"; break;
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
				renderer.rect(nx + 2, ny + 2, NodeWidth - 4, gnode.height - 4, 255, 255, 255, 50, true);
			}

			//renderer.textSmall(nx + 4, py + 4, std::to_string(node->level()), 255, 0, 0, 255);
		} else {
			renderer.flatPanel(nx, ny, NodeWidth, gnode.height, 0, 2, 0.2f);
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

			if (m_onChange) m_onChange();

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
			} else if (util::hits(x, y, nx, ny, NodeWidth, gnode.height)) {
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
						my = connDestY + src.height - 16;
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
	for (auto&& nid : m_selected) {
		GNode& gn = m_gnodes[nid];
		gn.x = ::floor(gn.x / GridSize) * GridSize;
		gn.y = ::floor(gn.y / GridSize) * GridSize;
	}

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

void NodeCanvas::load(Json json) {
	m_system->clear();

	Json nodes = json["nodes"];
	Json connections = json["connections"];

	for (u32 i = 0; i < nodes.size(); i++) {
		u32 id = 0;
		Json node = nodes[i];
		auto type = node["type"].get<std::string>();

		if (type == "sine") {
			id = m_system->create<SineWave>();
		} else if (type == "lfo") {
			id = m_system->create<LFO>();
		} else if (type == "adsr") {
			id = m_system->create<ADSRNode>();
		} else if (type == "map") {
			id = m_system->create<Map>();
		} else if (type == "value") {
			id = m_system->create<Value>();
		} else if (type == "mul") {
			id = m_system->create<Mul>();
		} else continue;

		GNode& gnd = m_gnodes[id];
		gnd.x = node["x"];
		gnd.y = node["y"];
		gnd.selected = false;
		gnd.node = id;

		m_system->get<Node>(id)->load(node);
	}

	// Connect
	for (u32 i = 0; i < connections.size(); i++) {
		Json conn = connections[i];
		m_system->connect(conn["src"], conn["dest"], conn["destParam"]);
	}
}

void NodeCanvas::save(Json& json) {
	Json nodes = Json::array();
	for (u32 nid : m_system->nodes()) {
		if (nid == 0) continue;

		Node* node = m_system->get<Node>(nid);
		GNode gnd = m_gnodes[nid];
		Json nodeJson;
		nodeJson["x"] = gnd.x;
		nodeJson["y"] = gnd.y;

		std::string txt = "NULL";
		switch (node->type()) {
			default: break;
			case NodeType::SineWave: txt = "sine"; break;
			case NodeType::LFO: txt = "lfo"; break;
			case NodeType::ADSR: txt = "adsr"; break;
			case NodeType::Map: txt = "map"; break;
			case NodeType::Value: txt = "value"; break;
			case NodeType::Mul: txt = "mul"; break;
		}
		nodeJson["type"] = txt;

		node->save(nodeJson);
		nodes.push_back(nodeJson);
	}
	json["nodes"] = nodes;

	Json connections = Json::array();
	for (u32 cid : m_system->connections()) {
		auto&& conn = m_system->getConnection(cid);
		Json connJson;
		connJson["src"] = conn->src;
		connJson["dest"] = conn->dest;
		connJson["destParam"] = conn->destParam;
		connections.push_back(connJson);
	}
	json["connections"] = connections;
}
