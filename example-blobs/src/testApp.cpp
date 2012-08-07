#include "testApp.h"

using namespace cv;
using namespace ofxCv;

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
		// build samples vector for all points within the bounds
		unsigned short minDistance = 100, maxDistance = 1000;
		vector<cv::Point2f> samples;
		const vector<unsigned short>& distance = sick.getDistanceFirst();
		const vector<ofVec2f>& points = sick.getPointsFirst();
		for(int i = 0; i < points.size(); i++) {
			if(distance[i] < maxDistance && distance[i] > minDistance) {
				samples.push_back(toCv(points[i]));
			}
		}
		Mat samplesMat = Mat(samples).reshape(1);
		
		int maxClusterCount = 6;
		clusters.resize(maxClusterCount);
		for(int clusterCount = 1; clusterCount < maxClusterCount; clusterCount++) {
			Mat labels, centersMat;
			if(samples.size() > clusterCount) {
				double compactness = cv::kmeans(samplesMat, clusterCount, labels, TermCriteria(), 8, KMEANS_PP_CENTERS, centersMat);
			}
			vector<cv::Point2f>& centers = clusters[clusterCount];
			centers.clear();
			centers = centersMat.reshape(2);
		}
	}
}

void testApp::draw() {
	ofBackground(0);
	
	ofPushMatrix();
	ofTranslate(20, 20);
	for(int i = 1; i < clusters.size(); i++) {
		ofSetColor(ofColor::fromHsb(ofMap(i, 0, clusters.size(), 0, 255), 255, 255));
		ofCircle(0, 0, 7);
		ofDrawBitmapString(ofToString(i) + " clusters", -4, 4);
		ofTranslate(0, 20);
	}
	ofPopMatrix();
		
	ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
	float scale = ofMap(mouseX, 0, ofGetWidth(), 0.05, 2, true);
	
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
	ofSetColor(ofColor::white);
	sick.draw();
	for(int i = 0; i < clusters.size(); i++) {
		ofSetColor(ofColor::fromHsb(ofMap(i, 0, clusters.size(), 0, 255), 255, 255));
		for(int j = 0; j < clusters[i].size(); j++) {
			ofVec2f center = toOf(clusters[i][j]);
			ofCircle(center, 100);
		}
	}
	ofPopMatrix();
}

void testApp::keyPressed(int key){
	if(key == 's') {
		//sick.save();
	}
}
