#pragma once
#include <vector>
#include <unordered_map>

class PrismInputs
{
public:
	PrismInputs();
	double lmx = 0, lmy = 0, dmx = 0, dmy = 0;
	bool nodata = true;
	std::vector<int> valid_keys;
	std::vector<bool> pressed_keys;
	void newMpos(double xpos, double ypos);
	void updateKeyPressState(int key, int scancode, int action, int mods);
	void clearKeyPressState(int key);
	bool wasKeyPressed(int key);
	void clearMOffset();
};

