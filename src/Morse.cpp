#include "plugin.hpp"
#include <osdialog.h>

#define NUM_ENTRIES(array)  int(sizeof(array)/sizeof(array[0]))

const std::string FILE_NOT_SELECTED_STRING = "<empty>";

//const int GAP_BETWEEN_LETTERS = 3;
//const int GAP_BETWEEN_WORDS = 7;

const std::string ERROR_CODE_SEQUENCE = "........";

struct MorseLetter { 
	char letter;
	std::string code_sequence; 
};

MorseLetter alpha[] = {
	// from https://morsecode.world/international/morse2.html
	// also https://en.wikipedia.org/wiki/Morse_code
	{ 'A', ".-" },
	{ 'B', "-..." },
	{ 'C', "-.-." },
	{ 'D', "-.." },
	{ 'E', "." },
	{ 'F', "..-." },
	{ 'G', "--." },
	{ 'H', "...." },
	{ 'I', ".." },
	{ 'J', ".---" },
	{ 'K', "-.-" },
	{ 'L', ".-.." },
	{ 'M', "--" },
	{ 'N', "-." },
	{ 'O', "---" },
	{ 'P', ".--." },
	{ 'Q', "--.-" },
	{ 'R', ".-." },
	{ 'S', "..." },
	{ 'T', "-" },
	{ 'U', "..-" },
	{ 'V', "...-" },
	{ 'W', ".--" },
	{ 'X', "-..-" },
	{ 'Y', "-.--" },
	{ 'Z', "--.." }
};

MorseLetter numeric[] = {
	{ '0', "-----" },
	{ '1', ".----" },
	{ '2', "..---" },
	{ '3', "...--" },
	{ '4', "....-" },
	{ '5', "....." },
	{ '6', "-...." },
	{ '7', "--..." },
	{ '8', "---.." },
	{ '9', "----." }
};

MorseLetter punct[] = {
	{ '&', ".-..." },
	{ '\'', ".----." },
	{ '@', ".--.-." },
	{ ')', "-.--.-" },
	{ '(', "-.--." },
	{ ':', "---..." },
	{ ',', "--..--" },
	{ '=', "-...-" },
	{ '!', "-.-.--" },
	{ '.', ".-.-.-" },
	{ '-', "-...-" },
	{ '*', "-..-" },            // same as 'X'
	{ '%', "------..-.-----" }, // 0/0
	{ '+', ".-.-." },
	{ '"', ".-..-." },
	{ '?', "..--.." },
	{ '/', "-..-." },
	{ '_', "..--.-" },
	{ '$', "...-..-" },
};

// TODO: add international accented letters ??
// #include <iostream>
#include <fstream>
struct BinaryFileReader { 
	std::ifstream stream;
	std::string path;
	int filesize; 
	int pos;
	int direction = 1; // 1 = fwd, -1 = reverse
	int at_end = 1; // 1 = wrap, -1 = bounce

	BinaryFileReader() { 
	}
	
	~BinaryFileReader() { 
		close();
	}
	
	bool open(std::string filepath) {
		close();
		path = filepath; 
		pos = 0;
		filesize = 0;
		stream.open(path.c_str(), (std::ifstream::in | std::ifstream::binary));
		if (stream) {
			// get file length
			stream.seekg (0, stream.end);
    		filesize = stream.tellg();
		    stream.seekg (0, stream.beg);
		}
		return stream.is_open();		
	}

	void close() {
		if (stream.is_open()) {
			stream.close();
		}
	}

	bool isOpen() const {
		return stream.is_open();
	}

	char getChar() { 
		char ch = -1;
		if (stream.is_open()) {
			if (direction > 0) { 
				movePositionForward();
			} else { 
				movePositionBackward();
			}
			ch = stream.get();
			pos = stream.tellg();
		}
		return ch;
	}

