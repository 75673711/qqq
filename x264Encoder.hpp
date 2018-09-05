#pragma once
#include <QObject>

#include <queue>
#include <stdint.h>
#include "x264/x264.h"

class x264Encoder : public QObject {
	Q_OBJECT

public:
	x264Encoder(QObject * parent = Q_NULLPTR);
	~x264Encoder();

public:
	void initilize();
	void unInitilize();
	void encodeFrame(char* buffer, int size);
	bool isNalsAvailableInOutputQueue();
	x264_nal_t getNalUnit();
private:
	// Use this context to convert your BGR Image to YUV image since x264 do not support RGB input
	std::queue<x264_nal_t> outputQueue;
	x264_param_t parameters;
	x264_picture_t picture_in, picture_out;
	x264_t* encoder;
	
};
