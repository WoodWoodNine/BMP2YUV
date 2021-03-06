#include "bmp.h"


/*
输入参数：
bmpfilename：bmp文件名；
yuvfilename：yuv文件名
yuvmode：yuv的格式
功能：将bmp文件转换为yuv格式，模式可选
返回值：无
*/

void get_bmpdata(char * bmpfilename, char * yuvfilename, char yuvmode)
{
	FILE * fp, *fv;
	int i;
	WORD lineSize;				//一行的字节个数
	BMPFileHead bfh;			//定义文件头信息
	BMPHeaderInfo bhi;			//定义bmp头信息
								//bmp有单色，16色，256色,16位、24位、32位无调色板四种模式。
								//色板作为数据的颜色映射（直接映射表项），不改动，访问频率高，所以采用数组来存储
	RGB  color[256] = { 0 };
	YUV  yuvcolor[256] = { 0 };		//RGB的YUV映射
	BYTE * databuf;				//数据缓存

	if ((fp = fopen(bmpfilename, "r+b")) == NULL) {
		printf("open bmpfile error!\n");
		exit(EXIT_FAILURE);
	}
	//读取文件头信息
	if ((fread(&bfh, sizeof(WORD), 7, fp)) < 7) {
		printf("read file error!\n");
		exit(EXIT_FAILURE);
	}
	//printBMPFileHead(&bfh);
	
	//读取位图信息头
	if ((fread(&bhi, sizeof(WORD), 20, fp)) < 20) {
		printf("read file error!\n");
		exit(EXIT_FAILURE);
	}
	//printBMPHeaderInfo(&bhi);

	//printf("bhi.biBitCount=%u\n\n", bhi.biBitCount);
	//如果是单/16/256色则获取调色板
	if (bhi.biBitCount == 0x01 || bhi.biBitCount == 0x04 || bhi.biBitCount == 0x08) {
		for (i = 0; i<pow(2, bhi.biBitCount); i++) {
			fread(&(color[i]), sizeof(BYTE), 4, fp);
			calculateYUV((yuvcolor + i), color[i]);
		}
	}

	if (bhi.biSizeImage == 0) {
		bhi.biSizeImage = bfh.bfSize - 54;
	}
	//printf("bhi.biSizeImage=%u\n", bhi.biSizeImage);
	//printf("bhi.biHeight=%u\n", bhi.biHeight);
	//printf("bhi.biWidth=%u\n", bhi.biWidth);
	if(bhi.biHeight >> 31 == 0)
		lineSize = bhi.biSizeImage / bhi.biHeight;
	else
		lineSize = bhi.biSizeImage / (-bhi.biHeight);

	//分配内存获取bmp数据
	databuf = (BYTE *)malloc(sizeof(BYTE)*bhi.biSizeImage);
	if (databuf == NULL) {
		printf("mallocation error!\n");
		exit(EXIT_FAILURE);
	}
	//一次性读取全部数据
	for (i = bhi.biSizeImage - lineSize; i >= 0; i = i - lineSize) {
		if ((fread(databuf + i, sizeof(BYTE), lineSize, fp)) < lineSize) {
			printf("get data error!\n");
			exit(EXIT_FAILURE);
		}
	}
	fclose(fp);
	//打开要写入的yuv文件
	if ((fv = fopen(yuvfilename, "w+b")) == NULL) {
		printf("open yuv file error!\n");
		exit(EXIT_FAILURE);
	}
	//格式转换
	to_yuv(fv, databuf, yuvcolor, yuvmode, bhi);
	//释放内存关闭文件
	free(databuf);
	fclose(fv);
}




