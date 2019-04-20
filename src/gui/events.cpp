#include "events.h"

#include <algorithm>

bool Element::hits(i32 x, i32 y) {
	return x >= m_bounds.x &&
		x <= m_bounds.x + m_bounds.width &&
		y >= m_bounds.y &&
		y <= m_bounds.y + m_bounds.height;
}

void EventHandler::subscribe(Element* element) {
	m_elements.push_back(element);
}

bool EventHandler::poll() {
	bool status = true;
	while (SDL_PollEvent(&m_event)) {
		switch (m_event.type) {
			case SDL_QUIT: status = false; break;
			case SDL_MOUSEBUTTONDOWN: {
				i32 x = m_event.button.x;
				i32 y = m_event.button.y;

				bool cont = false;
				for (Element* el : m_elements) {
					if (!el->hits(x, y)) continue;
					cont = true;

					if (el->enabled()) {
						if (m_focused != nullptr) {
							m_focused->m_focused = false;
						}
						el->m_focused = true;
						m_focused = el;

						el->m_clicked = true;
						el->onPress(m_event.button.button, x - el->bounds().x, y - el->bounds().y);
					}

					if (cont) break;
				}
			} break;
			case SDL_MOUSEBUTTONUP: {
				i32 x = m_event.button.x;
				i32 y = m_event.button.y;

				bool cont = false;
				for (Element* el : m_elements) {
					if (!el->hits(x, y)) continue;
					cont = true;

					if (el->enabled()) {
						if (el->m_clicked) {
							el->onClick(m_event.button.button, x - el->bounds().x, y - el->bounds().y);
						}
						el->onRelease(m_event.button.button, x - el->bounds().x, y - el->bounds().y);
					}

					el->m_clicked = false;
					if (cont) break;
				}
			} break;
			case SDL_MOUSEMOTION: {
				i32 x = m_event.motion.x;
				i32 y = m_event.motion.y;

				for (Element* el : m_elements) {
					if (!el->hits(x, y)) {
						if (el->m_hovered) {
							el->onExit();
							el->m_hovered = false;
						}
						el->m_clicked = false;
						continue;
					}

					if (el->enabled()) {
						if (!el->m_hovered) {
							el->onEnter();
						}
						el->onMove(x - el->bounds().x, y - el->bounds().y);
						el->m_hovered = true;
					}
				}
			} break;
			case SDL_MOUSEWHEEL: {
				if (m_focused != nullptr && m_focused->enabled()) {
					m_focused->onScroll(m_event.wheel.y);
				}
			} break;
		}
	}
	return status;
}
