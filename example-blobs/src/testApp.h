#pragma once

#include "ofMain.h"
#include "ofxSick.h"
#include "ofxCv.h"

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	void keyPressed(int key);
	
	ofxSickGrabber sick;
};