/*
输入参数：
fv：yuv文件指针
databuf：从bmp读取的4字节数据
yuvcolor：rgb的yuv颜色映射表
yuvmode：yuv写的模式
bhi:bmp的格式信息
功能：根据databuf的数据查找yuvcolor中的颜色，按照yuvmode的格式写入fv指向的文件中
返回值：无；
*/
void to_yuv(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi)
{
	switch (bhi.biBitCount) {
	case 1://1个像素1位,2种颜色
		writeyuv1(fv, databuf, yuvcolor, yuvmode, bhi);
		break;
	case 4://1个像素4位， 16种颜色
		writeyuv4(fv, databuf, yuvcolor, yuvmode, bhi);
		break;
	case 8://1个像素8位，256种颜色
		writeyuv8(fv, databuf, yuvcolor, yuvmode, bhi);
		break;
	case 24://24位无调色板
		writeyuv24(fv, databuf, yuvmode, bhi);
		break;
	case 32://32位无调色板
		writeyuv32(fv, databuf, yuvmode, bhi);
		break;
	default: {
		printf("bmp error!\n");
		exit(EXIT_FAILURE);
	}
	}
}


/*
输入参数：
fv：yuv文件指针；
databuf：数据缓冲区；
yuvcolor：rgb的yuv颜色映射；
yuvmode：模式；
bhi：bmp文件信息
功能：将单色的bmp文件转换为指定格式的yuv文件
返回值：无
*/
void writeyuv1(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi)
{
	unsigned long i;
	int flag = 0, count = 0;
	BYTE bit = 0x80;
	DWORD lineSize = bhi.biSizeImage / bhi.biHeight;

	if (yuvmode == '0') {
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth)) {
					count = 0;
					//写完Y之后，i加上该行剩余的空字节
					if ((i + 1) % lineSize != 0)
						i = i + (lineSize - (i + 1) % lineSize);
					break;
				}
				bit >>= 1;
			}
		}
		//写U
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth) / 2) {
					count = 0;
					//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
					if ((i + 1) % lineSize == 0 && (i + 1) >= lineSize)
						i = i + lineSize;
					else
						i = i + (lineSize - (i + 1) % lineSize) + lineSize;
					break;
				}
				bit >>= 2;
			}
		}
		//写V
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth) / 2) {
					count = 0;
					//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
					if ((i + 1) % lineSize == 0 && (i + 1) >= lineSize)
						i = i + lineSize;
					else
						i = i + (lineSize - (i + 1) % lineSize) + lineSize;
					break;
				}
				bit >>= 2;
			}
		}

	}
	else if (yuvmode == '2') {
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth)) {
					count = 0;
					//写完Y之后，i加上该行剩余的空字节
					if ((i + 1) % lineSize != 0)
						i = i + (lineSize - (i + 1) % lineSize);
					break;
				}
				bit >>= 1;
			}
		}
		//写U
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth) / 2) {
					count = 0;
					//写完U之后，i加上该行剩余的空字节
					if ((i + 1) % lineSize != 0)
						i = i + (lineSize - (i + 1) % lineSize);
					break;
				}
				bit >>= 2;
			}
		}
		//写V
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth) / 2) {
					count = 0;
					//写完V之后，i加上该行剩余的空字节
					if ((i + 1) % lineSize != 0)
						i = i + (lineSize - (i + 1) % lineSize);
					break;
				}
				bit >>= 2;
			}
		}
	}
	else if (yuvmode == '4') {
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth)) {
					count = 0;
					//写完Y之后，i加上该行剩余的空字节
					if ((i + 1) % lineSize != 0)
						i = i + (lineSize - (i + 1) % lineSize);
					break;
				}
				bit >>= 1;
			}
		}
		//写U
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth)) {
					count = 0;
					//写完U之后，i加上该行剩余的空字节
					if ((i + 1) % lineSize != 0)
						i = i + (lineSize - (i + 1) % lineSize);
					break;
				}
				bit >>= 1;
			}
		}
		//写V
		for (i = 0; i<(bhi.biSizeImage*sizeof(BYTE)); i++) {
			bit = 0x80;
			while (bit != 0) {
				flag = 0;
				if ((databuf[i] & bit) == bit) {
					flag = 1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if (count == (bhi.biWidth)) {
					count = 0;
					//写完V之后，i加上该行剩余的空字节
					if ((i + 1) % lineSize != 0)
						i = i + (lineSize - (i + 1) % lineSize);
					break;
				}
				bit >>= 1;
			}
		}
	}
	else {
		printf("bmp to yuv error!（2）\n");
		exit(EXIT_FAILURE);
	}
}



