#include "testApp.h"

class LogOverlay : public ofBaseLoggerChannel {
public:
	list<string> messages;
	stringstream joined;
	ofTrueTypeFont font;
	int maxSize;
	LogOverlay()
	:maxSize(10) {
		font.loadFont(OF_TTF_MONO, 8, false);
	}
	void draw() {
		ofPushStyle();
		ofSetColor(255, 128);
		if(messages.size()) {
			font.drawString(joined.str(), 10, 20);
		}
		ofPopStyle();
	}
	void log(ofLogLevel level, const string & module, const string & message) {
		messages.push_back(module + (module != "" ? ": " : "") + message);
		cout << messages.size() << endl;
		if(messages.size() > maxSize) {
			messages.pop_front();
		}
		joined.str("");
    copy(messages.begin(), messages.end(), ostream_iterator<string>(joined, "\n"));
	}
	void log(ofLogLevel level, const string & module, const char* format, ...) {}
	void log(ofLogLevel level, const string & module, const char* format, va_list args) {}
};
ofPtr<LogOverlay> logOverlay;

void testApp::setup() {
	ofSetFrameRate(120);
	ofSetCircleResolution(64);
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	logOverlay = ofPtr<LogOverlay>(new LogOverlay());
	ofSetLoggerChannel(logOverlay);
	
	ofXml xml;
	xml.load("settings.xml");
	for(int i = 0; i < xml.getNumChildren("sick"); i++) {
		xml.setTo("sick[" + ofToString(i) + "]");
		ofPtr<ofxSickGrabber> cur(new ofxSickGrabber());
		cur->setIp(xml.getValue("ip"));
		cur->setAngleOffset(xml.getValue<int>("angleOffset"));
		cur->setAngleRange(xml.getValue<int>("startAngle"), xml.getValue<int>("stopAngle"));
		cur->setup();
		sick.push_back(cur);
		xml.setToParent();
	}
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
		
	}
}

void testApp::draw() {
	ofBackground(0);
	
	ofPushMatrix();
	float scale = ofMap(mouseX, 0, ofGetWidth(), 0.05, 2, true);	
	ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
	
	ofScale(scale, scale);
	ofSetColor(ofColor::white);
	for(int i = 0; i < sick.size(); i++) {
		sick[i]->draw();
	}
	ofPopMatrix();
	
	logOverlay->draw();
}
