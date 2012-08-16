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
		
		// should try implementing this with a class:
		// TrackerAuto<TrackedType, PairedType>
		// where after every TrackerAuto::track(vector<TrackedType>& all)
		// it calls PairedType::track(TrackedType& cur)
		// or a class that does update(Tracker) so it can run off a contour finder?
		
		// or a class that has update(TrackedType, PairedType)
		// default behavior of the PairedType is to be removed immediately after
		// but this can be overridden to create a delay
		
		// remove old points
		LabeledPoints::iterator itr = smooth.begin();
		while(itr != smooth.end()) {
			if(!tracker.existsCurrent(itr->first)) {
				smooth.erase(itr++);
			} else {
				itr++;
			}
		}
		// update current remaining points
		for(itr = smooth.begin(); itr != smooth.end(); itr++) {
			cv::Point2f& prev = itr->second;
			const cv::Point2f& cur = tracker.getCurrent(itr->first);
			prev = toCv(toOf(prev).interpolate(toOf(cur), .1));
		}
		// add new points
		const vector<unsigned int>& newLabels = tracker.getNewLabels();
		for(int i = 0; i < newLabels.size(); i++) {
			unsigned int curLabel = newLabels[i];
			smooth[curLabel] = tracker.getCurrent(curLabel);
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
	for(int i = 0; i < clusters.size(); i++) {
		ofVec2f center = toOf(clusters[i]);
		ofCircle(center, clusterRadius);
		
		unsigned int label = tracker.getLabelFromIndex(i);
		ofDrawBitmapString(ofToString(label), center);
		ofVec2f smoothCenter = toOf(smooth[label]);
		ofLine(center, smoothCenter);
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
