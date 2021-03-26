#pragma once
/*
From:   YiXiaoNaiHe
*/
#include <QtWidgets/QWidget>
#include "ui_ImageEn.h"
#include<QDebug>
#include<QByteArray>
#include<QPainter>
#include<QTimer>
#include<QDateTime>

extern "C"
{
#include"libavutil\imgutils.h"
#include"libavutil\samplefmt.h"
#include"libavformat\avformat.h"
#include"libavutil\opt.h"
#include<libavformat/avformat.h>
#include<libswscale/swscale.h>
#include <libswresample/swresample.h>
}


class ImageEn : public QWidget
{
	Q_OBJECT

public:
	ImageEn(QWidget *parent = Q_NULLPTR);
public:


	QByteArray   ImageEnde(QImage image);
	std::string GetError();
	void CloseEn();
	//void ShowImage(QByteArray p);
	//void QByteArraytoImage(AVPacket *pkt);
	bool ToRGB(AVFrame *yuv, char *out);
	void SaveYUV420(AVFrame* Frameyuv);
	void SaveH264(AVPacket * packet);

public:

	char errorbuf[1024];
	AVFormatContext *ic = NULL;
	AVFrame *yuv = NULL;
	AVFrame *Twoyuv = NULL;
	SwsContext *cCtx = NULL;  //ת����Ŀռ�
	AVFrame *rgbFrame = NULL; //QImageͼ�����
	AVFrame *Frameyuv = NULL;
	AVCodec *Encodec = NULL;  //�������1
	AVCodec *ABC = NULL;  //�������2
	AVCodecContext *Enc = NULL; //�����
	AVCodecContext *toImageEnc = NULL; //�����
	SwsContext *SWSctx = NULL;
	uint8_t* buffer1 = NULL;
	uint8_t* buffer2 = NULL;


private:
	Ui::ImageEnClass ui;
};
