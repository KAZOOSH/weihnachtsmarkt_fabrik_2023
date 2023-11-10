#pragma once
#include "ofMain.h"

namespace Utils
{
	map<string,shared_ptr<ofTrueTypeFont>> loadFonts(ofJson des);
	shared_ptr<ofTrueTypeFont> loadFont(ofJson des);
};

