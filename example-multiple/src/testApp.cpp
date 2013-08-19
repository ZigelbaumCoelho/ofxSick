#include "testApp.h"

void testApp::setup() {
	ofSetFrameRate(120);
	ofSetCircleResolution(64);
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	ofXml xml;
	xml.load("settings.xml");
	for(int i = 0; i < xml.getNumChildren("sick"); i++) {
		xml.setTo("sick[" + ofToString(i) + "]");
		ofPtr<ofxSickGrabber> cur(new ofxSickGrabber());
		cur->setIp(xml.getValue("ip"));
		cur->setAngleOffset(xml.getValue<int>("angleOffset"));
		cur->setScanningFrequency(xml.getValue<float>("scanningFrequency"));
		cur->setAngularResolution(xml.getValue<float>("angularResolution"));
		cur->setAngleRange(xml.getValue<int>("startAngle"), xml.getValue<int>("stopAngle"));
		cur->setup();
		sick.push_back(cur);
		xml.setToParent();
	}
}

void testApp::update() {
	bool frameNew = false;
	for(int i = 0; i < sick.size(); i++) {
		sick[i]->update();
		if(sick[i]->isFrameNew()) {
			frameNew = true;
		}
	}
	if(frameNew) {
		
	}
}

void testApp::draw() {
	ofBackground(0);
	
	ofPushMatrix();
	float scale = ofMap(mouseX, 0, ofGetWidth(), 0.05, 2, true);	
	ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
	
	ofScale(scale, scale);
	ofSetColor(ofColor::white);
	for(int i = 0; i < sick.size(); i++) {
		sick[i]->draw();
	}
	ofPopMatrix();
}
