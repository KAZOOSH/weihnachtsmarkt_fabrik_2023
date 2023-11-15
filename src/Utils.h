#pragma once
#include "ofMain.h"

namespace Utils
{
	map<string,shared_ptr<ofTrueTypeFont>> loadFonts(ofJson des);
	shared_ptr<ofTrueTypeFont> loadFont(ofJson des);

	ofVec2f convertCatapultToScreenCoords(ofVec2f v, ofVec2f screenSize);
};

