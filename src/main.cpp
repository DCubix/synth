#include <iostream>

#include "common.h"
#include "sdl2.h"
#include "log/log.h"

#include "engine/node_logic.h"
#include "engine/sine_wave.hpp"
#include "engine/lfo.hpp"
#include "engine/adsr_node.hpp"
#include "engine/remap.hpp"
#include "engine/storage.hpp"

#include "gui/gui.h"
#include "gui/widgets/button.h"
#include "gui/widgets/spinner.h"
#include "gui/widgets/check.h"
#include "gui/widgets/label.h"
#include "gui/widgets/panel.h"
#include "gui/widgets/node_canvas.h"
#include "gui/widgets/keyboard.h"

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
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

	std::unique_ptr<Synth> vm = std::unique_ptr<Synth>(new Synth());
	try {
		std::unique_ptr<RtMidiIn> midin = std::unique_ptr<RtMidiIn>(new RtMidiIn(
			RtMidi::UNSPECIFIED, "Synth MIDI In"
		));
		midin->openPort(0, "SYMain In");
		midin->setCallback(&midiCallback, vm.get());
		midin->ignoreTypes();
	} catch (RtMidiError& error) {
		error.printMessage();
	}
	
	GUI* gui = new GUI(ren);
	Panel* root = gui->root();
	root->gridHeight(20);
	
	NodeCanvas* cnv = gui->create<NodeCanvas>();
	cnv->configure(0, 0, 11, 17);
	root->add(cnv);

	Label* optTitle = gui->create<Label>();
	optTitle->configure(0, 11, 5);
	optTitle->text("Node Options");
	optTitle->textAlign(Label::Center);
	root->add(optTitle);

	Panel* options = gui->create<Panel>();
	options->configure(1, 11, 5, 6);
	options->gridWidth(4);
	options->gridHeight(8);
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

	Button* newADSR = gui->create<Button>();
	newADSR->configure(8, 11, 2);
	newADSR->text("+ ADSR");
	newADSR->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<ADSRNode>();
	});
	root->add(newADSR);

	Button* newMap = gui->create<Button>();
	newMap->configure(8, 13, 2);
	newMap->text("+ Map");
	newMap->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<Map>();
	});
	root->add(newMap);

	Button* newReader = gui->create<Button>();
	newReader->configure(9, 11, 2);
	newReader->text("+ Reader");
	newReader->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<Reader>();
	});
	root->add(newReader);

	Button* newWriter = gui->create<Button>();
	newWriter->configure(9, 13, 2);
	newWriter->text("+ Writer");
	newWriter->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<Writer>();
	});
	root->add(newWriter);

	Keyboard* kb = gui->create<Keyboard>();
	kb->configure(17, 0, root->gridWidth(), 3);
	root->add(kb);

	kb->onNoteOn([&](i32 note, f32 vel) {
		vm->noteOn(note, vel);
	});

	kb->onNoteOff([&](i32 note, f32 vel) {
		vm->noteOff(note, 0.0f);
	});

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
				case NodeType::ADSR: {
					optTitle->text("Node Options (ADSR)");

					ADSRNode* an = (ADSRNode*)nd;
					f32* values[4] = { &an->a, &an->d, &an->s, &an->r };
					const std::string labels[] = { " A.", " D.", " S.", " R." };
					for (u32 i = 0; i < 4; i++) {
						Spinner* a = gui->create<Spinner>();
						a->onChange([=](f32 value) {
							ADSRNode* an = (ADSRNode*)cnv->current();
							f32* values[4] = { &an->a, &an->d, &an->s, &an->r };
							*(values[i]) = value;
						});
						a->configure(row++, 0, 4);
						a->suffix(labels[i]);
						a->minimum(0.0f);
						a->maximum(10.0f);
						a->value(*(values[i]));
						options->add(a);
					}
				} break;
				case NodeType::Map: {
					optTitle->text("Node Options (Map)");

					Map* an = (Map*)nd;
					f32* values[4] = { &an->fromMin, &an->fromMax, &an->toMin, &an->toMax };
					const std::string labels[] = { " F.Min.", " F.Max.", " T.Min.", " T.Max." };
					for (u32 i = 0; i < 4; i++) {
						Spinner* a = gui->create<Spinner>();
						a->onChange([=](f32 value) {
							Map* an = (Map*)cnv->current();
							f32* values[4] = { &an->fromMin, &an->fromMax, &an->toMin, &an->toMax };
							*(values[i]) = value;
						});
						a->configure(row++, 0, 4);
						a->suffix(labels[i]);
						a->minimum(-9999.0f);
						a->maximum(9999.0f);
						a->value(*(values[i]));
						a->draggable(false);
						options->add(a);
					}
				} break;
				case NodeType::Reader: {
					optTitle->text("Node Options (Reader)");

					Reader* rnd = (Reader*)nd;
					Spinner* a = gui->create<Spinner>();
					a->onChange([=](f32 value) {
						Reader* rnd = (Reader*)cnv->current();
						rnd->channel = u32(value);
					});
					a->configure(row++, 0, 4);
					a->suffix(" Ch.");
					a->minimum(0);
					a->maximum(32);
					a->value(rnd->channel);
					options->add(a);
				} break;
				case NodeType::Writer: {
					optTitle->text("Node Options (Writer)");

					Writer* rnd = (Writer*)nd;
					Spinner* a = gui->create<Spinner>();
					a->onChange([=](f32 value) {
						Writer* rnd = (Writer*)cnv->current();
						rnd->channel = u32(value);
					});
					a->configure(row++, 0, 4);
					a->suffix(" Ch.");
					a->minimum(0);
					a->maximum(32);
					a->value(rnd->channel);
					options->add(a);
				} break;
				default: break;
			}
		}
	});

	SDL_AudioDeviceID device;
	SDL_AudioSpec spec;
	spec.freq = 44100;
	spec.samples = 512;
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