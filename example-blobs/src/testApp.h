#pragma once

#include "ofMain.h"
#include "ofxSick.h"
#include "ofxCv.h"

namespace ofxCv {

	template <class T>
	class Follower {
	protected:
		bool dead;
	public:
		Follower()
		:dead(false) {
		}
		virtual void update(const T& track) = 0;
		virtual void kill() {
			dead = true;
		}
		bool getDead() const {
			return dead;
		}
	};
	
	/*
	 a list of Follower objects is maintained
	 when a new tracked object is found, a new Follower is created
	 when an old tracked object is found, Follower::update() is called
	 when an object can't be found, Follower::kill() is called
	 when Follower::getDead() is true, the object is finally removed
	 the Followers should be accessible via a vector
	 */
	
	template <class T, class F>
	class TrackerFollower : public Tracker<T> {
	protected:
		vector<unsigned int> labels;
		vector<bool> killed;
		vector<F> followers;
	public:
		vector<unsigned int>& track(const vector<T>& objects) {
			Tracker<T>::track(objects);
			// kill missing, update old
			for(int i = 0; i < labels.size(); i++) {
				unsigned int curLabel = labels[i];
				F& curFollower = followers[i];
				if(!Tracker<T>::existsCurrent(curLabel)) {
					if(!killed[i]) {
						curFollower.kill();
						killed[i] = true;
					}
				} else {
					curFollower.update(Tracker<T>::getCurrent(curLabel));
				}
			}
			// add new
			for(int i = 0; i < Tracker<T>::newLabels.size(); i++) {
				labels.push_back(Tracker<T>::newLabels[i]);
				followers.push_back(F());
			}			
			// remove dead
			for(int i = labels.size() - 1; i >= 0; i--) {
				if(followers[i].getDead()) {
					followers.erase(followers.begin() + i);
					labels.erase(labels.begin() + i);
				}
			}
		}
		vector<F>& getFollowers() {
			return followers;
		}
	};
	
	template <class F> class RectTrackerFollower : public TrackerFollower<cv::Rect, F> {};
	template <class F> class PointTrackerFollower : public TrackerFollower<cv::Point2f, F> {};
}

class MyFollower : public ofxCv::Follower<cv::Point2f> {
protected:
	cv::Point2f position;
public:
	void update(const cv::Point2f& track) {
		position = ofxCv::toCv(ofxCv::toOf(position).interpolate(ofxCv::toOf(track), .1));
	}
	void draw() {
		ofPushStyle();
		ofSetColor(ofColor::red);
		ofCircle(ofxCv::toOf(position), 4);
		ofPopStyle();
	}
};

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	void keyPressed(int key);
	
	ofxSickGrabber grabber;
	ofxSickPlayer player;
	ofxSick* sick;
	
	vector<cv::Point2f> clusters;
	vector<float> stddev;
	bool showGains;
	ofPath activeRegion;
	bool recording;
	
	ofxCv::PointTrackerFollower<MyFollower> trackerFollower;
	
	ofxCv::Tracker<cv::Point2f> tracker;
	typedef map<unsigned int, cv::Point2f> LabeledPoints;
	LabeledPoints smooth;
};