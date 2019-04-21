#ifndef SYG_GUI_H
#define SYG_GUI_H

#include <memory>

#include "../sdl2.h"
#include "events.h"
#include "renderer.h"

#include "widgets/panel.h"

class GUI {
public:
	GUI(SDL_Renderer* ren);

	template <typename T, typename... Args>
	T* create(Args&&... args) {
		T* widget = new T(std::forward<Args>(args)...);
		widget->m_gui = this;
		m_widgets.push_back(std::unique_ptr<T>(widget));
		m_events->subscribe(m_widgets.back().get());
		return dynamic_cast<T*>(m_widgets.back().get());
	}

	void destroy(Widget* widget);

	EventHandler* events() { return m_events.get(); }
	Renderer& renderer() { return *m_renderer.get(); }

	Panel* root() { return m_root; }

	void render(i32 width, i32 height);
	void clear() { m_clear = true; }

private:
	Panel* m_root;
	std::vector<std::unique_ptr<Widget>> m_widgets;
	std::unique_ptr<EventHandler> m_events;
	std::unique_ptr<Renderer> m_renderer;
	bool m_clear{ true };
};

#endif // SYG_GUI_H