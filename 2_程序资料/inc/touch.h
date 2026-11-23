#ifndef __TOUCH_H
#define __TOUCH_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/input.h>

extern int ts_fd;

int Touch_Init(void);
int Touch_Destroy(void);
int get_xy(int *tx,int *ty);//获取坐标值
int get_slide(int *x0,int *y0,int *x1,int *y1);//获取滑动：按下起点+抬起终点

#endif
