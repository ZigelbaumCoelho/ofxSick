#include "testApp.h"

using namespace cv;
using namespace ofxCv;

// wrists are approximately 60-80mm in diameter
// a closed hand extends about 100mm
// a stretched hand can extend up to 200mm
const float clusterRadius = 80; // 80mm radius is bigger than a closed hand, smaller than stretched
const float ignoreAmount = .1; // ignore up to 10% of the points
const int maxClusterCount = 12;

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
		
		clusters.clear();
		for(int clusterCount = 1; clusterCount < maxClusterCount; clusterCount++) {			
			if(samples.size() > clusterCount) {
				Mat labelsMat, clustersMat;
				double compactness = cv::kmeans(samplesMat, clusterCount, labelsMat, TermCriteria(), 8, KMEANS_PP_CENTERS, clustersMat);
				clusters = clustersMat.reshape(2);
				vector<int> labels = labelsMat;
				int outside = 0;
				for(int i = 0; i < samples.size(); i++) {
					ofVec2f curCluster = toOf(clusters[labels[i]]);
					ofVec2f curSample = toOf(samples[i]);
					if(curSample.distance(curCluster) > clusterRadius) {
						outside++;
					}
				}				
				if(outside < samples.size() * ignoreAmount) {
					break;
				}
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
}
