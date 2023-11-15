#include "Utils.h"

namespace Utils {

	map<string, shared_ptr<ofTrueTypeFont>> loadFonts(ofJson des)
	{
		map<string, shared_ptr<ofTrueTypeFont>> ret;
		for (auto& el : des.items()) {
			ret.insert(make_pair(el.key(), loadFont(el.value())));
		}
		return ret;
	}

	shared_ptr<ofTrueTypeFont> loadFont(ofJson des)
	{
		shared_ptr<ofTrueTypeFont> ret = shared_ptr<ofTrueTypeFont>(new ofTrueTypeFont());
		ret->load(des["font"].get<string>(), des["size"].get<int>(), true, true);
		return ret;
	}

	ofVec2f convertCatapultToScreenCoords(ofVec2f v, ofVec2f screenSize){
		return ofVec2f(
                    ofMap(v.x, 0, 100, 0, screenSize.x),
                    ofMap(v.y, 55, 100, screenSize.y, 0));
	}
}
