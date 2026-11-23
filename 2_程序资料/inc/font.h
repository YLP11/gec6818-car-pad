#ifndef __font_h__
#define __font_h__

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#define color u32
#define getColor(a, b, c, d) (a|b<<8|c<<16|d<<24)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef struct stbtt_fontinfo
{
   void           * userdata;
   unsigned char  * data;              // pointer to .ttf file
   int              fontstart;         // offset of start of font

   int numGlyphs;                     // number of glyphs, needed for range checking

   int loca,head,glyf,hhea,hmtx,kern; // table locations as offset from start of .ttf
   int index_map;                     // a cmap mapping for our chosen character encoding
   int indexToLocFormat;              // format needed to map from glyph index to glyph
} stbtt_fontinfo;


typedef struct{
	u32 height;          // 画布高度，必须大于 0
	u32 width;           // 画布宽度，必须大于 0
	u32 byteperpixel;    // 每像素字节数，本工程配合 draw_rgb 使用 3
	u8 *map;             // width * height * byteperpixel 大小的像素缓冲区
}bitmap;

typedef struct{
	stbtt_fontinfo *info;
	u8 *buffer;
	float scale;
}font;



/**
 * 加载 TrueType 字库。
 * @param fontPath 字库文件路径，例如 "simfang.ttf"
 * @return 成功返回 font 指针；失败返回 NULL
 * @note 使用完成后必须调用 fontUnload()。
 */
font *fontLoad(char *fontPath);

/**
 * 设置字体大小。
 * @param f fontLoad() 返回的字体对象
 * @param pixels 目标字体高度，单位为像素，必须大于 0，常用 16~72
 * @return 无返回值
 * @note f 不能为 NULL；字号越大需要的 bitmap 高度越大。
 */
void fontSetSize(font *f, s32 pixels);

/**
 * 创建空白 bitmap 画布。
 * @param width 画布宽度，单位为像素，必须大于 0；不能超过目标 LCD 宽度
 * @param height 画布高度，单位为像素，必须大于 0；不能超过目标 LCD 高度
 * @param byteperpixel 每个像素的字节数，本工程使用 3
 * @return 成功返回 bitmap 指针；失败返回 NULL
 * @note width * height * byteperpixel 会申请对应大小的内存；数值过大会申请失败。
 *       使用完成后必须调用 destroyBitmap()。
 */
bitmap *createBitmap(u32 width, u32 height, u32 byteperpixel);

/**
 * 创建并填充背景色的 bitmap 画布。
 * @param width 画布宽度，单位为像素，必须大于 0；不能超过目标 LCD 宽度
 * @param height 画布高度，单位为像素，必须大于 0；不能超过目标 LCD 高度
 * @param byteperpixel 每个像素的字节数，本工程使用 3
 * @param c 背景颜色
 * @return 成功返回 bitmap 指针；失败返回 NULL
 * @note 本工程的 draw_rgb() 按 3 字节 RGB 读取，因此这里必须传 3。
 *       使用 4 时必须同步修改 draw_rgb()，否则会出现颜色或内存访问错误。
 */
bitmap *createBitmapWithInit(u32 width, u32 height, u32 byteperpixel, color c);


/**
 * 把 UTF-8 字符串绘制到 bitmap 中。
 * @param f 已加载的字体对象
 * @param screen 目标 bitmap
 * @param x 文字在 bitmap 中的横坐标，推荐范围为 0 <= x < screen->width
 * @param y 文字在 bitmap 中的纵坐标，推荐范围为 0 <= y < screen->height
 * @param text 要显示的 UTF-8 字符串
 * @param c 字体颜色
 * @param maxWidth 最大显示宽度；0 表示不限制
 * @return 无返回值
 * @note 只修改内存中的 bitmap，不会直接操作 LCD。
 *       当前实现对负坐标保护不足，x、y 不要传负数；bitmap 高度应足够容纳文字。
 */
void fontPrint(font *f, bitmap *screen, s32 x, s32 y, char *text, color c, s32 maxWidth);


//5.把输出框的所有信息显示到LCD屏幕中 
// bitmap.width
// bitmap.height
// bitmap.bytepixel
// bitmap.map


/**
 * 释放字体对象和字库缓冲区。
 * @param f fontLoad() 返回的字体对象
 * @return 无返回值
 * @note 释放后不能继续使用 f。
 */
void fontUnload(font *f);

/**
 * 释放 bitmap 及其像素数据。
 * @param bm createBitmap() 或 createBitmapWithInit() 返回的画布
 * @return 无返回值
 * @note 释放后不能继续使用 bm。
 */
void destroyBitmap(bitmap *bm);

#endif
