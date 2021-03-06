#include <iostream>
#include <fstream>
#include <sstream>

#include "common.h"
#include "sdl2.h"
#include "log/log.h"

#include "engine/node_logic.h"
#include "engine/sine_wave.hpp"
#include "engine/lfo.hpp"
#include "engine/adsr_node.hpp"
#include "engine/remap.hpp"
#include "engine/storage.hpp"
#include "engine/synth_vm.h"

#ifndef SYN_PERF_TESTS
#	include "gui/gui.h"
#	include "gui/widgets/button.h"
#	include "gui/widgets/spinner.h"
#	include "gui/widgets/check.h"
#	include "gui/widgets/label.h"
#	include "gui/widgets/panel.h"
#	include "gui/widgets/keyboard.h"
#	include "node_canvas.h"
#	include "../osdialog/OsDialog.hpp"

#include "../rtmidi/RtMidi.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

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

struct MidiUserData {
	MidiUserData() = default;
	~MidiUserData() = default;
	Keyboard* kb;
	Synth* sys;
};

void midiCallback(double dt, std::vector<u8>* message, void* userdata) {
	MidiUserData* udat = static_cast<MidiUserData*>(userdata);

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

	// LogI("MIDI: CMD = ", (command), ", P0 = ", int(param0), ", P1 = ", int(param1), " (", param, ")");

	switch (command) {
		case MidiCommand::NoteOn: {
			if (param1 == 0) {
				udat->kb->noteOff(param0, 0.0f);
			} else {
				udat->kb->noteOn(param0, f32(param1) / 128.0f);
			}
		} break;
		case MidiCommand::NoteOff: udat->kb->noteOff(param0, 0.0f); break;
		case MidiCommand::ContinuousController:
			switch (param0) {
				default: break;
				case 64: {
					param1 > 0 ? udat->sys->sustainOn() : udat->sys->sustainOff();
				} break;
			}
			break;
		default: break;
	}
}

void callback(void* userdata, Uint8* stream, int len) {
	Synth* sys = static_cast<Synth*>(userdata);

	const u32 ulen = u32(len) / sizeof(f32);
	f32* ustream = reinterpret_cast<f32*>(stream);

	const u32 step = 2 * SynSampleCount;
	for (u32 i = 0; i < ulen; i += step) {
		auto L = sys->sample(0);
		auto R = sys->sample(1);
		for (u32 j = 0; j < step; j += 2) {
			ustream[i + j + 0] = L[j/2];
			ustream[i + j + 1] = R[j/2];
		}
	}
}
#endif

