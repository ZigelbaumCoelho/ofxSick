/*
 ofxSickTracker helps cluster and tracker data from an ofxSick source, but it
 relies on ofxCv/OpenCv to do this so it's an optional class. If you don't
 need to cluster or track points, you can just remove the files.
 */

#pragma once

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
public:
	ofxSickTracker()
	:maxClusterCount(12)
	,minClusterSize(1)
	,maxPointDistance(50)
	,useKmeans(true)
	,maxStddev(60) { // 60 is good for hand/arm tracking
	}
	void setupKmeans(float maxStddev, unsigned int maxClusterCount) {
		this->maxClusterCount = maxClusterCount;
		this->maxStddev = maxStddev;
		useKmeans = true;
	}
	void setupNaive(int minClusterSize, int maxPointDistance) {
		this->maxPointDistance = maxPointDistance;
		this->minClusterSize = minClusterSize;
		useKmeans = false;
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
	void update(const vector<ofVec2f>& points) {
		if(useKmeans) {
			updateKmeans(points);
		} else {
			updateNaive(points);
		}
		ofxCv::PointTrackerFollower<F>::track(clusters);		
	}
	void update(ofxSick& sick) {
		update(sick.getPointsFirst());
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
protected:
	vector<cv::Point2f> clusters;
	ofRectangle region;
	unsigned int maxClusterCount;
	float maxStddev;
	bool useKmeans;
	int minClusterSize, maxPointDistance;
	
	void updateNaive(const vector<ofVec2f>& points) {
		if(points.size() > 0) {
			vector< vector<ofVec2f> > all;
			for(int i = 0; i < points.size(); i++) {
				const ofVec2f& cur = points[i];
				if(region.inside(cur)) {
					if(all.empty()) {
						all.push_back(vector<ofVec2f>());
					} else {
						ofVec2f& prev = all.back().back();
						float distance = cur.distance(prev);
						if(distance > maxPointDistance) {
							all.push_back(vector<ofVec2f>());
						}
					}
					all.back().push_back(cur);
				}
			}
			clusters.clear();
			for(int i = 0; i < all.size(); i++) {
				if(all[i].size() >= minClusterSize) {
					vector<cv::Point2f> allCv = ofxCv::toCv(all[i]);
					
					cv::Mat curMat(allCv);
					cv::Scalar curMean, curStddev;
					cv::meanStdDev(curMat, curMean, curStddev);
					clusters.push_back(cv::Point2f(curMean[0], curMean[1]));
					
					/*
					cv::Point2f enclosingCenter;
					float radius;
					cv::minEnclosingCircle(allCv, enclosingCenter, radius);
					clusters.push_back(enclosingCenter);
					*/
				}
			}
		}
	}
	
	void updateKmeans(const vector<ofVec2f>& points) {
		// build samples vector for all points within the bounds
		vector<cv::Point2f> samples;
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
				vector<int> clusterCount(centers.size());
				for(int i = 0; i < samples.size(); i++) {
					centered[i] = centers[labels[i]];
					clusterCount[labels[i]]++;
				}
				cv::Mat centeredMat(centered);
				centeredMat -= cv::Mat(samples);
				cv::Scalar curMean, curStddev;
				cv::meanStdDev(centeredMat, curMean, curStddev);
				float totalDev = ofVec2f(curStddev[0], curStddev[1]).length();
				if(totalDev < maxStddev) {
					for(int i = centers.size() - 1; i >= 0; i--) {
						if(clusterCount[i] < minClusterSize) {
							centers.erase(centers.begin() + i);
						}
					}
					clusters = centers;
					break;
				}
			}
		}
	}
};