	void movePositionForward() {
		if (stream.eof()) {
			if (at_end > 0) {  // wrap
				setPositionToBegin();
			} else {  // bounce 
				direction = -1; // start going backwards 
				setPositionToEnd();
			}
		} else {
			pos++;
			stream.seekg(pos, stream.beg);
		}
	}	

	void movePositionBackward() {
		if (pos <= 0) { 
			if (at_end > 0) { // wrap 
				setPositionToEnd();
			} else { // bounce
				direction = 1; // start going forward
				setPositionToBegin();
				} 
		} else {
			pos --;
			stream.seekg(pos, stream.beg);
		}
	}	


	void setPositionToEnd() {
		stream.seekg(0, stream.end);
		pos = stream.tellg();
	}

	void setPositionToBegin() {
		stream.seekg(0, stream.end);
		pos = stream.tellg();
	}

	void setPositionByPercent(float zeroToOne) {
		if (stream) {
			pos = int(float(filesize) * zeroToOne);
			clamp(pos, 0, filesize);
			stream.seekg(pos, stream.beg);
		}
	}
};


struct Morse : Module {
	enum ParamIds {
		DIRECTION_KNOB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_CV_INPUT,
		SEMITONE_CV_INPUT,
		LOOP_CV_INPUT,
		BOUNCE_CV_INPUT,
		AUDIO_IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		FILE_LOADED_LED_LIGHT,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger clockTrigger;

	std::string filepath = FILE_NOT_SELECTED_STRING;
	// fstream 
	// position ??

    std::string active_sequence;
	int sequenceIndex;
	//int gapCount;

	std::string lookup_table[256];

	void initializeLookupTable() {
		for (int k = 0; k < NUM_ENTRIES(alpha); k++) {
			int ch = alpha[k].letter;
			lookup_table[tolower(ch)] = dotDashToGates(alpha->code_sequence); 
			lookup_table[toupper(ch)] = dotDashToGates(alpha->code_sequence); 
		}
		for (int k = 0; k < NUM_ENTRIES(numeric); k++) {
			int ch = numeric[k].letter;
			lookup_table[ch] = dotDashToGates(numeric->code_sequence); 
		}
		for (int k = 0; k < NUM_ENTRIES(punct); k++) {
			int ch = punct[k].letter;
			lookup_table[ch] = dotDashToGates(punct->code_sequence); 
		}
	}

	std::string dotDashToGates(std::string dotDashSequence) {
		std::string result; 

		int len = dotDashSequence.length();
		for (int k = 0; k < len; k++) {
			if (k > 0) {
				result += '0';   // single gap between dot/dash symbols 
			}
			if (dotDashSequence[k] == '.') {
				result += '1';   // 1 time unit for dots 
			}
			else if (dotDashSequence[k] == '-') {
				result += "111"; // 3 time unit for dash 
			}
		}
		return result;
	}

	void initializeSequence() { 
		active_sequence.clear();
		sequenceIndex = 0;
		//gapCount = 0;
	}

	Morse() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(DIRECTION_KNOB_PARAM, 0.f, 1.f, 1.f, "");

		initializeLookupTable();
        initializeSequence();
	}

	virtual void onReset() override { 
        initializeSequence();
	}

