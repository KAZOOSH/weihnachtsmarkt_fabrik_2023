#pragma once
#include "ofMain.h"

namespace Utils
{

	map<string,ofTrueTypeFont> loadFonts(ofJson des);
	ofTrueTypeFont loadFont(ofJson des);
};