int main(int argc, char** argv) {
	bool saved = true;
	std::string savedFile = "";
	std::unique_ptr<Synth> vm = std::make_unique<Synth>();

#ifndef SYN_PERF_TESTS
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* win;
	SDL_GLContext ctx;

	win = SDL_CreateWindow(
		"Synth",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
	);

	ctx = SDL_GL_CreateContext(win);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetSwapInterval(0);

	GUI* gui = new GUI();
	Panel* root = gui->root();
	root->setLayout(new BorderLayout());
	root->spacing(4);
	root->padding(4);

	Keyboard* kb = gui->create<Keyboard>();
	MidiUserData udata = { kb, vm.get() };

	//// MIDI Setup
	std::unique_ptr<RtMidiIn> midin;
	try {
		midin = std::make_unique<RtMidiIn>(
			RtMidi::UNSPECIFIED, "Synth MIDI In"
		);
		midin->openPort(0, "SYMain In");
		midin->setCallback(&midiCallback, &udata);
		midin->ignoreTypes();
		LogI("Initialized MIDI");
	} catch (RtMidiError& error) {
		LogE(error.getMessage());
	}
	LogI("MIDI Ports: ", midin->getPortCount());
	for (u32 i = 0; i < midin->getPortCount(); i++) {
		LogI("\tPort ", i, ": ", midin->getPortName(i));
	}
	//

	NodeCanvas* cnv = gui->create<NodeCanvas>();
	cnv->layoutParam(BorderLayoutPosition::Center);
	root->add(cnv);

	Panel* topPanel = gui->create<Panel>();
	topPanel->drawBackground(false);
	topPanel->padding(1);
	topPanel->setLayout(new FlowLayout());
	topPanel->layoutParam(BorderLayoutPosition::Top);
	topPanel->bounds().height = 22;
	root->add(topPanel);

	Panel* sidePanel = gui->create<Panel>();
	sidePanel->drawBackground(false);
	sidePanel->padding(0);
	sidePanel->setLayout(new BorderLayout());
	sidePanel->layoutParam(BorderLayoutPosition::Right);
	sidePanel->bounds().width = 200;
	root->add(sidePanel);

	Label* optTitle = gui->create<Label>();
	optTitle->layoutParam(BorderLayoutPosition::Top);
	optTitle->bounds().height = 22;
	optTitle->text("Node Options");
	optTitle->textAlign(Label::Center);

	Panel* optionsPanel = gui->create<Panel>();
	optionsPanel->padding(0);
	optionsPanel->spacing(0);
	optionsPanel->setLayout(new BorderLayout());
	optionsPanel->layoutParam(BorderLayoutPosition::Top);
	optionsPanel->bounds().height = 240;
	sidePanel->add(optionsPanel);

	Panel* nodeButtonsPanel = gui->create<Panel>();
	nodeButtonsPanel->layoutParam(BorderLayoutPosition::Bottom);
	nodeButtonsPanel->bounds().height = 36;
	nodeButtonsPanel->gridWidth(6);
	nodeButtonsPanel->gridHeight(2);
	optionsPanel->add(nodeButtonsPanel);

	Panel* options = gui->create<Panel>();
	options->setLayout(new StackLayout());
	options->layoutParam(BorderLayoutPosition::Center);
	optionsPanel->add(options);

	optionsPanel->add(optTitle);

	Panel* buttonsPanel = gui->create<Panel>();
	buttonsPanel->layoutParam(BorderLayoutPosition::Bottom);
	buttonsPanel->bounds().height = 128;
	buttonsPanel->gridWidth(6);
	buttonsPanel->gridHeight(4);
	sidePanel->add(buttonsPanel);

	Button* newSine = gui->create<Button>();
	newSine->configure(0, 0, 2);
	newSine->text("Sine");
	newSine->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<SineWave>();
	});
	buttonsPanel->add(newSine);

	Button* newLFO = gui->create<Button>();
	newLFO->configure(0, 2, 2);
	newLFO->text("LFO");
	newLFO->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<LFO>();
	});
	buttonsPanel->add(newLFO);

	Button* newADSR = gui->create<Button>();
	newADSR->configure(0, 4, 2);
	newADSR->text("ADSR");
	newADSR->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<ADSRNode>();
	});
	buttonsPanel->add(newADSR);

	Button* newMap = gui->create<Button>();
	newMap->configure(1, 0, 2);
	newMap->text("Map");
	newMap->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<Map>();
	});
	buttonsPanel->add(newMap);

	Button* newValue = gui->create<Button>();
	newValue->configure(1, 2, 2);
	newValue->text("Value");
	newValue->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<Value>();
	});
	buttonsPanel->add(newValue);

	Button* newMul = gui->create<Button>();
	newMul->configure(1, 4, 2);
	newMul->text("Mul");
	newMul->onClick([&](u8 btn, i32 x, i32 y) {
		cnv->create<Mul>();
	});
	buttonsPanel->add(newMul);

	Label* lblFile = gui->create<Label>();
	lblFile->text("*");
	lblFile->autoWidth(true);

	auto onChange = [&]() { saved = false; lblFile->text(savedFile + "*"); };

	// EFFECTS
	Panel* effectsPanel = gui->create<Panel>();
	effectsPanel->padding(2);
	effectsPanel->setLayout(new StackLayout());
	effectsPanel->layoutParam(BorderLayoutPosition::Center);
	sidePanel->add(effectsPanel);

	Label* lblFXTitle = gui->create<Label>();
	lblFXTitle->bounds().height = 22;
	lblFXTitle->text("Effects");
	lblFXTitle->textAlign(Label::Center);
	effectsPanel->add(lblFXTitle);

	// CHORUS
	Panel* chorusPanel = gui->create<Panel>();
	chorusPanel->padding(2);
	chorusPanel->spacing(2);
	chorusPanel->setLayout(new StackLayout());
	chorusPanel->bounds().height = 128;
	effectsPanel->add(chorusPanel);

	Check* lblChTitle = gui->create<Check>();
	lblChTitle->bounds().height = 20;
	lblChTitle->text("Chorus");
	chorusPanel->add(lblChTitle);
	lblChTitle->onChecked([&](bool v) {
		vm->chorusEnabled(v);
		onChange();
	});

	Spinner* spnDelay = gui->spinner(
		&vm->chorusEffect().delay, 10.0f, 50.0f,
		" Delay", true, onChange
	);
	chorusPanel->add(spnDelay);

	Spinner* spnWidth = gui->spinner(
		&vm->chorusEffect().width, 10.0f, 50.0f,
		" Width", true, onChange
	);
	chorusPanel->add(spnWidth);

	Spinner* spnDepth = gui->spinner(
		&vm->chorusEffect().depth, 0.0f, 1.0f,
		" Depth", true, onChange
	);
	chorusPanel->add(spnDepth);

	Spinner* spnVoices = gui->spinner(
		&vm->chorusEffect().numVoices, 2.0f, 5.0f,
		" Voices", false, onChange, 1.0f
	);
	chorusPanel->add(spnVoices);

	Spinner* spnFreq = gui->spinner(
		&vm->chorusEffect().freq, 0.05f, 2.0f,
		" Freq.", true, onChange, 0.1f
	);
	chorusPanel->add(spnFreq);
	//

	auto newFileFunc = [&]() {
		cnv->system()->clear();
		vm->setProgram(cnv->system()->compile());
		cnv->invalidate();
		savedFile = "";
		saved = true;
		lblFile->text("*");
	};

	Button* btNew = gui->create<Button>();
	btNew->bounds().width = 42;
	btNew->text("New");
	btNew->onClick([&](u8 btn, i32 x, i32 y) {
		if (!saved) {
			auto res = osd::Dialog::message(
				osd::MessageLevel::Warning,
				osd::MessageButtons::YesNo,
				"Unsaved Program",
				"You program has not been saved, and your changes will be lost. Do you wish to continue?"
			);
			if (res == osd::Dialog::Yes) {
				newFileFunc();
			}
		} else {
			newFileFunc();
		}
	});
	topPanel->add(btNew);

	auto openFileFunc = [&]() {
		auto res = osd::Dialog::file(
			osd::DialogAction::OpenFile,
			".",
			osd::Filters("Synth Program:sprog")
		);
		if (res.has_value()) {
			Json json = util::pack::loadFile(res.value());
			cnv->load(json);
			vm->setProgram(cnv->system()->compile());

			// Load Effects...
			if (json["chorus"].is_object()) {
				Json chorus = json["chorus"];
				vm->chorusEnabled(chorus.value("enabled", false));
				vm->chorusEffect().depth = chorus.value("depth", 0.0f);
				vm->chorusEffect().width = chorus.value("width", 10.0f);
				vm->chorusEffect().delay = chorus.value("delay", 10.0f);
				vm->chorusEffect().freq = chorus.value("freq", 0.2f);
				vm->chorusEffect().numVoices = chorus.value("numVoices", 2.0f);

				lblChTitle->checked(vm->chorusEnabled());
			}

			cnv->invalidate();
			savedFile = res.value();
			saved = true;
			lblFile->text(savedFile);
		}
	};

	Button* btOpen = gui->create<Button>();
	btOpen->bounds().width = 42;
	btOpen->text("Open");
	btOpen->onClick([&](u8 btn, i32 x, i32 y) {
		if (!saved) {
			auto res = osd::Dialog::message(
				osd::MessageLevel::Warning,
				osd::MessageButtons::YesNo,
				"Unsaved Program",
				"You program has not been saved, and your changes will be lost. Do you wish to continue?"
			);
			if (res == osd::Dialog::Yes) {
				openFileFunc();
			}
		} else {
			openFileFunc();
		}
	});
	topPanel->add(btOpen);

	auto saveFileFunc = [&](const std::string& fileName) {
		Json json; //cnv->save(json);

		// Save Effects...
		json["chorus"]["enabled"] = vm->chorusEnabled();
		json["chorus"]["depth"] = vm->chorusEffect().depth;
		json["chorus"]["width"] = vm->chorusEffect().width;
		json["chorus"]["delay"] = vm->chorusEffect().delay;
		json["chorus"]["freq"] = vm->chorusEffect().freq;
		json["chorus"]["numVoices"] = vm->chorusEffect().numVoices;

		util::pack::saveFile(fileName, json);
		saved = true;
		savedFile = fileName;
		lblFile->text(savedFile);
	};

	Button* btSave = gui->create<Button>();
	btSave->bounds().width = 42;
	btSave->text("Save");
	btSave->onClick([&](u8 btn, i32 x, i32 y) {
		if (savedFile.empty()) {
			auto res = osd::Dialog::file(
				osd::DialogAction::SaveFile,
				".",
				osd::Filters("Synth Program:sprog")
			);
			if (res.has_value()) {
				saveFileFunc(res.value());
			}
		} else {
			saveFileFunc(savedFile);
		}
	});
	topPanel->add(btSave);

	Button* btSaveAs = gui->create<Button>();
	btSaveAs->bounds().width = 64;
	btSaveAs->text("Save As");
	btSaveAs->onClick([&](u8 btn, i32 x, i32 y) {
		savedFile = "";
		btSave->onClick(btn, 2, 2);
	});
	topPanel->add(btSaveAs);

	topPanel->add(lblFile);

	// Add Keyboard
	kb->bounds().height = 64;
	kb->layoutParam(BorderLayoutPosition::Bottom);
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

	cnv->onChange([&]() {
		saved = false;
	});

	cnv->onSelect([&](Node* nd) {
		optTitle->text("Node Options");
		options->removeAll();
		nodeButtonsPanel->removeAll();

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
						vm->setProgram(cnv->system()->compile());
						saved = false;
						cnv->invalidate();
						lblFile->text(savedFile + "*");
					}
				});
				btnDel->configure(0, 3, 3, 2);
				btnDel->text("Delete");
				nodeButtonsPanel->add(btnDel);
			}

			u32 row = 0;

			Spinner* level = gui->create<Spinner>();
			level->configure(row++, 0, 4);
			level->value(nd->level());
			level->onChange([&](f32 value) {
				cnv->current()->level(value);
				saved = false;
			});
			level->suffix(" Lvl.");
			level->step(0.01f);
			options->add(level);

			Spinner* pan = gui->create<Spinner>();
			pan->configure(row++, 0, 4);
			pan->value(nd->pan());
			pan->onChange([&](f32 value) {
				cnv->current()->pan(value);
				saved = false;
			});
			pan->suffix(" Pan");
			pan->step(0.01f);
			options->add(pan);

			switch (nd->type()) {
				case NodeType::Out: optTitle->text("Node Options (Out)"); break;
				case NodeType::SineWave: optTitle->text("Node Options (Sine)"); break;
				case NodeType::Value: {
					optTitle->text("Node Options (Value)");
					Value* val = (Value*)nd;
					Spinner* a = gui->spinner(
						&val->value, -9999, 9999,
						" Val.", true, onChange
					);
					a->configure(row++, 0, 4);
					options->add(a);
				} break;
				case NodeType::LFO: {
					optTitle->text("Node Options (LFO)");

					Spinner* freq = gui->spinner(
						&cnv->current()->param(0).value, 0.01f, 100.0f,
						" Freq.", true,	onChange
					);
					freq->configure(row++, 0, 4);
					options->add(freq);

					Spinner* vmin = gui->spinner(
						&((LFO*)cnv->current())->min, -9999, 9999,
						" Min.", false, onChange
					);
					vmin->configure(row++, 0, 4);
					options->add(vmin);

					Spinner* vmax = gui->spinner(
						&((LFO*)cnv->current())->max, -9999, 9999,
						" Min.", false, onChange
					);
					vmax->configure(row++, 0, 4);
					options->add(vmax);
				} break;
				case NodeType::ADSR: {
					optTitle->text("Node Options (ADSR)");

					ADSRNode* an = (ADSRNode*)nd;
					f32* values[4] = { &an->a, &an->d, &an->s, &an->r };
					const std::string labels[] = { " A.", " D.", " S.", " R." };

					for (u32 i = 0; i < 4; i++) {
						Spinner* a = gui->spinner(
							values[i], 0, i == 2 ? 1 : 10,
							labels[i], true, onChange
						);
						a->configure(row++, 0, 4);
						options->add(a);
					}
				} break;
				case NodeType::Map: {
					optTitle->text("Node Options (Map)");

					Map* an = (Map*)nd;
					f32* values[4] = { &an->fromMin, &an->fromMax, &an->toMin, &an->toMax };
					const std::string labels[] = { " F.Min.", " F.Max.", " T.Min.", " T.Max." };
					for (u32 i = 0; i < 4; i++) {
						Spinner* a = gui->spinner(
							values[i], -9999, 9999,
							labels[i], false, onChange
						);
						a->configure(row++, 0, 4);
						options->add(a);
					}
				} break;
				default: break;
			}
		}
	});

	SDL_AudioDeviceID device;
	SDL_AudioSpec spec;
	spec.freq = 44100;
	spec.samples = 256;
	spec.channels = 2;
	spec.callback = callback;
	spec.userdata = vm.get();
	spec.format = AUDIO_F32;

	int count = SDL_GetNumAudioDevices(0);
	for (int i = 0; i < count; ++i) {
		LogI("AUDIO DEVICE #", i, ": ", SDL_GetAudioDeviceName(i, 0));
	}

	SDL_AudioSpec obspec;
	if ((device = SDL_OpenAudioDevice(nullptr, 0, &spec, &obspec, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 0) {
		LogE(SDL_GetError());
		return 1;
	}

	SDL_PauseAudioDevice(device, 0);
#endif

#ifdef SYN_PERF_TESTS
#	include "wav.h"

	const std::string testSynth =
		#include "rhodes.h"
			;
	Json json = Json::parse(testSynth);

	NodeSystem* sys = new NodeSystem();

	Json nodes = json["nodes"];
	Json connections = json["connections"];

	for (int i = 0; i < nodes.size(); i++) {
		int id = 0;
		Json node = nodes[i];
		auto type = node["type"].get<std::string>();

		if (type == "sine") {
			id = sys->create<SineWave>();
		} else if (type == "lfo") {
			id = sys->create<LFO>();
		} else if (type == "adsr") {
			id = sys->create<ADSRNode>();
		} else if (type == "map") {
			id = sys->create<Map>();
		} else if (type == "value") {
			id = sys->create<Value>();
		} else if (type == "mul") {
			id = sys->create<Mul>();
		} else continue;

		sys->get<Node>(id)->load(node);
	}

	// Connect
	for (int i = 0; i < connections.size(); i++) {
		Json conn = connections[i];
		sys->connect(conn["src"], conn["dest"], conn["destParam"]);
	}

	// Load Effects...
	if (json["chorus"].is_object()) {
		Json chorus = json["chorus"];
		vm->chorusEnabled(chorus.value("enabled", false));
		vm->chorusEffect().depth = chorus.value("depth", 0.0f);
		vm->chorusEffect().width = chorus.value("width", 10.0f);
		vm->chorusEffect().delay = chorus.value("delay", 10.0f);
		vm->chorusEffect().freq = chorus.value("freq", 0.2f);
		vm->chorusEffect().numVoices = chorus.value("numVoices", 2.0f);
	}
	vm->setProgram(sys->compile());

	vm->noteOn(48);
	vm->noteOn(52);
	vm->noteOn(55);
	vm->noteOn(59);
	vm->noteOn(62);
	vm->noteOn(65);
	vm->noteOn(69);
	vm->noteOn(72);
	vm->noteOn(76);
	vm->noteOn(79);
	vm->noteOn(83);

	// Generate samples
	constexpr u32 Seconds = 10;
	std::array<f32, Seconds * 44100 * 2> buf;

	const u32 step = 2 * SynSampleCount;
	for (u32 i = 0; i < buf.size(); i += step) {
		auto L = vm->sample(0);
		auto R = vm->sample(1);
		for (u32 j = 0; j < step; j += 2) {
			buf[i + j + 0] = L[j/2];
			buf[i + j + 1] = R[j/2];
		}
	}

	delete sys;
#else
	const double timeStep = 1.0 / 30.0;
	double lastTime = double(SDL_GetTicks()) / 1000.0;
	double accum = 0.0;

	SDL_StartTextInput();

	bool running = true;
	while (running) {
		bool canRender = false;
		double current = double(SDL_GetTicks()) / 1000.0;
		double delta = current - lastTime;
		lastTime = current;
		accum += delta;

		switch (gui->events()->poll()) {
			case EventHandler::Status::Quit: {
				if (!saved) {
					auto res = osd::Dialog::message(
						osd::MessageLevel::Warning,
						osd::MessageButtons::YesNo,
						"Unsaved Program",
						"You program has not been saved, and your changes will be lost. Do you wish to continue?"
					);
					if (res == osd::Dialog::Yes) {
						running = false;
					}
				} else {
					running = false;
				}
			} break;
			case EventHandler::Status::Resize: gui->clear(); gui->root()->invalidate(); break;
			default: break;
		}

		while (accum > timeStep) {
			accum -= timeStep;
			canRender = true;
		}

		if (canRender) {
			int w, h;
			SDL_GetWindowSize(win, &w, &h);
			gui->render(w, h);
			SDL_GL_SwapWindow(win);
		}
	}
	SDL_PauseAudioDevice(device, 1);
	SDL_CloseAudioDevice(device);

	SDL_GL_DeleteContext(ctx);
	SDL_DestroyWindow(win);
	SDL_Quit();

	delete gui;
#endif
	return 0;
}
