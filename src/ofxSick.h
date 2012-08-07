#pragma once

#include "ofMain.h"
#include "LMS1xx.h"

class PulseData {
public:
	vector<unsigned short> distance, brightness;
};

class ScanData {
public:
	PulseData first, second;
};

class ofxSick : public ofThread {
public:
	ofxSick();
	~ofxSick();

	void setup();
	void update();
	bool isFrameNew();
	
	const vector<unsigned short>& getDistanceFirst() const;
	const vector<unsigned short>& getBrightnessFirst() const;
	const vector<unsigned short>& getDistanceSecond() const;
	const vector<unsigned short>& getBrightnessSecond() const;
	
	const vector<ofVec2f>& getPoints() const;
	
protected:
	bool newFrame;
	
	void analyze();
	
	ScanData scanBack, scanFront;
	vector<ofVec2f> points;
};

class ofxSickGrabber : public ofxSick {
public:
	
protected:
	LMS1xx laser;
	
	void connect();
	void disconnect();
	void threadedFunction();
};
