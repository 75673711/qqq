#include "x264Encoder.hpp"

#include "ffmpeg/libswscale/swscale.h"

x264Encoder::x264Encoder(QObject * parent) : QObject(parent) {
	
}

x264Encoder::~x264Encoder() {
	
}

void x264Encoder::initilize()
{
	x264_param_default_preset(&parameters, "veryfast", "zerolatency");
	parameters.i_log_level = X264_LOG_INFO;
	parameters.i_threads = 1;
	parameters.i_width = 640;
	parameters.i_height = 480;
	parameters.i_fps_num = 25;
	parameters.i_fps_den = 1;
	parameters.i_keyint_max = 25;
	parameters.b_intra_refresh = 1;
	parameters.rc.i_rc_method = X264_RC_CRF;
	parameters.rc.i_vbv_buffer_size = 1000000;
	parameters.rc.i_vbv_max_bitrate = 90000;
	parameters.rc.f_rf_constant = 25;
	parameters.rc.f_rf_constant_max = 35;
	parameters.i_sps_id = 7;
	// the following two value you should keep 1
	parameters.b_repeat_headers = 1;    // to get header before every I-Frame
	parameters.b_annexb = 1; // put start code in front of nal. we will remove start code later
	x264_param_apply_profile(&parameters, "baseline");

	encoder = x264_encoder_open(&parameters);
	x264_picture_alloc(&picture_in, X264_CSP_I420, parameters.i_width, parameters.i_height);
	picture_in.i_type = X264_TYPE_AUTO;
	picture_in.img.i_csp = X264_CSP_I420;
}

void x264Encoder::unInitilize()
{
	x264_encoder_close(encoder);
}

void x264Encoder::encodeFrame(char* buffer, int size)
{
	int srcStride = parameters.i_width * 4;
	
	picture_in.img.plane;
	picture_in.img.i_stride;

	x264_nal_t* nals;
	int i_nals = 0;
	int frameSize = -1;

	//x264_picture_t pic_in, pic_out;
	//x264_picture_alloc(&pic_in, X264_CSP_I420, w, h)
	//struct SwsContext* convertCtx = sws_getContext(in_w, in_h, PIX_FMT_RGB24, out_w, out_h, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	//sws_scale(convertCtx, &data, &srcstride, 0, h, pic_in.img.plane, pic_in.img.stride);

	frameSize = x264_encoder_encode(encoder, &nals, &i_nals, &picture_in, &picture_out);
	if (frameSize > 0)
	{
		for (int i = 0; i < i_nals; i++)
		{
			outputQueue.push(nals[i]);
		}
	}
}

bool x264Encoder::isNalsAvailableInOutputQueue()
{
	if (outputQueue.empty() == true)
	{
		return false;
	}
	else
	{
		return true;
	}
}

x264_nal_t x264Encoder::getNalUnit()
{
	x264_nal_t nal;
	nal = outputQueue.front();
	outputQueue.pop();
	return nal;
}