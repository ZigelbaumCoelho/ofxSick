#pragma once

#include "ofMain.h"
#include "LMS1xx.h"
//#include "ofxKmeans.h"
#include "ofxOpenCv.h"

#define NUM_SAMPLES 541
#define NUM_TIME_SAMPLES 1
#define NUM_BLOB_SAMPLES 10


//These are the active sensing area in mm from the LMS
#define X_MIN_BOUNDS -1200
#define X_MAX_BOUNDS 1200
#define Y_MIN_BOUNDS 600
#define Y_MAX_BOUNDS 2500


class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
        
        
        void drawRawData();
        void drawPolar2CartData();
        void polar2CartData();
        void findClusterCentroids();
        void drawTrackingAndRawData();
        void filterBlobs();
        void drawFilteredBlobs();
        void filterSingleBlobs();
        void drawSingleFilteredBlob();
    
        vector<int> clusterPoints(vector<ofPoint> data, unsigned nClusters);
    
        LMS1xx laser;
        scanData dataBuf;
        
        uint16_t distVals[541];
        
        //Ring buffer of values
        uint16_t bufferDistVals[NUM_TIME_SAMPLES][541];
        
        //Current buffer in ring
        int currentSample;
    
        vector<ofPoint> trackedPts;
        ofPoint allPts[541];
        
        //centroids of clusters
        vector<ofPoint> centroids;
        
        ofPoint blob1Prev;
    
        ofPoint blob2Prev;
    
        ofPoint blob1Avg;
    
        ofPoint blob2Avg;
        
        ofPoint blobRingBuffer[NUM_BLOB_SAMPLES];
        int currentBlobRingSample;
        bool blobVisible;
        bool runningBlobOpacityRampUp;
        bool runningBlobOpacityRampDown;
        float blobOpacity;
        ofPoint filterdSingleBlob;
        
    
    
        double avgDistVals[541];
    
        bool scanning;
    
        ofImage softcircle;
};

typedef struct _clusterInfo {
    int numPoints;
    
    double xSum;
    
    double ySum;
    
    double xCentroid;
    
    double yCentroid;
    
}clusterInfo;

