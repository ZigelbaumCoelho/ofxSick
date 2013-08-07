#include "ofxSick.h"

template <class T> void writeRaw(ofFile& out, T data) {
	out.write((char*) &data, sizeof(data)); 
}

template <class T> void writeRaw(ofFile& out, vector<T>& data) {
	writeRaw(out, data.size());
	for(int i = 0; i < data.size(); i++) {
		writeRaw(out, data[i]);
	}
}

template <class T> void readRaw(ofFile& in, T& data) {
	in.read((char*) &data, sizeof(data)); 
}

template <class T> void readRaw(ofFile& in, vector<T>& data) {
	unsigned int n;
	readRaw(in, n);
	data.resize(n);
	for(int i = 0; i < data.size(); i++) {
		readRaw(in, data[i]);
	}
}

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

ofxSick::ofxSick()
	:angleOffset(0)
	,scanningFrequency(50)
	,startAngle(-45)
	,stopAngle(225)
	,angularResolution(.5)
	,newFrame(false)
	,invert(false) {
}

void ofxSick::setup() {
	startThread();
}

void ofxSick::setInvert(bool invert) {
	this->invert = invert;
}

void ofxSick::setScanningFrequency(float scanningFrequency) {
	this->scanningFrequency = scanningFrequency;
}

void ofxSick::setAngularResolution(float angularResolution) {
	this->angularResolution = angularResolution;
}

void ofxSick::setAngleRange(float startAngle, float stopAngle) {
	this->startAngle = startAngle;
	this->stopAngle = stopAngle;
}

void ofxSick::setAngleOffset(float angleOffset) {
	this->angleOffset = angleOffset;
}

