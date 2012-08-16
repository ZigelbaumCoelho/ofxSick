#include "testApp.h"

using namespace cv;
using namespace ofxCv;

// wrists are approximately 60-80mm in diameter
// a closed hand extends about 100mm
// a stretched hand can extend up to 200mm
const float clusterRadius = 80; // 80mm radius is bigger than a closed hand, smaller than stretched
const float maxStddev = 60;
const int maxClusterCount = 12;
const unsigned short minDistance = 1200, maxDistance = 2400;
const float minAngle = -26, maxAngle = +26;
const float dyingTime = 1;

void Cluster::setup(const cv::Point2f& track) {
	position = track;
	recent = track;
}

void Cluster::update(const cv::Point2f& track) {
	position = toCv(toOf(position).interpolate(toOf(track), .1));
	recent = track;
	all.addVertex(toOf(position));
}

void Cluster::kill() {
	cout << "killing from " << startedDying << endl;
	float curTime = ofGetElapsedTimef();
	if(startedDying == 0) {
		cout << "setting to " << startedDying << endl;
		startedDying = curTime;
	} else if(curTime - startedDying > dyingTime) {
		cout << curTime << " " << startedDying << endl;
		dead = true;
	}
}

void Cluster::draw() {
	ofPushStyle();
	float size = clusterRadius;
	if(startedDying) {
		ofSetColor(ofColor::red);
		size = ofMap(ofGetElapsedTimef() - startedDying, 0, dyingTime, size, 0, true);
	} else {
		ofSetColor(ofColor::green);
	}
	ofCircle(toOf(recent), size);
	ofLine(toOf(recent), toOf(position));
	ofSetColor(255);
	ofDrawBitmapString(ofToString(label), toOf(recent));
	all.draw();
	ofPopStyle();
}

void testApp::setup() {
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofSetCircleResolution(64);
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	showGains = false;
	recording = false;
	
	grabber.setup();
	player.load("recording.lms");
	
	sick = &grabber;
	
	activeRegion.setArcResolution(64);
	activeRegion.setFilled(false);
	activeRegion.arc(ofVec2f(0, 0), minDistance, minDistance, minAngle, maxAngle, true);
	activeRegion.arc(ofVec2f(0, 0), maxDistance, maxDistance, maxAngle, minAngle, false);
	activeRegion.close();
	
	tracker.setMaximumDistance(100); // 200 mm
	tracker.setPersistence(10); // 10 frames
}

void testApp::update() {
	sick->update();
	if(sick->isFrameNew()) {
		// build samples vector for all points within the bounds
		vector<cv::Point2f> samples;
		const vector<unsigned short>& distance = sick->getDistanceFirst();
		const vector<ofVec2f>& points = sick->getPointsFirst();
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
				float compactness = kmeans(samplesMat, clusterCount, labelsMat, TermCriteria(), 8, KMEANS_PP_CENTERS, centersMat);
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
				// compactness is a better metric, but has to be analyzed for gains
				stddev.push_back(totalDev);
			}
		}
		
		tracker.track(clusters);
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
			float y = ofMap(curGain, .5, 6, 0, ofGetHeight());
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
	sick->draw();
	vector<Cluster>& followers = tracker.getFollowers();
	for(int i = 0; i < followers.size(); i++) {
		followers[i].draw();
	}
	activeRegion.draw(0, 0);
	ofPopMatrix();
}

void testApp::keyPressed(int key){
	if(key == 'r') {
		recording = !recording;
		if(recording) {
			grabber.startRecording();
		} else {
			grabber.stopRecording("out.lms");
		}
	}
	if(key == '\t') {
		if(sick == &player) {
			sick = &grabber;
		} else {
			sick = &player;
		}
	}
	if(key == 'g') {
		showGains = !showGains;
	}
}
