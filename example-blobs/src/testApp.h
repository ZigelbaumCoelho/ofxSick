#pragma once

#include "ofMain.h"
#include "ofxSick.h"
#include "ofxCv.h"

namespace ofxCv {

	template <class T>
	class Follower {
	protected:
		bool dead;
		unsigned int label;
	public:
		Follower()
		:dead(false)
		,label(0) {}
		
		virtual void setup(const T& track) {}
		virtual void update(const T& track) {}
		virtual void kill() {
			dead = true;
		}
		
		void setLabel(unsigned int label) {
			this->label = label;
		}
		unsigned int getLabel() const {
			return label;
		}
		bool getDead() const {
			return dead;
		}
	};
	
	typedef Follower<cv::Rect> RectFollower;
	typedef Follower<cv::Point2f> PointFollower;
	
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
		vector<F> followers;
	public:
		vector<unsigned int>& track(const vector<T>& objects) {
			Tracker<T>::track(objects);
			// kill missing, update old
			for(int i = 0; i < labels.size(); i++) {
				unsigned int curLabel = labels[i];
				F& curFollower = followers[i];
				if(!Tracker<T>::existsCurrent(curLabel)) {
					curFollower.kill();
				} else {
					curFollower.update(Tracker<T>::getCurrent(curLabel));
				}
			}
			// add new
			for(int i = 0; i < Tracker<T>::newLabels.size(); i++) {
				unsigned int curLabel = Tracker<T>::newLabels[i];
				labels.push_back(curLabel);
				followers.push_back(F());
				followers.back().setup(Tracker<T>::getCurrent(curLabel));
				followers.back().setLabel(curLabel);
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

class MyFollower : public ofxCv::PointFollower{
protected:
	cv::Point2f position, recent;
	float startedDying;
public:
	MyFollower()
		:startedDying(0) {
	}
	void setup(const cv::Point2f& track) {
		position = track;
		recent = track;
	}
	void update(const cv::Point2f& track) {
		position = ofxCv::toCv(ofxCv::toOf(position).interpolate(ofxCv::toOf(track), .1));
		recent = track;
	}
	void kill() {
		cout << "killing from " << startedDying << endl;
		float curTime = ofGetElapsedTimef();
		if(startedDying == 0) {
			cout << "setting to " << startedDying << endl;
			startedDying = curTime;
		} else if(curTime - startedDying > 1) {
			cout << curTime << " " << startedDying << endl;
			dead = true;
		}
	}
	void draw() {
		ofPushStyle();
		float size = 80;
		if(startedDying) {
			ofSetColor(ofColor::red);
			size = ofMap(ofGetElapsedTimef() - startedDying, 0, 1, size, 0, true);
		} else {
			ofSetColor(ofColor::blue);
		}
		ofCircle(ofxCv::toOf(recent), size);
		ofLine(ofxCv::toOf(recent), ofxCv::toOf(position));
		ofSetColor(255);
		ofDrawBitmapString(ofToString(label), ofxCv::toOf(recent));
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
	
	ofxCv::PointTrackerFollower<MyFollower> tracker;
};