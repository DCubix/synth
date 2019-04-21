#include "gui.h"

#include <algorithm>

GUI::GUI(SDL_Renderer* ren) {
	m_events = std::make_unique<EventHandler>();
	m_renderer = std::make_unique<Renderer>(ren);

	m_root = create<Panel>();
	m_root->gridWidth(16);
	m_root->gridHeight(16);
	m_root->spacing(4);
	m_root->padding(2);
}

void GUI::destroy(Widget* widget) {
	auto it = std::remove_if(
		m_widgets.begin(),
		m_widgets.end(),
		[&](std::unique_ptr<Widget> const& p) {
			return p.get() == widget;
		});
	if (it != m_widgets.end()) {
		it->reset();
		m_widgets.erase(it, m_widgets.end());
	}
	m_clear = true;
}

void GUI::render(i32 width, i32 height) {
	m_root->bounds().width = width;
	m_root->bounds().height = height;

	// Clean orphans (except main panel)
	std::vector<u32> orphans;
	for (u32 i = 1; i < m_widgets.size(); i++) {
		if (m_widgets[i]->parent() == nullptr) {
			orphans.push_back(i);
		}
	}
	std::reverse(orphans.begin(), orphans.end());
	for (u32 i : orphans) {
		m_widgets.erase(m_widgets.begin() + i);
	}
	//

	if (m_clear) {
		SDL_RenderClear(m_renderer->sdlRenderer());
		for (auto&& widget : m_widgets) {
			widget->m_dirty = true;
		}
		m_clear = false;
	}

	for (auto&& widget : m_widgets) {
		auto b = widget->bounds();
		if (widget->parent() == nullptr) {
			m_renderer->enableClipping(b.x, b.y, b.width, b.height);
		}

		if (widget->m_dirty && widget->visible()) {
			widget->onDraw(*m_renderer.get());
			widget->m_dirty = false;
		}
		if (widget->parent() == nullptr) {
			m_renderer->disableClipping();
		}
	}
}
