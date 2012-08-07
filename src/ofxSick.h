#pragma once

#include "ofMain.h"
#include "LMS1xx.h"

class ofxSick : public ofThread {
public:
	ofxSick();
	~ofxSick();

	void setup();
	void update();
	
	LMS1xx laser;
	scanData data;
};
