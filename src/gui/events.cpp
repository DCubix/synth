#include "events.h"

#include <algorithm>

#include "../log/log.h"

bool Element::hits(i32 x, i32 y) {
	return x >= m_bounds.x &&
		x <= m_bounds.x + m_bounds.width &&
		y >= m_bounds.y &&
		y <= m_bounds.y + m_bounds.height;
}

void EventHandler::subscribe(Element* element) {
	m_elements.push_back(element);
	m_elementsChanged = true;
}

void EventHandler::unsubscribe(Element* element) {
	auto it = std::find(m_elements.begin(), m_elements.end(), element);
	if (it != m_elements.end()) {
		m_elements.erase(it);
		m_elementsChanged = true;
		m_focused = nullptr;
	}
}

EventHandler::Status EventHandler::poll() {
	Status status = Running;
	while (SDL_PollEvent(&m_event)) {
		switch (m_event.type) {
			case SDL_QUIT: status = Quit; break;
			case SDL_MOUSEBUTTONDOWN: {
				i32 x = m_event.button.x;
				i32 y = m_event.button.y;

				bool cont = false;
				for (Element* el : m_elements) {
					if (!el->hits(x, y)) continue;
					cont = true;

					if (el->enabled()) {
						if (m_focused != nullptr) {
							m_focused->onBlur();
							m_focused->m_focused = false;
							if (m_elementsChanged) {
								m_elementsChanged = false;
								break;
							}
						}
						m_focused = el;

						m_focused->m_focused = true;
						m_focused->onFocus();

						el->m_clicked = true;
						if (m_event.button.clicks > 1) {
							el->onDoubleClick(m_event.button.button, x - el->bounds().x, y - el->bounds().y);
							el->onPress(m_event.button.button, x - el->bounds().x, y - el->bounds().y);
						} else {
							el->onPress(m_event.button.button, x - el->bounds().x, y - el->bounds().y);
						}
						if (m_elementsChanged) {
							m_elementsChanged = false;
							break;
						}
					}
					//if (cont) break;
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
						if (m_elementsChanged) {
							m_elementsChanged = false;
							break;
						}
					}

					el->m_clicked = false;
					//if (cont) break;
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
						if (m_elementsChanged) {
							m_elementsChanged = false;
							break;
						}
					}
				}
			} break;
			case SDL_MOUSEWHEEL: {
				if (m_focused != nullptr && m_focused->enabled()) {
					m_focused->onScroll(m_event.wheel.y);
				}
			} break;
			case SDL_KEYDOWN: {
				if (m_focused != nullptr) {
					m_focused->onKeyPress(m_event.key.keysym.sym, m_event.key.keysym.mod);
				}
			} break;
			case SDL_KEYUP: {
				if (m_focused != nullptr) {
					m_focused->onKeyRelease(m_event.key.keysym.sym, m_event.key.keysym.mod);
				}
			} break;
			case SDL_TEXTINPUT: {
				if (m_focused != nullptr) {
					m_focused->onType(m_event.text.text[0]);
				}
			} break;
			case SDL_WINDOWEVENT: {
				switch (m_event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
					case SDL_WINDOWEVENT_SIZE_CHANGED:
					case SDL_WINDOWEVENT_MAXIMIZED:
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_MINIMIZED:
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						status = Resize;
						break;
				}
			} break;
		}
	}
	return status;
}
