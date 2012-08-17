#include "ofxSickTracker.h"

using namespace cv;
using namespace ofxCv;

const float clusterRadius = 80;
const float dyingTime = 1;

void ofxSickFollower::setup(const cv::Point2f& track) {
	position = track;
	recent = track;
}

void ofxSickFollower::update(const cv::Point2f& track) {
	position = toCv(toOf(position).interpolate(toOf(track), .1));
	recent = track;
	all.addVertex(toOf(position));
}

void ofxSickFollower::kill() {
	float curTime = ofGetElapsedTimef();
	if(startedDying == 0) {
		startedDying = curTime;
	} else if(curTime - startedDying > dyingTime) {
		dead = true;
	}
}

void ofxSickFollower::draw() {
	ofPushStyle();
	float size = clusterRadius;
	if(startedDying) {
		ofSetColor(ofColor::red);
		size = ofMap(ofGetElapsedTimef() - startedDying, 0, dyingTime, size, 0, true);
	} else {
		ofSetColor(ofColor::green);
	}
	ofCircle(toOf(position), size);
	ofLine(toOf(recent), toOf(position));
	ofSetColor(255);
	ofDrawBitmapString(ofToString(label), toOf(recent));
	all.draw();
	ofPopStyle();
}
