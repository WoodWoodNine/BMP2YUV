#ifndef BMP_H
#define BMP_H


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#if defined(__linux__)
#include <stdint.h>
typedef uint8_t BYTE ;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint32_t LONG;
#else
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
#endif

#pragma pack(1)
typedef struct  {  
	WORD bfType;			/* ˵���ļ������� */  
	DWORD bfSize;			/* ˵���ļ��Ĵ�С�����ֽ�Ϊ��λ */  
	WORD bfReserved1;		/* ����������Ϊ0 */  
	WORD bfReserved2;		/* ����������Ϊ0 */  
	DWORD bfOffsetBytes;		/* ˵����BITMAPFILEHEADER�ṹ��ʼ��ʵ�ʵ�ͼ������֮����ֽ�ƫ���� */  
	
}BMPFileHead; //14�ֽ�,����sizeof���㳤��ʱΪ16�ֽ�


typedef struct  {  
	DWORD biSize;			/* ˵���ṹ�������ֽ��� */  
	LONG biWidth;			/* ������Ϊ��λ˵��ͼ��Ŀ�� */  
	LONG biHeight;			/* ������Ϊ��λ˵��ͼ��ĸ߶� */  
	WORD biPlanes;			/* ˵��λ����������Ϊ1 */  
	WORD biBitCount;		/* ˵��λ��/���أ�1��2��4��8��24 */  
	DWORD biCompression;	/* ˵��ͼ���Ƿ�ѹ����ѹ������BI_RGB��BI_RLE8��BI_RLE4��BI_BITFIELDS */  
	DWORD biSizeImage;		/* ���ֽ�Ϊ��λ˵��ͼ���С��������4��������*/  
	LONG biXPixelsPerMeter;	/*Ŀ���豸��ˮƽ�ֱ��ʣ�����/�� */  
	LONG biYPixelsPerMeter;	/*Ŀ���豸�Ĵ�ֱ�ֱ��ʣ�����/�� */  
	DWORD biClrUsed;		/* ˵��ͼ��ʵ���õ�����ɫ�������Ϊ0������ɫ��Ϊ2��biBitCount�η� */  
	DWORD biClrImportant;	/*˵����ͼ����ʾ����ҪӰ�����ɫ��������Ŀ�������0����ʾ����Ҫ��*/  
}BMPHeaderInfo;  //40�ֽ�
#pragma pack()

typedef struct  {  
	BYTE rgbBlue;		/*ָ����ɫ����*/  
	BYTE rgbGreen;		/*ָ����ɫ����*/  
	BYTE rgbRed;		/*ָ����ɫ����*/  
	BYTE rgbReserved;	/*������ָ��Ϊ0*/  
}RGB;

typedef struct yuvtype {  
	BYTE yuvY;		/*ָ��Y���ȷ���*/  
	BYTE yuvU;		/*ָ��U��Cb������*/  
	BYTE yuvV;		/*ָ��V��Cr������*/  
}YUV;
void get_bmpdata(char * bmpfilename,char * yuvfilename, char yuvmode);
void to_yuv(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi);
void calculateYUV(YUV *yuvcolor,RGB rgbcolor );


void writeyuv1(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi);
void writeyuv4(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi);
void writeyuv8(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi);
void writeyuv24(FILE *fv, BYTE databuf[], int yuvmode, BMPHeaderInfo bhi);
void writeyuv32(FILE *fv, BYTE databuf[], int yuvmode, BMPHeaderInfo bhi);

void printBMPFileHead(BMPFileHead* bfh);
void printBMPHeaderInfo(BMPHeaderInfo* bhi);
#endif