I use ffmpeg to encode picture to video stream. After I send this stream to RTP protocol. How I can set NAL units size. Here's my codec settings:

pAVCodecCtxEncode =  avcodec_alloc_context();
	AVCodec* codecE = avcodec_find_encoder(CODEC_ID_H264);
	 if (!codecE)
	 { 
	 return ; // Didn't find a video stream
	 }
	 if(codecE->capabilities & CODEC_CAP_TRUNCATED)
	 pAVCodecCtxEncode->flags|= CODEC_FLAG_TRUNCATED; 	
	 avcodec_get_context_defaults2(pAVCodecCtxEncode, CODEC_TYPE_VIDEO);
	 
	 // set codec id
	 pAVCodecCtxEncode->codec_id = CODEC_ID_H264;
	 pAVCodecCtxEncode->coder_type = 1;
	 pAVCodecCtxEncode->thread_count = 3;
	 
	 // put bitrate parameters
	 pAVCodecCtxEncode->bit_rate = 90000;
	 pAVCodecCtxEncode->bit_rate_tolerance = 90000;
	 
	 // put width and height
	 pAVCodecCtxEncode->width = 192;
	 pAVCodecCtxEncode->height = 144;
	 
	 // set frame rate
	 pAVCodecCtxEncode->time_base.den = 5;
	 pAVCodecCtxEncode->time_base.num = 1;
	 
	 // set pixel format
	 pAVCodecCtxEncode->pix_fmt = PIX_FMT_YUV420P;

	 pAVCodecCtxEncode->gop_size = 15;
	 pAVCodecCtxEncode->keyint_min = 15;
	 

	 pAVCodecCtxEncode->b_frame_strategy = 1;
	 pAVCodecCtxEncode->max_b_frames = 300;
	 pAVCodecCtxEncode->max_b_frames = 300;
	 

	 pAVCodecCtxEncode->qblur = 0.5;
	 pAVCodecCtxEncode->qcompress = 0.5;
	 pAVCodecCtxEncode->b_quant_offset = 1.25;
	 pAVCodecCtxEncode->b_quant_factor = 1.25;
	 pAVCodecCtxEncode->i_quant_offset = 0.0;
	 pAVCodecCtxEncode->i_quant_factor = -0.71f;

	 pAVCodecCtxEncode->mb_qmax = pAVCodecCtxEncode->qmax = 0.2;
	 pAVCodecCtxEncode->mb_qmin = pAVCodecCtxEncode->qmin = 0;
	 pAVCodecCtxEncode->max_qdiff = 0;
	 
	 // motion estimation range
	 pAVCodecCtxEncode->me_range = 0;
	 
	 pAVCodecCtxEncode->error_concealment   = 3;
	 pAVCodecCtxEncode->error_recognition   = 1;
	// pAVCodecCtxEncode->rtp_mode= 1;
	 pAVCodecCtxEncode->rtp_payload_size= 650;

	 	
	 if (avcodec_open(pAVCodecCtxEncode, codecE) < 0){
	 
		 return ;
	 }

Changing this "pAVCodecCtxEncode->rtp_payload_size " setting does nothing.
This I encode frame:

        int outbuf_size = avpicture_get_size(pAVCodecCtxEncode->pix_fmt, w, h);
	uint8_t *outbuf = (uint8_t*)av_malloc(outbuf_size);
	AVFrame *packet = avcodec_alloc_frame();
	
	packet->data[0] = (uint8_t*)Y.bytes;
	packet->data[1] = (uint8_t*)V.bytes;
	packet->data[2] = (uint8_t*)U.bytes;
	packet->data[3] = NULL;
	packet->linesize[0] = w;
	packet->linesize[1] = w/2;
	packet->linesize[2] = w/2;
	packet->linesize[3] = 0;
	pAVCodecCtxEncode->rtp_payload_size   = 500;
	int out_size = avcodec_encode_video(pAVCodecCtxEncode, outbuf, outbuf_size, packet);

When I get a buffer, I'm looking for 0x0000001 and is divided into NAL units. The size of them receive more. How can I manage it?