/*
输入参数：
fv：yuv文件指针；
databuf：数据缓冲区；
yuvcolor：rgb的yuv颜色映射；
yuv：模式；
bhi：bmp文件信息
功能：将16色的bmp文件转换为指定格式的yuv文件
返回值：无
*/
void writeyuv4(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi)
{
	DWORD lineSize = bhi.biSizeImage / bhi.biHeight;
	unsigned long i, count = 0;
	BYTE bit = 0x0f;

	if (yuvmode == '0') {
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
				continue;
			}
			bit = databuf[i] & 0x0f;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写U
		count = 0;
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvU), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth) / 2) {
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if ((i + 1) % lineSize == 0 && (i + 1) >= lineSize)
					i = i + lineSize;
				else
					i = i + (lineSize - (i + 1) % lineSize) + lineSize;
			}

		}
		//写V
		count = 0;
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvV), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth) / 2) {
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if ((i + 1) % lineSize == 0 && (i + 1) >= lineSize)
					i = i + lineSize;
				else
					i = i + (lineSize - (i + 1) % lineSize) + lineSize;
			}
		}
	}
	else if (yuvmode == '2') {
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
				continue;
			}
			bit = databuf[i] & 0x0f;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写U
		count = 0;
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvU), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth) / 2) {
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写V
		count = 0;
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvV), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth) / 2) {
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
	}
	else if (yuvmode == '4') {
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			bit = databuf[i] & 0x0f;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
				continue;
			}
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写U
		count = 0;
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvU), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			bit = databuf[i] & 0x0f;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完U之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
				continue;
			}
			if ((fwrite(&(yuvcolor[bit].yuvU), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完U之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写V
		count = 0;
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			bit = databuf[i] & 0xf0;
			bit >>= 4;
			if ((fwrite(&(yuvcolor[bit].yuvV), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			bit = databuf[i] & 0x0f;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完V之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
				continue;
			}
			if ((fwrite(&(yuvcolor[bit].yuvV), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完V之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
	}
}


/*
输入参数：
fv：yuv文件指针；
databuf：数据缓冲区；
yuvcolor：rgb的yuv颜色映射；
yuv：模式；
bhi：bmp文件信息
功能：将256色的bmp文件转换为指定格式的yuv文件
返回值：无
*/
void writeyuv8(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi)
{
	unsigned long i, count = 0;
	DWORD lineSize = bhi.biSizeImage / bhi.biHeight;

	if (yuvmode == '0') {
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写U
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i = i + 2) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvU), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth) / 2) {
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if ((i + 1) % lineSize == 0 && (i + 1) >= lineSize)
					i = i + lineSize;
				else
					i = i + (lineSize - (i + 1) % lineSize) + lineSize;
			}
		}
		//写V
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i = i + 2) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvV), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth) / 2) {
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if ((i + 1) % lineSize == 0 && (i + 1) >= lineSize)
					i = i + lineSize;
				else
					i = i + (lineSize - (i + 1) % lineSize) + lineSize;
			}
		}
	}
	else if (yuvmode == '2') {
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写U
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i = i + 2) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvU), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth) / 2) {
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写V
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i = i + 2) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvV), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth) / 2) {
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
	}
	else if (yuvmode == '4') {
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvY), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写U
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvU), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完U之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
		//写V
		for (i = 0; i<bhi.biSizeImage*sizeof(BYTE); i++) {
			if ((fwrite(&(yuvcolor[databuf[i]].yuvV), sizeof(BYTE), 1, fv)) == 0) {
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if (count == (bhi.biWidth)) {
				count = 0;
				//写完V之后，i加上该行剩余的空字节
				if ((i + 1) % lineSize != 0)
					i = i + (lineSize - (i + 1) % lineSize);
			}
		}
	}

}