	virtual json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "filepath", json_string(filepath.c_str()));
		return rootJ;
	}

	virtual void dataFromJson(json_t* rootJ) override {
		json_t* jsonValue;
        
        jsonValue = json_object_get(rootJ, "filepath");
		if (jsonValue)
			filepath = json_string_value(jsonValue);
	}

	void setPath(std::string path) {
		if (this->filepath == path)
			return;

		closeFile();

		if (path == "") {
			this->filepath = FILE_NOT_SELECTED_STRING;
		} else {
			this->filepath = path;
			openFile();		
		}


	}

	void closeFile() {
		// TODO: close the file if it is open 
	}

	void openFile() {
		// TODO: open the file for reading 
	}

    int getNextCharacter() {
		// TODO: get this from a file
		// for now just return a random value 
		return int(rand() % 127);
	}

	void process(const ProcessArgs& args) override {

		if (inputs[CLOCK_CV_INPUT].isConnected() == false) {
			// TODO: out = 0
			return;
		}


		bool clockChanged = false;
		bool gateHighBefore = clockTrigger.isHigh();
		if (clockTrigger.process(inputs[CLOCK_CV_INPUT].getVoltage())) {
			clockChanged = gateHighBefore != clockTrigger.isHigh();
		}

        if (clockChanged) {
			// advance to next symbol in the sequence 
			// at end of sequence 
			sequenceIndex++;
			if (sequenceIndex >= int(active_sequence.length())) {
				int ch = getNextCharacter();

				if (ch == ' ' || ch == '\t' || ch == '\n') {
					active_sequence = "0000000"; // gap 7 between words  
				}
				else if (lookup_table[ch].length() > 0)	{
					active_sequence = "000" + lookup_table[ch]; // 3 unit gap between letters
				} else {
					active_sequence = "1010101010101"; // ERROR_CODE_SEQUENCE;
				}
				sequenceIndex = 0;
			}
		}

		char bit = active_sequence[ sequenceIndex ];
		float out = (bit == '1' ? 10.0 : 0.0);

		// DEBUG("-OUT=%f",out);
        outputs[GATE_OUT_OUTPUT].setVoltage(out);

		// TODO: open file 
		// loop:
		//   if clock is high:
		//      if gapCount > 0
		//         decrement gap count
		//         if gap count == 0
		//             if no sequence
		//                ch = get next character (based on direction)
		//                if ch == ' '
		//                   set gapCount = 7
		//                else
		//                   code sequence = lookup_table[ char ]
		//                   if no sequence
		//                      seq = error_sequence
		//                   seq_index = 0
		//             play next symbol in sequence
		//             advance symbol index
		//             if end of sequence
		//                set gap count = 3

	}


};


static void selectPath(Morse* module) {
	std::string dir;
	std::string filename;
	if (module->filepath != "") {
		dir = string::directory(module->filepath);
		filename = string::filename(module->filepath);
	}
	else {
		dir = asset::user("");
		filename = "Untitled";
	}

	char* path = osdialog_file(OSDIALOG_SAVE, dir.c_str(), filename.c_str(), NULL);
	if (path) {
		module->setPath(path);
		free(path);
	}
}

struct MorseSelectFile : MenuItem
{
	Morse *module;

	void onAction(const event::Action &e) override
	{
		const std::string dir = "";
		char *path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, NULL);

		if(path)
		{
			module->setPath(path);
//		      module->sample.load(path);
//			  module->root_dir = std::string(path);
//			  module->loaded_filename = module->sample.filename;
			free(path);
		}
	}
};
struct MorseWidget : ModuleWidget {
	MorseWidget(Morse* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Morse.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

// TODO restore this - hidden for now while work-in-progress
//		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.919, 33.895)), module, Morse::DIRECTION_KNOB_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.608, 16.311)), module, Morse::CLOCK_CV_INPUT));
// TODO restore this - hidden for now while work-in-progress
//		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.099, 48.372)), module, Morse::SEMITONE_CV_INPUT));
//		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.608, 73.046)), module, Morse::LOOP_CV_INPUT));
//		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.608, 82.519)), module, Morse::BOUNCE_CV_INPUT));
//		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.608, 91.971)), module, Morse::AUDIO_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.165, 111.237)), module, Morse::GATE_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(26.151, 101.547)), module, Morse::FILE_LOADED_LED_LIGHT));
	}

	
	void appendContextMenu(Menu *menu) override
	{
		Morse *module = dynamic_cast<Morse*>(this->module);
		assert(module);

        menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Data File"));

		MorseSelectFile *menu_item = new MorseSelectFile();
		menu_item->text = module->filepath;
		menu_item->module = module;
		menu->addChild(menu_item);
	}
};

Model* modelMorse = createModel<Morse, MorseWidget>("Morse");