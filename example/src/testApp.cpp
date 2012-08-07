#include "testApp.h"

//These are the active sensing area in mm from the LMS
#define X_MIN_BOUNDS -1200
#define X_MAX_BOUNDS 1200
#define Y_MIN_BOUNDS 600
#define Y_MAX_BOUNDS 2500

void testApp::setup() {
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofSetCircleResolution(6);
	
	sick.setup();
}

void testApp::update() {
	if(sick.isFrameNew()) {
		
	}
}

void testApp::draw() {
	ofBackground(255);
}

void testApp::keyPressed(int key){
	if(key == 's') {
		//sick.save();
	}
}
