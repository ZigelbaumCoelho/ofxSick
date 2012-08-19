#include "ofxSickTracker.h"

using namespace cv;
using namespace ofxCv;

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

float ofxSickFollower::getLiving() const {
	if(startedDying) {
		return 1 - (ofGetElapsedTimef() - startedDying) / dyingTime;
	} else {
		return 1;
	}
}

void ofxSickFollower::draw(float clusterSize) {
	ofPushStyle();
	if(startedDying) {
		ofSetColor(ofColor::red);
		clusterSize = getLiving() * clusterSize;
	} else {
		ofSetColor(ofColor::green);
	}
	ofCircle(toOf(position), clusterSize);
	ofLine(toOf(recent), toOf(position));
	ofSetColor(255);
	ofDrawBitmapString(ofToString(label), toOf(recent));
	all.draw();
	ofPopStyle();
}
