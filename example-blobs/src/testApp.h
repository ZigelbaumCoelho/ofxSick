#pragma once

#include "ofMain.h"
#include "ofxSick.h"
#include "ofxCv.h"

/*
 somewhere a list of Follower objects is maintained
 when a new tracked object is found, a new Follower is created
 when an old tracked object is found, Follower::update() is called
 when an object can't be found, Follower::kill() is called
 when Follower::getDead() is true, the object is finally removed
 the Followers should be accessible via a vector
*/

template <class T>
class Follower {
protected:
	bool dead;
public:
	Follower()
		:dead(false) {
	}
	virtual void update(T& track) = 0;
	virtual void kill() {
		dead = true;
	}
	bool getDead() const {
		return dead;
	}
};

class PointFollower : public Follower<cv::Point2f> {
protected:
	cv::Point2f position;
public:
	void update(cv::Point2f& track) {
		position = ofxCv::toCv(ofxCv::toOf(position).interpolate(ofxCv::toOf(track), .1));
	}
	void draw() {
		ofPushStyle();
		ofSetColor(ofColor::red);
		ofCircle(ofxCv::toOf(position), 4);
		ofPopStyle();
	}
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
	
	ofxCv::Tracker<cv::Point2f> tracker;
	typedef map<unsigned int, cv::Point2f> LabeledPoints;
	LabeledPoints smooth;
};