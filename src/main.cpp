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

#include "../rtmidi/RtMidi.h"

enum MidiCommand {
	NoteOff = 0x8,
	NoteOn = 0x9,
	Aftertouch = 0xA,
	ContinuousController = 0xB,
	PatchChange = 0xC,
	ChannelPressure = 0xD,
	PitchBend = 0xE,
	System = 0xF
};

void midiCallback(double dt, std::vector<u8>* message, void* userdata) {
	Synth* sys = static_cast<Synth*>(userdata);

	u32 nBytes = message->size();
	if (nBytes > 3) return;

	std::array<u8, 3> data;
	for (int i = 0; i < nBytes; i++)
		data[i] = (*message)[i];

	MidiCommand command = (MidiCommand)((data[0] & 0xF0) >> 4);
	u8 channel = data[0] & 0xF;
	u8 param0 = data[1] & 0x7F;
	u8 param1 = data[2] & 0x7F;
	u32 param = (param0 & 0x7F) | (param1 & 0x7F) << 7;

	LogI("MIDI: CMD = ", (command), ", P0 = ", int(param0), ", P1 = ", int(param1), " (", param, ")");

	switch (command) {
		case MidiCommand::NoteOn: {
			if (param1 > 0) {
				sys->noteOn(param0, f32(param1) / 128.0f);
			} else {
				sys->noteOff(param0, 0.0f);
			}
		} break;
		case MidiCommand::NoteOff: sys->noteOff(param0, 0.0f); break;
		default: break;
	}
}

void callback(void* userdata, Uint8* stream, int len) {
	Synth* sys = static_cast<Synth*>(userdata);

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

	std::unique_ptr<Synth> vm = std::unique_ptr<Synth>(new Synth());
	std::unique_ptr<RtMidiIn> midin = std::unique_ptr<RtMidiIn>(new RtMidiIn(
		RtMidi::UNSPECIFIED, "Synth MIDI In"
	));
	midin->openPort(0, "SYMain In");
	midin->setCallback(&midiCallback, vm.get());
	midin->ignoreTypes();

	GUI* gui = new GUI(ren);
	Panel* root = gui->root();
	root->gridHeight(20);
	
	NodeCanvas* cnv = gui->create<NodeCanvas>();
	cnv->configure(0, 0, 11, 20);
	root->add(cnv);

	Label* optTitle = gui->create<Label>();
	optTitle->configure(0, 11, 5);
	optTitle->text("Node Options");
	optTitle->textAlign(Label::Center);
	root->add(optTitle);

	Panel* options = gui->create<Panel>();
	options->configure(1, 11, 5, 6);
	options->gridWidth(4);
	options->gridHeight(6);
	root->add(options);

	Button* newSine = gui->create<Button>();
	newSine->configure(7, 11, 2);
	newSine->text("+ Sine");
	newSine->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<SineWave>();
	});
	root->add(newSine);
	
	Button* newLFO = gui->create<Button>();
	newLFO->configure(7, 13, 2);
	newLFO->text("+ LFO");
	newLFO->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<LFO>();
	});
	root->add(newLFO);

	cnv->onConnect([&]() {
		vm->setProgram(cnv->system()->compile());
	});

	cnv->onSelect([&](Node* nd) {
		optTitle->text("Node Options");
		options->removeAll();

		//Node* nd = cnv->current();
		if (nd != nullptr) {
			if (nd->type() != NodeType::Out) {
				Button* btnDel = gui->create<Button>();
				btnDel->onClick([&](u8 btn, i32 x, i32 y) {
					if (btn == SDL_BUTTON_LEFT) {
						u32 nid = cnv->selected()[0];
						Node* nd = cnv->system()->get<Node>(nid);
						cnv->system()->destroy(nd->id());
						cnv->deselect();
					} else {
						vm->noteOn(36);
						vm->noteOn(40);
					}
				});
				btnDel->configure(5, 3);
				btnDel->text("X");
				options->add(btnDel);
			}

			u32 row = 0;

			Spinner* level = gui->create<Spinner>();
			level->configure(row++, 0, 4);
			level->value(nd->level());
			level->onChange([&](f32 value) {
				cnv->current()->level(value);
			});
			level->suffix(" Lvl.");
			level->step(0.01f);
			options->add(level);

			switch (nd->type()) {
				case NodeType::Out: optTitle->text("Node Options (Out)"); break;
				case NodeType::SineWave: optTitle->text("Node Options (Sine)"); break;
				case NodeType::LFO: {
					optTitle->text("Node Options (LFO)");
					Spinner* freq = gui->create<Spinner>();
					freq->onChange([&](f32 value) {
						cnv->current()->param(0).value = value;
					});
					freq->configure(row++, 0, 4);
					freq->suffix(" Freq.");
					freq->minimum(0.01f);
					freq->maximum(20.0f);
					freq->value(nd->param(0).value);
					options->add(freq);

					Spinner* vmin = gui->create<Spinner>();
					vmin->onChange([&](f32 value) {
						cnv->current()->param(1).value = value;
					});
					vmin->configure(row++, 0, 4);
					vmin->suffix(" Min.");
					vmin->minimum(-999.0f);
					vmin->maximum(9999.0f);
					vmin->value(nd->param(1).value);
					vmin->draggable(false);
					options->add(vmin);

					Spinner* vmax = gui->create<Spinner>();
					vmax->onChange([&](f32 value) {
						cnv->current()->param(2).value = value;
					});
					vmax->configure(row++, 0, 4);
					vmax->suffix(" Max.");
					vmax->minimum(-999.0f);
					vmax->maximum(9999.0f);
					vmax->value(nd->param(2).value);
					vmax->draggable(false);
					options->add(vmax);
				} break;
				default: break;
			}
		}
	});

	SDL_AudioDeviceID device;
	SDL_AudioSpec spec;
	spec.freq = 44100;
	spec.samples = 1024;
	spec.channels = 2;
	spec.callback = callback;
	spec.userdata = vm.get();
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
			case EventHandler::Status::Resize: gui->clear(); gui->root()->invalidate(); break;
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