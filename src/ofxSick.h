#pragma once

#include "ofMain.h"
#include "LMS1xx.h"

class ofxSick : public ofThread {
public:
	ofxSick();
	~ofxSick();

	void setup();
	bool isFrameNew();
	
	const vector<unsigned short>& getDistanceFirst() const;
	const vector<unsigned short>& getBrightnessFirst() const;
	const vector<unsigned short>& getDistanceSecond() const;
	const vector<unsigned short>& getBrightnessSecond() const;
	
protected:
	bool newFrame;
	
	vector<unsigned short> distanceFirst, brightnessFirst;
	vector<unsigned short> distanceSecond, brightnessSecond;
};

class ofxSickGrabber : public ofxSick {
public:
	~ofxSickGrabber();
	
protected:
	LMS1xx laser;
	
	void connect();
	void threadedFunction();
};
