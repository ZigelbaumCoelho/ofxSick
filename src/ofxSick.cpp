#include "ofxSick.h"

ofxSick::ofxSick() {
}

ofxSick::~ofxSick() {
	stopThread();
}

void ofxSick::setup() {
	startThread();
}

bool ofxSick::isFrameNew() {
	bool curNewFrame = newFrame;
	newFrame = false;
	return curNewFrame;
}

const vector<unsigned short>& ofxSick::getDistanceFirst() const {
	return scanFront.first.distance;
}

const vector<unsigned short>& ofxSick::getBrightnessFirst() const {
	return scanFront.first.brightness;
}

const vector<unsigned short>& ofxSick::getDistanceSecond() const {
	return scanFront.second.distance;
}

const vector<unsigned short>& ofxSick::getBrightnessSecond() const {
	return scanFront.second.brightness;
}

const vector<ofVec2f>& ofxSick::getPoints() const {
	return points;
}

void ofxSick::update() {
	bool needToAnalyze = false;
	lock();
	if(newFrame) {
		scanFront = scanBack;
		needToAnalyze = true;
	}
	unlock();
	if(needToAnalyze) {
		analyze();
	}
}

void ofxSick::analyze() {
	vector<unsigned short>& distances = scanFront.first.distance;
	points.clear();
	for(int i = 0; i < distances.size(); i++) {
		float theta = i * .5; // .5 is the angular resolution
		points.push_back(ofVec2f(distances[i], 0).rotate(theta));
	}
}

void ofxSickGrabber::connect() {
	ofLogVerbose() << "Connecting to LMS.";
	laser.connect("169.254.238.162");
	if(!laser.isConnected()) {
		ofLogError() << "Connection to LMS failed.";
		return;
	}
	
	ofLogVerbose() << "Logging in to LMS.";
	laser.login();
	laser.stopMeas();
	
	ofLogVerbose() << "Geting LMS scan configuration.";
	scanCfg c = laser.getScanCfg();
	ofLogVerbose() << "Scanning Frequency : " << c.scaningFrequency/100.0 << "Hz AngleResolution : " << c.angleResolution/10000.0 << "deg";
	
	c.angleResolution = 5000;
	c.scaningFrequency = 5000;
	
	laser.setScanCfg(c);
	
	scanDataCfg cc;
	cc.deviceName = false;
	cc.encoder = 0;
	cc.outputChannel = 3;
	cc.remission = true;
	cc.resolution = 0;
	cc.position = false;
	cc.outputInterval = 1;
	
	laser.setScanDataCfg(cc);
	
	int ret = 0;
	ofLogVerbose() << "Start LMS measurments.";
	laser.startMeas();
	
	ofLogVerbose() << "Wait for LMS ready status.";
	ret = 0;
	while (ret != 7) {
		ret = laser.queryStatus();
		ofLogVerbose() << "LMS Status: " << ret;
		ofSleepMillis(1000);
	}
	ofLogVerbose() << "LMS is ready, starting continuous data transmission.";
	laser.scanContinous(1);
}

ofxSickGrabber::~ofxSickGrabber() {
	ofLogVerbose() << "Stopping LMS continuous data transmission.";
	laser.scanContinous(0);
	laser.stopMeas();
	ofLogVerbose() << "Disconnecting from LMS.";
	laser.disconnect();
}

void ofxSickGrabber::threadedFunction() {
	connect();
	while(isThreadRunning()) {
		scanData data;
		laser.getData(data);
		lock();
		scanBack.first.distance.assign(data.dist1, data.dist1 + data.dist_len1);
		scanBack.first.brightness.assign(data.rssi1, data.rssi1 + data.rssi_len1);
		scanBack.second.distance.assign(data.dist2, data.dist2 + data.dist_len2);
		scanBack.second.brightness.assign(data.rssi2, data.rssi2 + data.rssi_len2);
		newFrame = true;
		unlock();
	}
}

/*
void ofxSick::save() {
	ofFile file("out.txt", ofFile::WriteOnly);
	for(int i = 0; i < allData.size(); i++) {
		vector<string> all;
		for(int j = 0; j < allData[i].dist_len1; j++) {
			all.push_back(ofToString(allData[i].dist1[j]));
		}
		file << ofJoinString(all, "\t") << endl;
	}
}
*/