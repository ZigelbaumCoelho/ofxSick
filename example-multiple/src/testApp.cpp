#include "testApp.h"

void testApp::setup() {
	ofSetFrameRate(120);
	ofSetCircleResolution(64);
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	ofXml xml;
	xml.load("settings.xml");
	osc.setup(xml.getValue<string>("osc/hostname"), xml.getValue<int>("osc/port"));
	rangeNearAngle = xml.getValue<float>("range/near/angle");
	rangeNear.setFromCenter(0, 0, xml.getValue<float>("range/near/width"), xml.getValue<float>("range/near/height"));
	rangeFarRadius = xml.getValue<float>("range/far/radius");
	for(int i = 0; i < xml.getNumChildren("sick"); i++) {
		xml.setTo("sick[" + ofToString(i) + "]");
		ofPtr<ofxSickGrabber> cur(new ofxSickGrabber());
		cur->setIp(xml.getValue("ip"));
		cur->setAngleOffset(xml.getValue<int>("angleOffset"));
		cur->setScanningFrequency(xml.getValue<float>("scanningFrequency"));
		cur->setAngularResolution(xml.getValue<float>("angularResolution"));
		cur->setAngleRange(xml.getValue<int>("startAngle"), xml.getValue<int>("stopAngle"));
		cur->setInvert(xml.getValue<bool>("invert"));
		ofVec2f positionOffset(xml.getValue<float>("positionOffset/x"), xml.getValue<float>("positionOffset/y"));
		cur->setup();
		sick.push_back(cur);
		positionOffsets.push_back(positionOffset);
		xml.setToParent();
	}
	
	trackingRegion.setFromCenter(0, 0, 2 * rangeFarRadius, 2 * rangeFarRadius);
	tracker.setRegion(trackingRegion);	
	tracker.setMaximumDistance(xml.getValue<float>("tracker/maximumDistance"));
	tracker.setPersistence(xml.getValue<int>("tracker/persistence"));
	int maxStddev = xml.getValue<int>("tracker/maxStddev");
	int maxClusterCount = xml.getValue<int>("tracker/maxClusterCount");
	cout << "setting up kmeans with " << maxStddev << " / " << maxClusterCount << endl;
	tracker.setupKmeans(maxStddev, maxClusterCount);
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
		points.clear();
		pointCloud.clear();
		pointCloud.setMode(OF_PRIMITIVE_POINTS);
		for(int i = 0; i < sick.size(); i++) {
			const vector<ofVec2f>& pointsFirst = sick[i]->getPointsFirst();
			for(int j = 0; j < pointsFirst.size(); j++) {
				ofVec2f cur = pointsFirst[j] + positionOffsets[i];
				float distance = cur.length();
				if(distance < rangeFarRadius && !rangeNear.inside(cur.getRotated(-rangeNearAngle))) {
					points.push_back(cur);
					pointCloud.addVertex(cur);
				}
			}
		}
		tracker.update(points);
		ofxOscMessage msg;
		msg.setAddress("/sick");
		msg.addIntArg(tracker.size());
		for(int i = 0; i < tracker.size(); i++) {
			cv::Point2f cluster = tracker.getCluster(i);
			msg.addFloatArg(cluster.x);
			msg.addFloatArg(cluster.y);
		}
		osc.sendMessage(msg);
	}
}

void testApp::draw() {
	ofBackground(0);
	
	ofPushMatrix();
	ofTranslate(20, 20);
	ofSetColor(255);
	ofCircle(0, 0, 7);
	ofDrawBitmapString(ofToString(tracker.size()) + " clusters", -4, 4);
	ofPopMatrix();
	
	ofPushMatrix();
	float scale = ofMap(mouseX, 0, ofGetWidth(), 0.05, 2, true);	
	ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
	ofScale(scale, scale);
	ofSetColor(ofColor::yellow);
	ofNoFill();
	ofPushMatrix();
	ofRotate(rangeNearAngle);
	ofRect(rangeNear);
	ofDrawBitmapString("near", rangeNear.getBottomRight());
	ofPopMatrix();
	ofCircle(0, 0, rangeFarRadius);
	ofDrawBitmapString("far", 0, rangeFarRadius);
	for(int i = 0; i < sick.size(); i++) {
		ofPushMatrix();
		ofTranslate(positionOffsets[i]);
		sick[i]->draw();
		ofDrawBitmapString(sick[i]->getIp() + (sick[i]->getConnected() ? "" : " (connecting)"), 10, 20);
		ofPopMatrix();
	}
	ofSetColor(ofColor::red);
	pointCloud.draw();
	
	tracker.draw();
	
	ofPopMatrix();
}
