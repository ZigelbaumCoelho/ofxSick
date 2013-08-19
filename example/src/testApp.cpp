#include "testApp.h"

void testApp::setup() {
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofSetCircleResolution(64);
	ofSetLogLevel(OF_LOG_VERBOSE);
	sick.setIp("192.168.0.1");
	sick.setScanningFrequency(25);
	sick.setAngularResolution(.25);
	sick.setAngleRange(0, 180);
	sick.setAngleOffset(45);
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
