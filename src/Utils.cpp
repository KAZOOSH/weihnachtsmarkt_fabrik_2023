#include "Utils.h"

namespace Utils {
	map<string, ofTrueTypeFont> loadFonts(ofJson des)
	{
		map<string, ofTrueTypeFont> ret;
		for (auto& el : des.items()) {
			ret.insert(make_pair(el.key(), loadFont(el.value())));
		}
		return ret;
	}
	ofTrueTypeFont loadFont(ofJson des)
	{
		ofTrueTypeFont ret;
		ret.load(des["font"].get<string>(), des["size"].get<int>(), true, true);
		return ret;
	}
}
