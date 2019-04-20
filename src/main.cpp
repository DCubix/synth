#include <iostream>

#include "common.h"
#include "sdl2.h"

#include "log/log.h"

#include "engine/node_logic.h"
#include "engine/sine_wave.hpp"
#include "engine/lfo.hpp"

#include "gui/renderer.h"
#include "gui/events.h"
#include "gui/widgets/button.h"
#include "gui/widgets/spinner.h"

void callback(void* userdata, Uint8* stream, int len) {
	NodeSystem* sys = static_cast<NodeSystem*>(userdata);

	const int flen = len / sizeof(f32);
	f32* fstream = reinterpret_cast<f32*>(stream);

	for (int i = 0; i < flen; i+=2) {
		auto s = sys->sample();
		fstream[i + 0] = s.first;
		fstream[i + 1] = s.second;
	}
}

int main(int argc, char** argv) {
	//SDL_Init(SDL_INIT_EVERYTHING);

	/*std::unique_ptr<NodeSystem> nsys = std::make_unique<NodeSystem>();
	
	SDL_AudioDeviceID device;
	SDL_AudioSpec spec;
	spec.freq = 44100;
	spec.samples = 1024;
	spec.channels = 2;
	spec.callback = callback;
	spec.userdata = nsys.get();
	spec.format = AUDIO_F32;

	SDL_AudioSpec obspec;
	if ((device = SDL_OpenAudioDevice(nullptr, 0, &spec, &obspec, 0)) < 0) {
		LogE(SDL_GetError());
		return 1;
	}

	SDL_PauseAudioDevice(device, 0);
	SDL_Delay(2000);*/

	SDL_Window* win;
	SDL_Renderer* ren;

	win = SDL_CreateWindow(
		"Synth",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		640, 480,
		SDL_WINDOW_SHOWN
	);

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

	Renderer gren(ren);
	EventHandler events{};

	Button* elem = new Button();
	elem->bounds().x = 34;
	elem->bounds().y = 45;

	elem->onClick([elem](u8 btn, i32 x, i32 y) {
		elem->text("Clicked \x02");
	});

	events.subscribe(elem);

	Spinner* spin = new Spinner();
	spin->bounds().x = 34;
	spin->bounds().y = 80;
	spin->minimum(0.0f);
	spin->maximum(1.0f);
	spin->step(0.1f);
	spin->suffix(" Vol");
	events.subscribe(spin);

	SDL_StartTextInput();

	bool running = true;
	while (running) {
		running = events.poll();

		SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
		SDL_RenderClear(ren);

		elem->onDraw(gren);
		spin->onDraw(gren);

		SDL_RenderPresent(ren);
	}

	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	SDL_Quit();

	delete elem;
	delete spin;

	return 0;
}