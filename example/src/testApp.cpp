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

ofMesh getMesh(const vector<ofVec2f>& points) {
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_POINTS);
	for(int i = 0; i < points.size(); i++) {
		mesh.addVertex(points[i]);
	}
	return mesh;
}

void testApp::draw() {
	ofBackground(0);
	
	float scale = ofMap(mouseX, 0, ofGetWidth(), 0.05, 2, true);	
	ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
	
	int count = 10;
	ofNoFill();
	float maxRange = MAX(ofGetWidth(), ofGetHeight()) / 2;
	ofSetColor(ofColor::blue);
	ofLine(0, 0, maxRange, 0); // forward
	ofSetColor(ofColor::magenta);
	ofLine(ofVec2f(0, 0), ofVec2f(maxRange, 0).rotate(-135)); // left bound
	ofLine(ofVec2f(0, 0), ofVec2f(maxRange, 0).rotate(+135)); // right bound
	ofSetColor(64);
	for(int i = 0; i < count; i++) {
		float radius = ofMap(i, 0, count - 1, 0, maxRange);
		float distance = radius / scale;
		ofCircle(0, 0, radius);
		ofVec2f textPosition = ofVec2f(radius, 0).rotate(45);
		ofDrawBitmapStringHighlight(ofToString(distance, 2) + "mm", textPosition);
	}
	
	ofPushMatrix();
	ofScale(scale, scale);
	ofSetColor(255);
	getMesh(sick.getPointsFirst()).draw();
	ofSetColor(ofColor::yellow);
	getMesh(sick.getPointsSecond()).draw();
	ofPopMatrix();
}

void testApp::keyPressed(int key){
	if(key == 's') {
		//sick.save();
	}
}
