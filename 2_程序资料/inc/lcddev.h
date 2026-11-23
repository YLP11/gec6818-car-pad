#ifndef _LCDDEV_H
#define _LCDDEV_H
#include <stdio.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

/* LCD 设备对象，保存 framebuffer 信息和绘图接口。 */
struct LcdDev{
    int fd;                // /dev/fb0 文件描述符
    unsigned int *mptr;    // mmap 后的显存首地址
    int w,h,pix;           // 屏幕宽、高、每像素字节数，实际值由 framebuffer 获取

    /**
     * 绘制直线。
     * @param x0 起点横坐标，推荐范围 0 <= x0 < lcd->w
     * @param y0 起点纵坐标，推荐范围 0 <= y0 < lcd->h
     * @param x1 终点横坐标，推荐范围 0 <= x1 < lcd->w
     * @param y1 终点纵坐标，推荐范围 0 <= y1 < lcd->h
     * @return 无返回值
     * @note 当前 draw_point() 只检查坐标上限，没有检查负数；不要传负坐标。
     */
    void (*draw_line)(int x0, int y0, int x1, int y1);

    /**
     * 绘制纯色矩形。
     * @param x 矩形左上角横坐标，范围应为 0 <= x < lcd->w
     * @param y 矩形左上角纵坐标，范围应为 0 <= y < lcd->h
     * @param w 矩形宽度，必须大于 0，且 x + w <= lcd->w
     * @param h 矩形高度，必须大于 0，且 y + h <= lcd->h
     * @param color 矩形颜色
     * @return 无返回值
     * @note 当前实现没有做边界裁剪，超出屏幕可能写越界；不要传负值或越界尺寸。
     */
    void (*draw_rect)(int x, int y, int w, int h, unsigned int color);

    /**
     * 将 RGB 像素数据绘制到 LCD。
     * @param x RGB 图像显示到 LCD 的横坐标，推荐范围 0 <= x < lcd->w
     * @param y RGB 图像显示到 LCD 的纵坐标，推荐范围 0 <= y < lcd->h
     * @param rgb RGB 像素数据首地址，本工程传入 bm->map
     * @param rgbw RGB 图像宽度，必须大于 0；建议满足 x + rgbw <= lcd->w
     * @param rgbh RGB 图像高度，必须大于 0；建议满足 y + rgbh <= lcd->h
     * @return 无返回值
     * @note 当前实现按 3 字节 RGB 读取数据，需要和 createBitmapWithInit(..., 3, ...) 匹配。
     *       右边和下边超出屏幕时会裁剪，但 x/y 不能为负数，rgb 不能为 NULL。
     */
    void (*draw_rgb)(int x, int y, char *rgb, int rgbw, int rgbh);
};

/* 全局 LCD 设备对象，create_lcd() 成功后可用，其他文件通过它访问显存。 */
extern struct LcdDev *lcd;

//带颜色的画线（Bresenham），颜色格式 0x00RRGGBB
void draw_line_c(int x0, int y0, int x1, int y1, unsigned int color);
//实心圆，颜色格式 0x00RRGGBB
void draw_fill_circle(int cx, int cy, int r, unsigned int color);
//实心三角形（三个顶点），颜色格式 0x00RRGGBB
void draw_fill_tri(int ax, int ay, int bx, int by, int cx, int cy, unsigned int color);

/**
 * 创建并初始化 LCD 设备。
 * @param devname framebuffer 设备路径，通常为 "/dev/fb0"
 * @return 成功返回 LcdDev 指针；失败返回 NULL
 * @note devname 通常为 "/dev/fb0"；内部会 open、ioctl、mmap。
 *       创建成功后才能使用 draw_line、draw_rect、draw_rgb。
 */
struct LcdDev* create_lcd(const char *devname);

/**
 * 销毁 LCD 设备并释放资源。
 * @return 接口声明为 bool；当前实现未显式返回值，调用时只关心资源是否被释放
 * @note 会解除 mmap、关闭 fd、释放 LcdDev 结构体；只能在 create_lcd() 成功后调用。
 */
bool destroy_lcd();
#endif
