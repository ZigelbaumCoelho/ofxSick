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
	
	ofxSickGrabber grabber;
	ofxSickPlayer player;
	ofxSick* sick;
	
	vector<cv::Point2f> clusters;
	vector<float> stddev;
	bool showGains;
	ofPath activeRegion;
	bool recording;
	
	ofxCv::PointTracker tracker;
	typedef map<unsigned int, cv::Point2f> LabeledPoints;
	LabeledPoints smooth;
};