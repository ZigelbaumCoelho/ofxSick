#pragma once

#include "ofMain.h"
#include "ofxSick.h"

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	vector<ofPtr<ofxSickGrabber> > sick;
	vector<ofVec2f> positionOffsets;
	vector<ofVec2f> points;
	ofMesh pointCloud;
	float rangeNearAngle;
	ofRectangle rangeNear;
	float rangeFarRadius;
};