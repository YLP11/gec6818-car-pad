#include "../inc/lcddev.h"
#include <string.h>
struct LcdDev *lcd = NULL;


//绘制点的函数
void draw_point(int x, int y)
{
    if(x<lcd->w && y <lcd->h)
        *(lcd->mptr+y*lcd->w+x) = 0xff0000; 
}

void swap(int *dx, int *dy)
{
    int t = *dx;
    *dx = *dy;
    *dy = t;
}

//绘制直线
void draw_line(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x2 > x1) ? 1 : -1;
    int sy = (y2 > y1) ? 1 : -1;
    int flag = 0;
    if (dx < dy) {
        flag = 1;
        swap(&dx, &dy);
        swap(&x1, &y1);
        swap(&x2, &y2);
    }
    int d = 2 * dy - dx;
    int steps = dx;
    int x = x1, y = y1;
    draw_point(x, y); // 绘制起点
    for (int i = 0; i < steps; ++i) {
        if (d >= 0) {
            y += sy;
            d += 2 * (dy - dx);
        } else {
            d += 2 * dy;
        }
        x += sx;
        if (flag) draw_point(y, x); // 若交换过，则输出时交换坐标
        else draw_point(x, y);
    }
}


//带颜色的画线（draw_line是写死红色的，这个才是可用的）
void draw_line_c(int x0, int y0, int x1, int y1, unsigned int color)
{
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = (x1 > x0) ? 1 : -1;
    int sy = (y1 > y0) ? 1 : -1;
    int err = dx - dy;
    while (1)
    {
        if (x0 >= 0 && x0 < lcd->w && y0 >= 0 && y0 < lcd->h)
            lcd->mptr[y0 * lcd->w + x0] = color;
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

//实心圆
void draw_fill_circle(int cx, int cy, int r, unsigned int color)
{
    int x, y;
    for (y = -r; y <= r; y++)
    {
        for (x = -r; x <= r; x++)
        {
            if (x * x + y * y <= r * r)
            {
                int px = cx + x, py = cy + y;
                if (px >= 0 && px < lcd->w && py >= 0 && py < lcd->h)
                    lcd->mptr[py * lcd->w + px] = color;
            }
        }
    }
}

//实心三角形（三个顶点），叉积同号判定点在三角形内
void draw_fill_tri(int ax, int ay, int bx, int by, int cx, int cy, unsigned int color)
{
    int minx = ax < bx ? (ax < cx ? ax : cx) : (bx < cx ? bx : cx);
    int maxx = ax > bx ? (ax > cx ? ax : cx) : (bx > cx ? bx : cx);
    int miny = ay < by ? (ay < cy ? ay : cy) : (by < cy ? by : cy);
    int maxy = ay > by ? (ay > cy ? ay : cy) : (by > cy ? by : cy);
    int x, y;
    for (y = miny; y <= maxy; y++)
    {
        for (x = minx; x <= maxx; x++)
        {
            //三个叉积
            int d1 = (x - ax) * (by - ay) - (y - ay) * (bx - ax);
            int d2 = (x - bx) * (cy - by) - (y - by) * (cx - bx);
            int d3 = (x - cx) * (ay - cy) - (y - cy) * (ax - cx);
            int neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            int pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
            if (!(neg && pos))
            {
                if (x >= 0 && x < lcd->w && y >= 0 && y < lcd->h)
                    lcd->mptr[y * lcd->w + x] = color;
            }
        }
    }
}

//在lcd上绘制矩形
void  draw_rect(int x, int y, int w, int h, unsigned int color)
{
    //在x=200，y=300位置显示100x100矩形框
    unsigned int *p=lcd->mptr+y*lcd->w+x;
    for(int i=0; i<h; i++)
    {
        for(int j=0; j<w; j++)
        {
            p[j] =color;
        }
        //p移动到下一行
        p+=lcd->w;
    }
}

//在lcd上绘制圆形
void draw_circle(int x, int y, int w, int h)
{

}

//在lcd上绘制RGB数据-图片
void draw_rgb(int x, int y, char *rgb, int rgbw, int rgbh)
{
    
    //把lcd指针移动到绘制开始位置
    unsigned int *p=lcd->mptr+y*lcd->w+x;
    //计算要绘制的宽和高
    int dw = (x+rgbw > lcd->w)?(lcd->w-x) : rgbw;
    int dh = (y+rgbh > lcd->h)?(lcd->h-y) : rgbh;

    for(int i=0; i<dh; i++)
    {
        for(int j=0; j<dw; j++)
        {
            memcpy(p+j, rgb+j*3, 3);
        }
        //屏幕和rgb数据都移动到下一行
        p += lcd->w;
        rgb += rgbw*3;
    }

}


//创建显示屏
struct LcdDev *create_lcd(const char *devname)
{
    if(lcd == NULL){
        lcd = malloc(sizeof(struct LcdDev));
        //1.打开文件/dev/fb0
        lcd->fd = open(devname, O_RDWR);
        if(lcd->fd < 0)
        {
            perror("open error");
            free(lcd);
            return NULL;
        }

        //获取属性
        //定义存储属性的变量
        struct fb_var_screeninfo  vinfo;
        if(ioctl(lcd->fd, FBIOGET_VSCREENINFO, &vinfo) != 0)
        {
            perror("获取LCD设备可变属性信息失败");
            free(lcd);
            return NULL;
        }
        lcd->w = vinfo.xres;
        lcd->h = vinfo.yres;
        //下面是虚拟机lcd尺寸
        //lcd->w = vinfo.xres_virtual;  
        //lcd->h = vinfo.yres_virtual;
        lcd->pix = vinfo.bits_per_pixel/8;

        //映射
        lcd->mptr = mmap(NULL, lcd->w*lcd->h*lcd->pix, PROT_READ|PROT_WRITE, MAP_SHARED,lcd->fd, 0);
        if(lcd->mptr == MAP_FAILED)
        {
            perror("mmap");
        }


        //初始化函数
        lcd->draw_line = draw_line;
        lcd->draw_rect = draw_rect;
        lcd->draw_rgb = draw_rgb;
    }
    return lcd;
}

//销毁显示屏
bool destroy_lcd()
{
    if(lcd != NULL)
    {
        munmap(lcd->mptr, lcd->w*lcd->h*lcd->pix);
        close(lcd->fd);
        free(lcd);
        lcd = NULL;
    }
    return true;
}