/*
输入参数：
fv：yuv文件指针；
databuf：数据缓冲区；
yuv：模式；
bhi：bmp文件信息
功能：将rgb共24位的bmp文件转换为指定格式的yuv文件
返回值：无
*/
void writeyuv24(FILE *fv, BYTE databuf[], int yuvmode, BMPHeaderInfo bhi)
{
	unsigned long i, count = 0;
	int line, offset;
	YUV yuv24;
	int UV[4];
	DWORD lineSize = bhi.biSizeImage / bhi.biHeight;

	if (yuvmode == '0' && bhi.biHeight >> 31 == 0) {
		//写Y
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = line*lineSize + offset;
				yuv24.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = 0; line < bhi.biHeight; line+=2) {
			for (offset = 0; offset < lineSize; offset += 6) {
				i = line*lineSize + offset;
				UV[0] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = line*lineSize + offset + 3;
				UV[1] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line+1)*lineSize + offset;
				UV[2] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line+1)*lineSize + offset + 3;
				UV[3] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				yuv24.yuvU = (UV[0] + UV[1] + UV[2] + UV[3]) >> 2;
				if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = 0; line < bhi.biHeight; line+=2) {
			for (offset = 0; offset < lineSize; offset += 6) {
				i = line*lineSize + offset;
				UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = line*lineSize + offset + 3;
				UV[1] = UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line+1)*lineSize + offset;
				UV[2] = UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line+1)*lineSize + offset + 3;
				UV[3] = UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				yuv24.yuvV = (UV[0] + UV[1] + UV[2] + UV[3]) >> 2;
				if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '0' && bhi.biHeight >> 31 != 0)
	{
		lineSize = bhi.biSizeImage / (-bhi.biHeight);
		//写Y
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = (line - 1)*lineSize + offset;
				yuv24.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = -bhi.biHeight; line > 0; line-=2) {
			for (offset = 0; offset < lineSize; offset += 6) {
				i = (line - 1)*lineSize + offset;
				UV[0] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line - 1)*lineSize + offset + 3;
				UV[1] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line - 2)*lineSize + offset;
				UV[2] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line - 2)*lineSize + offset + 3;
				UV[3] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				yuv24.yuvU = (UV[0] + UV[1] + UV[2] + UV[3]) >> 2;
				if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = -bhi.biHeight; line > 0; line -= 2) {
			for (offset = 0; offset < lineSize; offset += 6) {
				i = (line - 1)*lineSize + offset;
				UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line - 1)*lineSize + offset + 3;
				UV[1] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line - 2)*lineSize + offset;
				UV[2] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line - 2)*lineSize + offset + 3;
				UV[3] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				yuv24.yuvV = (UV[0] + UV[1] + UV[2] + UV[3]) >> 2;
				if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '2' && bhi.biHeight>>31 == 0) {
		//写Y
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = line*lineSize + offset;
				yuv24.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 6) {
				i = line*lineSize + offset;
				yuv24.yuvU = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 6) {
				i = line*lineSize + offset;
				yuv24.yuvV = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '2' && bhi.biHeight >> 31 != 0) {
		lineSize = bhi.biSizeImage / (-bhi.biHeight);
		//写Y
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = (line - 1)*lineSize + offset;
				yuv24.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 6) {
				i = (line - 1)*lineSize + offset;
				yuv24.yuvU = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = -bhi.biHeight; line > 0; line --) {
			for (offset = 0; offset < lineSize; offset += 6) {
				i = (line - 1)*lineSize + offset;
				yuv24.yuvV = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '4' && bhi.biHeight>>31 == 0) {
		//写Y
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = line*lineSize + offset;
				yuv24.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = line*lineSize + offset;
				yuv24.yuvU = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = line*lineSize + offset;
				yuv24.yuvV = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '4' && bhi.biHeight >> 31 != 0) {
		lineSize = bhi.biSizeImage / (-bhi.biHeight);
		//写Y
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = (line - 1)*lineSize + offset;
				yuv24.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = (line - 1)*lineSize + offset;
				yuv24.yuvU = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = -bhi.biHeight; line > 0; line --) {
			for (offset = 0; offset < lineSize; offset += 3) {
				i = (line - 1)*lineSize + offset;
				yuv24.yuvV = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
}


/*
输入参数：
fv：yuv文件指针；
databuf：数据缓冲区；
yuv：模式；
bhi：bmp文件信息
功能：将rgb共32位的bmp文件转换为指定格式的yuv文件
返回值：无
*/
void writeyuv32(FILE *fv, BYTE databuf[], int yuvmode, BMPHeaderInfo bhi)
{
	unsigned long i, count = 0;
	int line, offset;
	YUV yuv32;
	int UV[4];
	DWORD lineSize = bhi.biSizeImage / bhi.biHeight;

	if (yuvmode == '0' && bhi.biHeight>>31 == 0) {
		//写Y
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = line*lineSize + offset;
				yuv32.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv32.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = 0; line < bhi.biHeight; line+=2) {
			for (offset = 0; offset < lineSize; offset += 8) {
				i = line*lineSize + offset;
				UV[0] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = line*lineSize + offset + 4;
				UV[1] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line+1)*lineSize + offset;
				UV[2] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line+1)*lineSize + offset + 4;
				UV[3] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				yuv32.yuvU = (UV[0] + UV[1] + UV[2] + UV[3]) >> 2;
				if ((fwrite(&(yuv32.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = 0; line < bhi.biHeight; line+=2) {
			for (offset = 0; offset < lineSize; offset += 8) {
				i = line*lineSize + offset;
				UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = line*lineSize + offset + 4;
				UV[1] = UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line+1)*lineSize + offset;
				UV[2] = UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line+1)*lineSize + offset + 4;
				UV[3] = UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				yuv32.yuvV = (UV[0] + UV[1] + UV[2] + UV[3]) >> 2;
				if ((fwrite(&(yuv32.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '0' && bhi.biHeight >> 31 != 0) {
		lineSize = bhi.biSizeImage / (-bhi.biHeight);
		//写Y
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = (line - 1)*lineSize + offset;
				yuv32.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv32.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = -bhi.biHeight; line > 0; line-=2) {
			for (offset = 0; offset < lineSize; offset += 8) {
				i = (line - 1)*lineSize + offset;
				UV[0] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line - 1)*lineSize + offset + 4;
				UV[1] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line - 2)*lineSize + offset;
				UV[2] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				i = (line - 2)*lineSize + offset + 4;
				UV[3] = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;	
				yuv32.yuvU = (UV[0] + UV[1] + UV[2] + UV[3]) >> 2;
				if ((fwrite(&(yuv32.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = -bhi.biHeight; line > 0; line -= 2) {
			for (offset = 0; offset < lineSize; offset += 8) {
				i = (line - 1)*lineSize + offset;
				UV[0] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line - 1)*lineSize + offset + 4;
				UV[1] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line - 2)*lineSize + offset;
				UV[2] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				i = (line - 2)*lineSize + offset + 4;
				UV[3] = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				yuv32.yuvV = (UV[0] + UV[1] + UV[2] + UV[3]) >> 2;
				if ((fwrite(&(yuv32.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '2' && bhi.biHeight>>31 == 0) {
		//写Y
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = line*lineSize + offset;
				yuv32.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv32.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 8) {
				i = line*lineSize + offset;
				yuv32.yuvU = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				if ((fwrite(&(yuv32.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 8) {
				i = line*lineSize + offset;
				yuv32.yuvV = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				if ((fwrite(&(yuv32.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '2' && bhi.biHeight >> 31 != 0) {
		lineSize = bhi.biSizeImage / (-bhi.biHeight);
		//写Y
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = (line - 1)*lineSize + offset;
				yuv32.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv32.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 8) {
				i = (line - 1)*lineSize + offset;
				yuv32.yuvU = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				if ((fwrite(&(yuv32.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = -bhi.biHeight; line > 0; line --) {
			for (offset = 0; offset < lineSize; offset += 8) {
				i = (line - 1)*lineSize + offset;
				yuv32.yuvV = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				if ((fwrite(&(yuv32.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '4' && bhi.biHeight>>31 == 0) {
		//写Y
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = line*lineSize + offset;
				yuv32.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv32.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = line*lineSize + offset;
				yuv32.yuvU = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				if ((fwrite(&(yuv32.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = 0; line < bhi.biHeight; line++) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = line*lineSize + offset;
				yuv32.yuvV = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				if ((fwrite(&(yuv32.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (yuvmode == '4' && bhi.biHeight >> 31 != 0) {
		lineSize = bhi.biSizeImage / (-bhi.biHeight);
		//写Y
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = (line - 1)*lineSize + offset;
				yuv32.yuvY = 0.257*databuf[i + 2] + \
					0.504*databuf[i + 1] + 0.098*databuf[i] + 16;
				if ((fwrite(&(yuv32.yuvY), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写U
		for (line = -bhi.biHeight; line > 0; line--) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = (line - 1)*lineSize + offset;
				yuv32.yuvU = -0.148*databuf[i + 2] - \
					0.291*databuf[i + 1] + 0.439*databuf[i] + 128;
				if ((fwrite(&(yuv32.yuvU), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		//写V
		for (line = -bhi.biHeight; line > 0; line --) {
			for (offset = 0; offset < lineSize; offset += 4) {
				i = (line - 1)*lineSize + offset;
				yuv32.yuvV = 0.439*databuf[i + 2] - \
					0.368*databuf[i + 1] - 0.071*databuf[i] + 128;
				if ((fwrite(&(yuv32.yuvV), sizeof(BYTE), 1, fv)) == 0) {
					printf("wtite error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
}

//映射yuv的颜色表
void calculateYUV(YUV *yuvcolor, RGB rgbcolor)
{
	yuvcolor->yuvY = 0.257*rgbcolor.rgbRed + \
		0.504*rgbcolor.rgbGreen + 0.098*rgbcolor.rgbBlue + 16;
	yuvcolor->yuvU = -0.148*rgbcolor.rgbRed - \
		0.291*rgbcolor.rgbGreen + 0.439*rgbcolor.rgbBlue + 128;
	yuvcolor->yuvV = 0.439*rgbcolor.rgbRed - \
		0.368*rgbcolor.rgbGreen - 0.071*rgbcolor.rgbBlue + 128;
}

//输出BMPFileHead，文件头
void printBMPFileHead(BMPFileHead* bfh)
{
	if (bfh == NULL)
	{
		printf("BMPFileHead is NULL!\n");
		return;
	}

	printf("\nBMPFileHead:\n");
	printf("\t%d\n", sizeof(BMPFileHead));
	printf("\tbfType=0x%x\n", bfh->bfType);
	printf("\tbfSize=%u\n", bfh->bfSize);
	printf("\tbfReserved1=%u\n", bfh->bfReserved1);
	printf("\tbfReserved2=%u\n", bfh->bfReserved2);
	printf("\tbfOffsetBytes=%u\n", bfh->bfOffsetBytes);
	printf("\n");

	return;
}

//输出BMPHeaderInfo，头信息
void printBMPHeaderInfo(BMPHeaderInfo* bhi)
{
	if (bhi == NULL)
	{
		printf("BMPHeaderInfo is NULL!\n");
		return;
	}

	printf("\n%d %d %d %d\n", sizeof(BYTE), sizeof(WORD), sizeof(DWORD), sizeof(LONG));

	printf("\nBMPHeaderInfo:\n");
	printf("\t%d\n", sizeof(BMPHeaderInfo));
	printf("\tbiSize=%u\n", bhi->biSize);
	printf("\tbiWidth=%d\n", bhi->biWidth);
	printf("\tbiHeight=%d\n", bhi->biHeight);
	printf("\tbiPlanes=%u\n", bhi->biPlanes);
	printf("\tbiBitCount=%u\n", bhi->biBitCount);
	printf("\tbiCompression=%u\n", bhi->biCompression);
	printf("\tbiSizeImage=%u\n", bhi->biSizeImage);
	printf("\tbiXPixelsPerMeter=%u\n", bhi->biXPixelsPerMeter);
	printf("\tbiYPixelsPerMeter=%u\n", bhi->biYPixelsPerMeter);
	printf("\tbiClrUsed=%u\n", bhi->biClrUsed);
	printf("\tbiClrImportant=%u\n", bhi->biClrImportant);
	printf("\n");

	return;
}


