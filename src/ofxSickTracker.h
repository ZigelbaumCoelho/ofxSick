/*
 ofxSickTracker helps cluster and tracker data from an ofxSick source, but it
 relies on ofxCv/OpenCv to do this so it's an optional class. If you don't
 need to cluster or track points, you can just remove the files.
 */

#include "ofxSick.h"
#include "ofxCv.h"

class ofxSickFollower : public ofxCv::PointFollower {
protected:
	cv::Point2f position, recent;
	float startedDying;
	ofPolyline all;
	float dyingTime;
public:
	ofxSickFollower()
	:startedDying(0)
	,dyingTime(1) {
	}
	virtual void setup(const cv::Point2f& track);
	virtual void update(const cv::Point2f& track);
	virtual void kill();
	virtual float getLiving() const;
	virtual void draw(float clusterSize = 60);
};

template <class F>
class ofxSickTracker : public ofxCv::PointTrackerFollower<F> {
protected:
	vector<cv::Point2f> clusters;
	ofRectangle region;
	unsigned int maxClusterCount;
	float maxStddev;
public:
	ofxSickTracker()
		:maxClusterCount(12)
		,maxStddev(60) { // 60 is good for hand/arm tracking
	}
	void setMaxClusterCount(unsigned int maxClusterCount) {
		this->maxClusterCount = maxClusterCount;
	}
	void setMaxStddev(float maxStddev) {
		this->maxStddev = maxStddev;
	}
	void setRegion(const ofRectangle& region) {
		this->region = region;
	}
	void draw() {
		ofPushStyle();
		ofNoFill();
		vector<F>& followers = ofxCv::PointTrackerFollower<F>::followers;
		for(int i = 0; i < followers.size(); i++) {
			((ofxSickFollower) followers[i]).draw();
		}
		ofSetColor(255);
		ofRect(region);
		ofPopStyle();
	}
	void update(ofxSick& sick) {
		// build samples vector for all points within the bounds
		vector<cv::Point2f> samples;
		const vector<ofVec2f>& points = sick.getPointsFirst();
		for(int i = 0; i < points.size(); i++) {
			if(region.inside(points[i])) {
				samples.push_back(ofxCv::toCv(points[i]));
			}
		}
		
		cv::Mat samplesMat = cv::Mat(samples).reshape(1);
		clusters.clear();
		for(int clusterCount = 1; clusterCount < maxClusterCount; clusterCount++) {
			if(samples.size() > clusterCount) {
				cv::Mat labelsMat, centersMat;
				float compactness = cv::kmeans(samplesMat, clusterCount, labelsMat, cv::TermCriteria(), 8, cv::KMEANS_PP_CENTERS, centersMat);
				vector<cv::Point2f> centers = centersMat.reshape(2);
				vector<int> labels = labelsMat;
				vector<cv::Point2f> centered(samples.size());
				for(int i = 0; i < samples.size(); i++) {
					centered[i] = centers[labels[i]];
				}
				cv::Mat centeredMat(centered);
				centeredMat -= cv::Mat(samples);
				cv::Scalar curMean, curStddev;
				cv::meanStdDev(centeredMat, curMean, curStddev);
				float totalDev = ofVec2f(curStddev[0], curStddev[1]).length();
				if(totalDev < maxStddev) {
					clusters = centers;
					break;
				}
			}
		}
		
		ofxCv::PointTrackerFollower<F>::track(clusters);
	}
	unsigned int size() const {
		return clusters.size();
	}
	cv::Point2f getCluster(unsigned int i) const {
		return clusters.at(i);
	}
	const vector<cv::Point2f>& getClusters() const {
		return clusters;
	}
};