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
public:
	ofxSickFollower()
	:startedDying(0) {
	}
	void setup(const cv::Point2f& track);
	void update(const cv::Point2f& track);
	void kill();
	void draw();
};

template <class F>
class ofxSickTracker : public ofxCv::PointTrackerFollower<F> {
protected:
	vector<cv::Point2f> clusters;
	static const float minAngle = -26, maxAngle = +26;
	static const unsigned short minDistance = 1200, maxDistance = 2400;
	static const int maxClusterCount = 12;
	static const float maxStddev = 60;
public:
	void update(ofxSick& sick) {
		// build samples vector for all points within the bounds
		vector<cv::Point2f> samples;
		const vector<unsigned short>& distance = sick.getDistanceFirst();
		const vector<ofVec2f>& points = sick.getPointsFirst();
		for(int i = 0; i < points.size(); i++) {
			float theta = ofMap(i, 0, points.size(), -135, +135);
			if(distance[i] < maxDistance && distance[i] > minDistance &&
				 theta < maxAngle && theta > minAngle) {
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