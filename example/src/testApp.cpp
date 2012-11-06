#include "testApp.h"

void testApp::setup() {
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofSetCircleResolution(64);
	ofSetLogLevel(OF_LOG_VERBOSE);
	sick.setup();
}

void testApp::update() {
	sick.update();
	if(sick.isFrameNew()) {
	}
}

void testApp::draw() {
	ofBackground(0);
	
	float scale = ofMap(mouseX, 0, ofGetWidth(), 0.05, 2, true);	
	ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
	
	ofPushMatrix();
	ofScale(scale, scale);
	ofSetColor(ofColor::white);
	sick.draw();
	ofPopMatrix();
}

void testApp::keyPressed(int key){
	if(key == 's') {
		//sick.save();
	}
}
