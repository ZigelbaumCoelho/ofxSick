#pragma once

#include "ofMain.h"
#include "LMS1xx.h"

class ofxSick : public ofThread {
public:
	ofxSick();
	~ofxSick();

	void setup();
	bool isFrameNew();

protected:
	LMS1xx laser;
	scanData data;
	
	bool newFrame;
	void connect();
	void threadedFunction();
};
