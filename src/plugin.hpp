#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;

extern Model* modelBlur;
extern Model* modelMorse;
extern Model* modelTraveler;
extern Model* modelTcArpGen;


template <typename TBase = GrayModuleLightWidget>
struct TOrangeLight : TBase {
	TOrangeLight() {
		this->addBaseColor(SCHEME_ORANGE);
	}
};
using OrangeLight = TOrangeLight<>;

template <typename TBase = GrayModuleLightWidget>
struct TPurpleLight : TBase {
	TPurpleLight() {
		this->addBaseColor(SCHEME_PURPLE);
	}
};
using PurpleLight = TPurpleLight<>;


struct DigitalDisplay : Widget {
	std::string fontPath;
	std::string bgText;
	std::string text;
	float fontSize;
	NVGcolor bgColor = nvgRGB(0x46,0x46, 0x46);
	NVGcolor fgColor = SCHEME_YELLOW;
	Vec textPos;

	void prepareFont(const DrawArgs& args) {
		// Get font
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		if (!font)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, fontSize);
		nvgTextLetterSpacing(args.vg, 0.0);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
	}

	void draw(const DrawArgs& args) override {
		// Background
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
		nvgFillColor(args.vg, nvgRGB(0x19, 0x19, 0x19));
		nvgFill(args.vg);

		prepareFont(args);

		// Background text
		nvgFillColor(args.vg, bgColor);
		nvgText(args.vg, textPos.x, textPos.y, bgText.c_str(), NULL);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			prepareFont(args);

			// Foreground text
			nvgFillColor(args.vg, fgColor);
			nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
		}
		Widget::drawLayer(args, layer);
	}
};

struct TextDisplay : DigitalDisplay {
	TextDisplay() {
		fontPath = asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf");
		textPos = Vec(42, 20);
		bgText = "18";
		fontSize = 12;
	}
};
