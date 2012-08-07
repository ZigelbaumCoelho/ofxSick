#include "testApp.h"

//These are the active sensing area in mm from the LMS
#define X_MIN_BOUNDS -1200
#define X_MAX_BOUNDS 1200
#define Y_MIN_BOUNDS 600
#define Y_MAX_BOUNDS 2500

void testApp::setup() {
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
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
	ofSetColor(255);
	
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_POINTS);
	const vector<ofVec2f>& points = sick.getPoints();
	for(int i = 0; i < points.size(); i++) {
		mesh.addVertex(points[i]);
	}
	ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
	mesh.draw();
}

void testApp::keyPressed(int key){
	if(key == 's') {
		//sick.save();
	}
}
