#pragma once

#include <queue>

#include "x264Encoder.hpp"
#include "live555/FramedSource.hh"

class LiveSourceWithx264 :public FramedSource
{
public:
	static LiveSourceWithx264* createNew(UsageEnvironment& env);
	static EventTriggerId eventTriggerId;
public:
	LiveSourceWithx264(UsageEnvironment& env);
	virtual ~LiveSourceWithx264(void);
private:
	virtual void doGetNextFrame();
	static void deliverFrame0(void* clientData);
	void deliverFrame();
	void encodeNewFrame();
	static unsigned referenceCount;
	std::queue<x264_nal_t> nalQueue;
	timeval currentTime;
	// videoCaptureDevice is my BGR data source. You can have according to your need
	//cv::VideoCapture videoCaptureDevice;
	//cv::Mat rawImage;
	// Remember the x264 encoder wrapper we wrote in the start
	x264Encoder *encoder;
};