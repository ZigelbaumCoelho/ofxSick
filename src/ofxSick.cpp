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
	ofLogVerbose("ofxSick") << "Connecting.";
	laser.connect("169.254.238.162");
	if(!laser.isConnected()) {
		ofLogError("ofxSick") << "Connection failed.";
		return;
	}
	
	ofLogVerbose("ofxSick") << "Logging in.";
	laser.login();
	
	scanCfg targetCfg;
	targetCfg.angleResolution = .5 * 1000;
	targetCfg.scaningFrequency = 50 * 100;
	targetCfg.startAngle = 0; // defaults to -45 * 10000.
	targetCfg.stopAngle = 0; // defaults to 225 * 10000.
	
	ofLogVerbose("ofxSick") << "Geting current scan configuration.";
	scanCfg curCfg = laser.getScanCfg();
	
	if(curCfg.angleResolution != targetCfg.angleResolution ||
		curCfg.scaningFrequency != targetCfg.scaningFrequency) {
		ofLogVerbose("ofxSick") << "Setting new scan configuration.";
		laser.setScanCfg(targetCfg);
		ofLogVerbose("ofxSick") << "Updating current scan configuration.";
		curCfg = laser.getScanCfg();
	} else {
		ofLogVerbose("ofxSick") << "LMS is already configured.";
	}
	
	ofLogVerbose("ofxSick") << curCfg.scaningFrequency/100. << "Hz at " << curCfg.angleResolution/10000. << "deg, from " << curCfg.startAngle/10000. << "deg to " << curCfg.stopAngle/10000. << "deg";
	
	scanDataCfg targetDataCfg;
	targetDataCfg.deviceName = false;
	targetDataCfg.encoder = 0;
	targetDataCfg.outputChannel = 3;
	targetDataCfg.remission = true;
	targetDataCfg.resolution = 0;
	targetDataCfg.position = false;
	targetDataCfg.outputInterval = 1;
	
	ofLogVerbose("ofxSick") << "Setting scan data configuration.";
	laser.setScanDataCfg(targetDataCfg);
	
	ofLogVerbose("ofxSick") << "Start measurments.";
	laser.startMeas();
	
	ofLogVerbose("ofxSick") << "Wait for ready status.";
	int ret = 0, prevRet = 0;
	while (ret != 7) {
		ret = laser.queryStatus();
		if(ret != prevRet) {
			ofLogVerbose("ofxSick") << "Status: " << ret;
		}
		prevRet = ret;
		ofSleepMillis(500);
	}
	ofLogVerbose("ofxSick") << "Ready, starting continuous data transmission.";
	laser.scanContinous(1);
}

void ofxSickGrabber::disconnect() {
	ofLogVerbose("ofxSick") << "Stopping continuous data transmission.";
	laser.scanContinous(0);
	laser.stopMeas();
	ofLogVerbose("ofxSick") << "Disconnecting.";
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
	disconnect();
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