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
	void draw() const;
	
	const vector<unsigned short>& getDistanceFirst() const;
	const vector<unsigned short>& getBrightnessFirst() const;
	const vector<unsigned short>& getDistanceSecond() const;
	const vector<unsigned short>& getBrightnessSecond() const;
	
	const vector<ofVec2f>& getPointsFirst() const;
	const vector<ofColor>& getColorsFirst() const;
	const vector<ofVec2f>& getPointsSecond() const;
	const vector<ofColor>& getColorsSecond() const;
	
protected:
	bool newFrame;
	
	void analyze();
	void polarToCartesian(vector<unsigned short>& polar, vector<ofVec2f>& cartesian) const;
	void brightnessToColor(vector<unsigned short>& brightness, vector<ofColor>& color) const;
	
	ScanData scanBack, scanFront;
	vector<ofVec2f> pointsFirst, pointsSecond;
	vector<ofColor> colorsFirst, colorsSecond;
};

class ofxSickGrabber : public ofxSick {
public:
	
protected:
	LMS1xx laser;
	
	void connect();
	void disconnect();
	void threadedFunction();
};
