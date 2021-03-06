#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	// p->addModel(modelMyModule);

	p->addModel(modelBlur);
	p->addModel(modelMorse);
	p->addModel(modelTraveler);
	//p->addModel(modelTravelerExpander);
	//p->addModel(modelWaterfall);
	//p->addModel(modelHarmonyRotator);
	//p->addModel(modelHarmonySequencer);
	
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
