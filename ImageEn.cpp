#include "ImageEn.h"
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")
ImageEn::ImageEn(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

QByteArray  ImageEn::ImageEnde(QImage image)
{
	qDebug()<<QDateTime::currentDateTime();

	qDebug() <<"image format :"<< image.format();


	static int i = 0;

	av_register_all();

	avcodec_register_all();


	//fps每秒25帧
	int fps = 25;

	
	Encodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!Encodec)
	{
	
		printf("avcodec_find_encoder AV_CODEC_ID_H264 failed!\n");
		exit(0);
	}

	Enc = avcodec_alloc_context3(Encodec);
	if (!Enc)
	{

		printf("avcodec_alloc_context3  failed!\n");
		exit(0);
	}

	//设置编码器上下文参数
	Enc->width = image.width();
	Enc->height = image.height();
	Enc->bit_rate =120000;
	AVRational r = { 1, 25 };//每秒25帧
	Enc->time_base = r;
	Enc->gop_size = 50;//GOP组
	Enc->max_b_frames = 1;
	Enc->pix_fmt = AV_PIX_FMT_YUV420P;//视频格式
	av_opt_set(Enc->priv_data, "preset", "ultrafast", 0);
	av_opt_set(Enc->priv_data, "tune", "stillimage,fastdecode,zerolatency", 0);
	av_opt_set(Enc->priv_data, "x264opts", "crf=26:vbv-maxrate=728:vbv-bufsize=364:keyint=25", 0);


	//Enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	int ret = avcodec_open2(Enc, Encodec, NULL);
	if (ret < 0)
	{
	
		printf("avcodec_open2  failed!\n");
		exit(0);
	}
	printf("avcodec_open2 success!\n");

	//rgbFrame数据初始化
	rgbFrame = av_frame_alloc();
	rgbFrame->format = AV_PIX_FMT_RGBA;
	rgbFrame->width = image.width();
	rgbFrame->height = image.height();

	int numBytes1 = avpicture_get_size(AV_PIX_FMT_RGBA, image.width(), image.height());

	buffer1 = (uint8_t*)av_malloc(numBytes1 * sizeof(uint8_t));

	//rgbFrame挂载到image
	avpicture_fill((AVPicture*)rgbFrame, buffer1, AV_PIX_FMT_RGBA, image.width(), image.height());

	rgbFrame->data[0] = image.bits();

	qWarning() << "numBytes1" << numBytes1;

	AVFrame *Frameyuv = av_frame_alloc();

	int numBytes2 = avpicture_get_size(AV_PIX_FMT_YUV420P, image.width(), image.height());

	buffer2 = (uint8_t*)av_malloc(numBytes2 * sizeof(uint8_t));


	avpicture_fill((AVPicture*)Frameyuv, buffer2, AV_PIX_FMT_YUV420P, image.width(), image.height());

	qWarning() << "numBytes2" << numBytes2;


	//rgb to yuv 空间
	SWSctx = sws_getCachedContext(SWSctx,
		image.width(), image.height(), AV_PIX_FMT_BGRA,
		image.width(), image.height(), AV_PIX_FMT_YUV420P, SWS_BICUBIC,
		NULL, NULL, NULL);

	//输出空间
	Frameyuv->format = AV_PIX_FMT_YUV420P;
	Frameyuv->width = image.width();
	Frameyuv->height = image.height();


		clock_t start_time = clock();
		int h = sws_scale(SWSctx, rgbFrame->data, rgbFrame->linesize, 0, Enc->height, Frameyuv->data, Frameyuv->linesize);
		if (h <= 0)
		{
			qDebug() << "sws_scale No " << h;
		}
		
		//存储转换后的YUV420数据
		SaveYUV420(Frameyuv);


		int p = 0;
		int Top = 1;
		while (Top != 0)
	    {
		//6 encode frame
		Frameyuv->pts = p;

		p = p + 3600;

		Top = avcodec_send_frame(Enc, Frameyuv);
		if (Top != 0)
		{
			continue;
		}
		AVPacket Packetpkt;
		av_init_packet(&Packetpkt);


		Top = avcodec_receive_packet(Enc, &Packetpkt);
		if (Top != 0)
		{
			av_strerror(Top, errorbuf, sizeof(errorbuf));
			qDebug() << "avcodec_receive_packet  NO ->" << errorbuf<< Top;
			qDebug() << " Number :" << i << Top;
			i++;
			continue;
		}
		qDebug() << QDateTime::currentDateTime();

		SaveH264(&Packetpkt);

		//*********  Encodec Packetpkt  start   ***************
		qDebug() << " Packetpkt size :" << Packetpkt.size;
		qDebug() << " Packetpkt KB size :" << Packetpkt.size/1024;
		qDebug() << " Packetpkt data :" << Packetpkt.data;
		qDebug() << " Packetpkt pts  :" << Packetpkt.pts;
		qDebug() << " Packetpkt dts  :" << Packetpkt.dts;


		//*********  Encodec Packetpkt  End   ***************



		clock_t end_time = clock();
		qDebug() << "Codec Time: " << static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC * 1000 << "ms";
		qDebug() << " ********************* ";

		QByteArray p;
		p.append((uint8_t)Packetpkt.data,Packetpkt.size);
	    qDebug()<<"  QByteArray SIZE: "	<<p.size();
		return p;
		p.clear();
		av_packet_unref(&Packetpkt);
		qDebug() << " ImageEnde  End ";
	}
}