float ofxSick::getAngleOffset() const {
	return angleOffset;
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

void ofxSick::draw(int gridDivisions, float gridSize) const {
	ofPushMatrix();
	ofPushStyle();
	
	ofPushMatrix();
	ofRotate(invert ? -angleOffset : angleOffset);
	ofNoFill();
	ofSetColor(ofColor::blue);
	ofLine(0, 0, gridSize, 0); // forward
	ofSetColor(ofColor::magenta);
	ofLine(ofVec2f(0, 0), ofVec2f(gridSize, 0).rotate(startAngle)); // left bound
	ofLine(ofVec2f(0, 0), ofVec2f(gridSize, 0).rotate(stopAngle)); // right bound
	for(int i = 0; i < gridDivisions; i++) {
		float radius = ofMap(i, -1, gridDivisions - 1, 0, gridSize);
		ofSetColor(64);
		ofCircle(0, 0, radius);
		ofVec2f textPosition = ofVec2f(radius, 0).rotate(45);
		ofSetColor(255);
		ofDrawBitmapString(ofToString(radius, 2) + "mm", textPosition);
	}
	ofPopMatrix();
	
	ofSetColor(255);
	pointCloud(pointsFirst).draw();
	ofSetColor(ofColor::red);
	pointCloud(pointsSecond).draw();
	ofPopStyle();
	ofPopMatrix();
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
		float theta = i * angularResolution;
		theta += angleOffset;
		if(invert) {
			theta = (startAngle + stopAngle) / 2 - theta;
		} else {
			theta = stopAngle + theta;
		}
		theta += 90;
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

ofxSickGrabber::ofxSickGrabber()
:recording(false)
,ip("192.168.0.1") {
}

void ofxSickGrabber::setIp(string ip) {
	this->ip = ip;
}

void ofxSickGrabber::confirmCfg(int curCfg, int targetCfg, const string& name) {
	if(curCfg != targetCfg) {
		ofLogError("ofxSickGrabber") << "Failed to set " << name << " @ " << ip; 
	}
}

void ofxSickGrabber::connect() {
	ofLogVerbose("ofxSickGrabber") << "Connecting @ " << ip;
	laser.connect(ip);
	if(!laser.isConnected()) {
		ofLogError("ofxSickGrabber") << "Connection failed @ " << ip;
		return;
	}
	
	ofLogVerbose("ofxSickGrabber") << "Logging in.";
	laser.login();
	
	laser.stopMeas(); // unnecessary?
	
	scanCfg targetCfg;
	targetCfg.angleResolution = angularResolution * 1000;
	targetCfg.scaningFrequency = scanningFrequency * 100;
	targetCfg.startAngle = startAngle * 10000; // 0 defaults to -45 * 10000.
	targetCfg.stopAngle = stopAngle * 10000; // 0 defaults to 225 * 10000.
	
	ofLogVerbose("ofxSickGrabber") << "Setting new scan configuration.";
	laser.setScanCfg(targetCfg);
	
	ofLogVerbose("ofxSickGrabber") << "Updating current scan configuration.";
	scanCfg curCfg = laser.getScanCfg();
	
	scanningFrequency = curCfg.scaningFrequency / 100.;
	angularResolution = curCfg.angleResolution / 10000.;
	startAngle = curCfg.startAngle / 10000.;
	stopAngle = curCfg.stopAngle / 10000.;	
	ofLogVerbose("ofxSickGrabber") << scanningFrequency << "Hz at " << angularResolution << "deg, from " << startAngle << "deg to " << stopAngle << "deg";
	
	confirmCfg(curCfg.angleResolution, targetCfg.angleResolution, "angular resolution");
	confirmCfg(curCfg.scaningFrequency, targetCfg.scaningFrequency, "scanning frequency");
	confirmCfg(curCfg.startAngle, targetCfg.startAngle, "start angle");
	confirmCfg(curCfg.stopAngle, targetCfg.stopAngle, "stop angle");
	
	bool enableSecondReturn = false;
	scanDataCfg targetDataCfg;
	targetDataCfg.deviceName = false;
	targetDataCfg.encoder = 0;
	targetDataCfg.outputChannel = enableSecondReturn ? 3 : 1;
	targetDataCfg.remission = false;
	targetDataCfg.resolution = 0; // 8-bit remission values
	targetDataCfg.position = false;
	targetDataCfg.outputInterval = 1; // don't skip any frames
	
	ofLogVerbose("ofxSickGrabber") << "Setting scan data configuration.";
	laser.setScanDataCfg(targetDataCfg);
	
	ofLogVerbose("ofxSickGrabber") << "Start measurments.";
	laser.startMeas();
	
	ofLogVerbose("ofxSickGrabber") << "Wait for ready status.";
	int ret = 0, prevRet = 0;
	while (ret != 7) {
		ret = laser.queryStatus();
		if(ret != prevRet) {
			ofLogVerbose("ofxSickGrabber") << "Status: " << getStatusString(ret);
		}
		prevRet = ret;
		ofSleepMillis(10);
	}
	ofLogVerbose("ofxSickGrabber") << "Ready, starting continuous data transmission.";
	laser.scanContinous(1);
}

void ofxSickGrabber::disconnect() {
	ofLogVerbose("ofxSickGrabber") << "Stopping continuous data transmission.";
	laser.scanContinous(0);
	laser.stopMeas();
	ofLogVerbose("ofxSickGrabber") << "Disconnecting.";
	laser.disconnect();
}

void ofxSickGrabber::threadedFunction() {
	connect();
	while(isThreadRunning()) {
		//unsigned long start = ofGetSystemTime();
		scanData data;
		laser.getData(data);
		lock();
		scanBack.first.distance.assign(data.dist1, data.dist1 + data.dist_len1);
		scanBack.first.brightness.assign(data.rssi1, data.rssi1 + data.rssi_len1);
		scanBack.second.distance.assign(data.dist2, data.dist2 + data.dist_len2);
		scanBack.second.brightness.assign(data.rssi2, data.rssi2 + data.rssi_len2);
		newFrame = true;
		unlock();
		//unsigned long stop = ofGetSystemTime();
		//cout << "total time: " << (stop - start) << endl;
	}
	disconnect();
}

void ofxSickGrabber::startRecording() {
	ofLogVerbose("ofxSickGrabber") << "Started recording data.";
	recording = true;
	recordedData.clear();
}

void ofxSickGrabber::stopRecording(string filename) {
	if(recording) {
		ofLogVerbose("ofxSickGrabber") << "Stopped recording data, saving " << recordedData.size() << " frames to " << filename;
		recording = false;
		ofFile out(filename, ofFile::WriteOnly, true);
		for(int i = 0; i < recordedData.size(); i++) {
			ScanData& cur = recordedData[i];
			writeRaw(out, cur.first.distance);
			writeRaw(out, cur.first.brightness);
			writeRaw(out, cur.second.distance);
			writeRaw(out, cur.second.brightness);
		}
		ofLogVerbose("ofxSickGrabber") << "Done saving data.";
	} else {
		ofLogVerbose("ofxSickGrabber") << "Not recording data, not saving.";
	}
}

void ofxSickGrabber::analyze() {
	ofxSick::analyze();
	if(recording) {
		recordedData.push_back(scanFront);
	}
}

ofxSickPlayer::ofxSickPlayer() 
	:position(0) {
}

void ofxSickPlayer::load(string filename) {
	ofLogVerbose("ofxSickPlayer") << "Loading recorded data from " << filename;
	ofFile in(filename, ofFile::ReadOnly);
	if(!in.exists()) {
		ofLogVerbose("ofxSickPlayer") << "Cannot load recorded data: " << filename << " does not exist.";
	}
	while(!in.eof()) {
		ScanData cur;
		readRaw(in, cur.first.distance);
		readRaw(in, cur.first.brightness);
		readRaw(in, cur.second.distance);
		readRaw(in, cur.second.brightness);
		recordedData.push_back(cur);
	}
	ofLogVerbose("ofxSickPlayer") << "Done loading " << recordedData.size() << " frames of recorded data.";
	ofxSick::setup();
}

void ofxSickPlayer::threadedFunction() {
	while(isThreadRunning()) {
		lock();
		if(recordedData.size() > 0) {
			scanBack = recordedData[position];
			position = (position + 1) % recordedData.size();
			newFrame = true;
		}
		unlock();
		ofSleepMillis(20); // ~50Hz
	}
}