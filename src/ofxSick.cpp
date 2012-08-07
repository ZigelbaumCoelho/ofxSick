#include "ofxSick.h"

ofxSick::ofxSick() {
}

ofxSick::~ofxSick() {
	cout << "Stop continuous data transmission ..." << endl;
	laser.scanContinous(0);
	laser.stopMeas();
	cout << "Disconnect from laser" << endl;
	laser.disconnect();
}

void ofxSick::setup() {	
	laser.connect("169.254.238.162");
	if(!laser.isConnected()) {
		cout << "connection failend" << endl;
		return;
	}
	
	cout << "Loging in ..." << endl;
	laser.login();
	laser.stopMeas();
	
	cout << "Geting scan configuration ..." << ::endl;
	scanCfg c = laser.getScanCfg();
	
	cout << "Scanning Frequency : " << c.scaningFrequency/100.0 << "Hz AngleResolution : " << c.angleResolution/10000.0 << "deg " << endl;
	
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
	cout << "Start measurements ..." << endl;
	laser.startMeas();
	
	cout << "Wait for ready status ..." << endl;
	ret = 0;
	while (ret != 7) {
		ret = laser.queryStatus();
		cout << "status : " << ret << endl;
		sleep(1);
	}
	cout << "Laser ready" << endl;
	
	cout << "Start continuous data transmission ..." << endl;
	laser.scanContinous(1);
	
	for(int i =0; i < 3; i++) {
		cout << "Receive data sample ..." << endl;
		laser.getData(data);
	}
}

void ofxSick::update() {
	laser.getData(data);
}