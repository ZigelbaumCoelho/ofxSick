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
	ofSetColor(255);
	
	float scale = ofMap(mouseX, 0, ofGetWidth(), 0.05, 2, true);
	
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_POINTS);
	const vector<ofVec2f>& points = sick.getPoints();
	for(int i = 0; i < points.size(); i++) {
		mesh.addVertex(points[i]);
	}
	ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
	ofPushMatrix();
	ofScale(scale, scale);
	mesh.draw();
	ofPopMatrix();
	
	int count = 10;
	ofNoFill();
	ofSetColor(64);
	float maxRange = MAX(ofGetWidth(), ofGetHeight()) / 2;
	for(int i = 0; i < count; i++) {
		float radius = ofMap(i, 0, count, 0, maxRange);
		float distance = radius / scale;
		ofCircle(0, 0, radius);
		ofVec2f textPosition = ofVec2f(radius, 0).rotate(45);
		ofDrawBitmapStringHighlight(ofToString(distance, 2) + "mm", textPosition);
	}
}

void testApp::keyPressed(int key){
	if(key == 's') {
		//sick.save();
	}
}
