#include "testApp.h"

using namespace cv;
using namespace ofxCv;

// wrists are approximately 60-80mm in diameter
// a closed hand extends about 100mm
// a stretched hand can extend up to 200mm
const float clusterRadius = 80; // 80mm radius is bigger than a closed hand, smaller than stretched
const float maxStddev = 60;
const int maxClusterCount = 12;

void testApp::setup() {
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofSetCircleResolution(64);
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	showGains = false;
	
	sick.setup();
}

void testApp::update() {
	sick.update();
	if(sick.isFrameNew()) {
		// build samples vector for all points within the bounds
		unsigned short minDistance = 100, maxDistance = 2000;
		float minAngle = -90, maxAngle = +90;
		vector<cv::Point2f> samples;
		const vector<unsigned short>& distance = sick.getDistanceFirst();
		const vector<ofVec2f>& points = sick.getPointsFirst();
		for(int i = 0; i < points.size(); i++) {
			float theta = ofMap(i, 0, points.size(), -135, +135);
			if(distance[i] < maxDistance && distance[i] > minDistance &&
				theta < maxAngle && theta > minAngle) {
				samples.push_back(toCv(points[i]));
			}
		}
		Mat samplesMat = Mat(samples).reshape(1);
		
		bool found = false;
		clusters.clear();
		stddev.clear();
		for(int clusterCount = 1; clusterCount < maxClusterCount; clusterCount++) {			
			if(samples.size() > clusterCount) {
				Mat labelsMat, centersMat;
				kmeans(samplesMat, clusterCount, labelsMat, TermCriteria(), 8, KMEANS_PP_CENTERS, centersMat);
				vector<cv::Point2f> centers = centersMat.reshape(2);
				vector<int> labels = labelsMat;				
				vector<cv::Point2f> centered(samples.size());
				for(int i = 0; i < samples.size(); i++) {
					centered[i] = centers[labels[i]];
				}
				Mat centeredMat(centered);
				centeredMat -= Mat(samples);
				Scalar curMean, curStddev;
				meanStdDev(centeredMat, curMean, curStddev);
				float totalDev = ofVec2f(curStddev[0], curStddev[1]).length();
				if(!found && totalDev < maxStddev) {
					clusters = centers;
					found = true;
				}
				stddev.push_back(totalDev);
			}
		}
	}
}

void testApp::draw() {
	ofBackground(0);
	
	ofPushMatrix();
	ofTranslate(20, 20);
	ofSetColor(255);
	ofCircle(0, 0, 7);
	ofDrawBitmapString(ofToString(clusters.size()) + " clusters", -4, 4);
	ofPopMatrix();
	
	if(showGains) {
		ofPushMatrix();
		ofMesh mesh;
		mesh.setMode(OF_PRIMITIVE_LINE_STRIP);
		float prev = 400;
		for(int i = 0; i < stddev.size(); i++) {
			float curGain = prev / stddev[i];
			prev = stddev[i];
			float x = ofMap(i, 0, stddev.size(), 0, ofGetWidth());
			float y = ofMap(curGain, 0, 4, 0, ofGetHeight());
			mesh.addVertex(ofVec2f(x, y));
			ofDrawBitmapString(ofToString(curGain) + "/" + ofToString(stddev[i]), x, y);
		}
		mesh.draw();
		ofPopMatrix();
	}
	
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
	ofSetColor(255);
	sick.draw();
	for(int i = 0; i < clusters.size(); i++) {
		ofVec2f center = toOf(clusters[i]);
		ofCircle(center, clusterRadius);
	}
	ofPopMatrix();
}

void testApp::keyPressed(int key){
	if(key == 's') {
		//sick.save();
	}
	if(key == 'g') {
		showGains = !showGains;
	}
}
