#ifndef __BMP_H
#define __BMP_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "font.h"
#include "lcddev.h"

//全局字库指针，main.c里fontLoad后可用
extern font *f;

//显示图片
int Show_Bmp(const char* filename);
//显示开机动画
int Show_Open(char *path);
//在任意位置，显示任意大小的图片
int Show_Any_Bmp(const char* filename,int bmp_w,int bmp_h,int x0,int y0);
//图标叠加：白色线条图标（黑底）按亮度混合，黑色全透明，无方块底；宽需为4的倍数
int Show_Icon(const char* filename,int bmp_w,int bmp_h,int x0,int y0);
//显示图片的某一块区域：从图片的(sx,sy)处取sw×sh大小，贴到屏幕(x0,y0)处
int Show_Part_Bmp(const char* filename,int bmp_w,int bmp_h,int x0,int y0,int sx,int sy,int sw,int sh);

/**
 * @brief 在LCD屏幕指定位置打印一行文字
 * @param lcd      LCD设备指针，由create_lcd创建
 * @param f        字库指针，fontLoad加载的字体
 * @param lcd_x    文字画布在LCD上的起始X坐标
 * @param lcd_y    文字画布在LCD上的起始Y坐标
 * @param bmp_w    文字画布(bitmap)宽度
 * @param bmp_h    文字画布(bitmap)高度
 * @param bg_color 画布背景色 RGB
 * @param font_x   文字在bitmap内部的X偏移
 * @param font_y   文字在bitmap内部的Y偏移
 * @param str      要打印的字符串
 * @param font_color 文字颜色 RGB
 * @param font_size 字体大小
 */
void Lcd_DrawText(struct LcdDev *lcd, font *f,
                  int lcd_x, int lcd_y,
                  int bmp_w, int bmp_h,
                  unsigned int bg_color,
                  int font_x, int font_y,
                  char *str,
                  unsigned int font_color,
                  int font_size);



#endif 