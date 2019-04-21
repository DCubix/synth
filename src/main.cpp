#include <iostream>

#include "common.h"
#include "sdl2.h"
#include "log/log.h"

#include "engine/node_logic.h"
#include "engine/sine_wave.hpp"
#include "engine/lfo.hpp"

#include "gui/gui.h"
#include "gui/widgets/button.h"
#include "gui/widgets/spinner.h"
#include "gui/widgets/check.h"
#include "gui/widgets/label.h"
#include "gui/widgets/panel.h"
#include "gui/widgets/node_canvas.h"

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
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* win;
	SDL_Renderer* ren;

	win = SDL_CreateWindow(
		"Synth",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		640, 480,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

	GUI* gui = new GUI(ren);
	Panel* root = gui->root();
	
	NodeCanvas* cnv = gui->create<NodeCanvas>();
	cnv->configure(0, 0, 12, 16);
	root->add(cnv);

	Spinner* valueNode = gui->create<Spinner>();
	valueNode->onChange([&]() {
		for (u32 nid : cnv->selected()) {
			Node* node = cnv->system()->get<Node>(nid);
			node->level(valueNode->value());
		}
	});
	valueNode->configure(2, 12, 4);
	valueNode->minimum(-9999.0f);
	valueNode->maximum(9999.0f);
	valueNode->value(0.0f);
	valueNode->suffix(" Lvl.");
	root->add(valueNode);

	Spinner* spnMaster = gui->create<Spinner>();
	spnMaster->onChange([&]() {
		cnv->system()->master(spnMaster->value());
	});
	spnMaster->configure(0, 12, 4);
	spnMaster->value(0.5f);
	spnMaster->suffix(" Vol.");
	root->add(spnMaster);

	Spinner* spnFreq = gui->create<Spinner>();
	spnFreq->onChange([&]() {
		cnv->system()->frequency(spnFreq->value());
	});
	spnFreq->configure(1, 12, 4);
	spnFreq->minimum(20.0f);
	spnFreq->maximum(2000.0f);
	spnFreq->value(220.0f);
	spnFreq->suffix(" Hz");
	root->add(spnFreq);

	SDL_AudioDeviceID device;
	SDL_AudioSpec spec;
	spec.freq = 44100;
	spec.samples = 1024;
	spec.channels = 2;
	spec.callback = callback;
	spec.userdata = cnv->system();
	spec.format = AUDIO_F32;

	SDL_AudioSpec obspec;
	if ((device = SDL_OpenAudioDevice(nullptr, 0, &spec, &obspec, 0)) < 0) {
		LogE(SDL_GetError());
		return 1;
	}

	SDL_PauseAudioDevice(device, 0);

	SDL_StartTextInput();
	bool running = true;
	while (running) {
		switch (gui->events()->poll()) {
			case EventHandler::Status::Quit: running = false; break;
			case EventHandler::Status::Resize: gui->clear(); break;
			default: break;
		}
		int w, h;
		SDL_GetWindowSize(win, &w, &h);
		gui->render(w, h);
		SDL_RenderPresent(ren);
	}

	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	SDL_Quit();

	delete gui;

	return 0;
}