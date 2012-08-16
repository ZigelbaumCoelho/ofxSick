#pragma once

#include "ofMain.h"
#include "ofxSick.h"
#include "ofxCv.h"

class Cluster : public ofxCv::PointFollower {
protected:
	cv::Point2f position, recent;
	float startedDying;
	ofPolyline all;
public:
	Cluster()
		:startedDying(0) {
	}
	void setup(const cv::Point2f& track);
	void update(const cv::Point2f& track);
	void kill();
	void draw();
};

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
	
	ofxCv::PointTrackerFollower<Cluster> tracker;
};