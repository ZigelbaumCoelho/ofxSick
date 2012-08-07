#include "ofxSick.h"

string getStatusString(int status) {
	switch(status) {
		case 0: return "undefined";
		case 1: return "initialisation";
		case 2: return "configuration";
		case 3: return "idle";
		case 4: return "rotated";
		case 5: return "in preparation";
		case 6: return "ready";
		case 7: return "ready for measurement";
		default: return "unknown";
	}
}

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

ofMesh pointCloud(const vector<ofVec2f>& points) {
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_POINTS);
	for(int i = 0; i < points.size(); i++) {
		mesh.addVertex(points[i]);
	}
	return mesh;
}

void ofxSick::draw() const {
	ofPushStyle();
	pointCloud(pointsFirst).draw();
	ofSetColor(ofColor::red);
	pointCloud(pointsSecond).draw();
	ofPopStyle();
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

const vector<ofVec2f>& ofxSick::getPointsFirst() const {
	return pointsFirst;
}

const vector<ofColor>& ofxSick::getColorsFirst() const {
	return colorsFirst;
}

const vector<ofVec2f>& ofxSick::getPointsSecond() const {
	return pointsSecond;
}

const vector<ofColor>& ofxSick::getColorsSecond() const {
	return colorsSecond;
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

void ofxSick::polarToCartesian(vector<unsigned short>& polar, vector<ofVec2f>& cartesian) const {
	cartesian.resize(polar.size());
	for(int i = 0; i < cartesian.size(); i++) {
		float theta = i * .5; // .5 is the angular resolution
		theta += 225;
		cartesian[i] = ofVec2f(polar[i], 0).rotate(theta);
	}
}

void ofxSick::brightnessToColor(vector<unsigned short>& brightness, vector<ofColor>& color) const {
	color.resize(brightness.size());
	for(int i = 0; i < brightness.size(); i++) {
		color[i].set(brightness[i]); // normally 8-bit, but can be 16-bit
	}
}

void ofxSick::analyze() {
	polarToCartesian(scanFront.first.distance, pointsFirst);
	brightnessToColor(scanFront.first.brightness, colorsFirst);
	polarToCartesian(scanFront.second.distance, pointsSecond);
	brightnessToColor(scanFront.second.brightness, colorsSecond);
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
	targetDataCfg.resolution = 0; // 8-bit remission values
	targetDataCfg.position = false;
	targetDataCfg.outputInterval = 1; // don't skip any frames
	
	ofLogVerbose("ofxSick") << "Setting scan data configuration.";
	laser.setScanDataCfg(targetDataCfg);
	
	ofLogVerbose("ofxSick") << "Start measurments.";
	laser.startMeas();
	
	ofLogVerbose("ofxSick") << "Wait for ready status.";
	int ret = 0, prevRet = 0;
	while (ret != 7) {
		ret = laser.queryStatus();
		if(ret != prevRet) {
			ofLogVerbose("ofxSick") << "Status: " << getStatusString(ret);
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