std::string ImageEn::GetError()
{
	std::string re = this->errorbuf;
	return re;
}
void ImageEn::CloseEn()
{
	av_free(buffer1);
	av_free(buffer2);


	av_free(rgbFrame);
	av_free(Frameyuv);

	avcodec_close(Enc);
	avcodec_free_context(&Enc);

	if (SWSctx)
	{
		sws_freeContext(SWSctx);
	}

}
//void ImageEn::ShowImage(QByteArray p)
//{
//
//	qDebug() << " ShowImage QByte data :" << p.data();
//	qDebug() << " ShowImage QByte size :" << p.size();
//
//
//
//	AVPacket* si;
//	si=av_packet_alloc();//创建并初始化
//
//
//	si->data = (uint8_t*)p.data();
//	si->size = p.size();
//
//	QByteArraytoImage(si);
//
//}
//void ImageEn::QByteArraytoImage(AVPacket *pkt)
//{
//
//	if (!Enc)
//	{
//		printf("avcodec_alloc_context3  failed!\n");
//		return;
//	}
//
//	if (Twoyuv == NULL)
//	{
//		Twoyuv = av_frame_alloc();
//	}
//	clock_t start_time = clock();
//
//	int re = avcodec_send_packet(Enc, pkt);
//	if (re != 0)
//	{
//			av_strerror(re, errorbuf, sizeof(errorbuf));
//			qDebug() << "avcodec_send_packet  NO ->" << errorbuf << re;
//	}
//	re = avcodec_receive_frame(Enc, Twoyuv);
//	{
//			av_strerror(re, errorbuf, sizeof(errorbuf));
//			qDebug() << "avcodec_receive_frame  NO ->" << errorbuf << re;
//			return;
//	}
//
//
//	clock_t end_time = clock();
//	qDebug() << "Decode Time: " << static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC * 1000 << "ms";
//
//
//	qDebug() << " yuv size:" << Twoyuv->width;
//
//}

bool ImageEn::ToRGB(AVFrame *yuv, char *out)
{


	AVCodecContext *videoCtx = avcodec_alloc_context3(Encodec);

	cCtx = sws_getCachedContext(cCtx, videoCtx->width,
		videoCtx->height,
		videoCtx->pix_fmt,
		videoCtx->width, videoCtx->height,
		AV_PIX_FMT_BGRA,
		SWS_BICUBIC,
		NULL, NULL, NULL
	);
	if (!cCtx)
	{
		printf("sws_getCachedContext failed!\n");
		return false;
		//printf("sws_getCachedContext success!\n");
	}
	uint8_t *data[AV_NUM_DATA_POINTERS] = { 0 };
	data[0] = (uint8_t *)out;
	int linesize[AV_NUM_DATA_POINTERS] = { 0 };
	linesize[0] = videoCtx->width * 4;
	int h = sws_scale(cCtx, yuv->data, yuv->linesize, 0, videoCtx->height,
		data,
		linesize
	);
	if (h > 0)
	{
		printf("(%d)", h);
	}
	return true;
}
void ImageEn::SaveYUV420(AVFrame* Frameyuv)
{
	//********************** 保存YUV数据

	FILE *yuv_file = fopen("yuv_file.yuv", "wb");

	if (yuv_file)
	{
		qDebug() << "OK-----------";
	}

	char* buf = new char[Frameyuv->height * Frameyuv->width * 3 / 2];
	memset(buf, 0, Frameyuv->height * Frameyuv->width * 3 / 2);
	int height = Frameyuv->height;
	int width = Frameyuv->width;
	printf("decode video ok\n");
	int a = 0, i;
	for (i = 0; i < height; i++)
	{
		memcpy(buf + a, Frameyuv->data[0] + i * Frameyuv->linesize[0], width);
		a += width;
	}
	for (i = 0; i < height / 2; i++)
	{
		memcpy(buf + a, Frameyuv->data[1] + i * Frameyuv->linesize[1], width / 2);
		a += width / 2;
	}
	for (i = 0; i < height / 2; i++)
	{
		memcpy(buf + a, Frameyuv->data[2] + i * Frameyuv->linesize[2], width / 2);
		a += width / 2;
	}
	fwrite(buf, 1, Frameyuv->height * Frameyuv->width * 3 / 2, yuv_file);
	delete buf;
	buf = NULL;

	fclose(yuv_file);
}

void ImageEn::SaveH264(AVPacket * packet)
{
	FILE *fpSave;
	if ((fpSave = fopen("geth264.h264", "wb")) == NULL) //h264保存的文件名  
	{
		qDebug() << "fopen  SaveH264 NO";
		return;
	}
	fwrite(packet->data, 1, packet->size, fpSave);//写数据到文件中  

	fclose(fpSave);
}


