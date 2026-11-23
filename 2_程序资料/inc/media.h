#ifndef __MEDIA_H
#define __MEDIA_H

// ==================== 音乐（madplay 后台播放） ====================

#define MUSIC_DIR "/root/car/res/system/music"
#define SONG_NUM 3

//三个按钮图标热区（触摸屏有偏差，y 400~480 整条响应，x 按图标中心均分）
#define BTN_BAND_Y 400
#define BTN_PREV_L 280
#define BTN_PREV_R 386
#define BTN_PLAY_R 423
#define BTN_NEXT_R 530
//收起箭头热区（实测中心92,21，热区扩大到箭头周围一大片）
#define BACK_W 140
#define BACK_H 70

//到头提示条（和"已设为壁纸"同款：400x64 深底绿字，弹屏幕中心）
#define TOAST_W 400
#define TOAST_H 64
#define TOAST_FIRST MUSIC_DIR "/toast_first.bmp"
#define TOAST_LAST MUSIC_DIR "/toast_last.bmp"

// ==================== 视频（mplayer 直播到 framebuffer） ====================

#define VIDEO_DIR "/root/car/res/system/video"
#define BACK_IMG "/root/car/res/system/photo/ui_back.bmp"
#define COL_BG 0x00F7F2EA   //整屏底色 暖白 RGB(247,242,234)
#define COL_BAR 0x000A0C12  //标题栏 RGB(10,12,18)
#define TXT_BAR 0x0A0C1200  //字库画布色0xRRGGBB00：RGB(10,12,18)
#define TXT_BG 0xEEE6D800   //字库画布色0xRRGGBB00：RGB(238,230,216)与条目浅暖灰一致
#define COL_ITEM 0x00EEE6D8 //列表条目 浅暖灰 RGB(238,230,216)
#define COL_TXTD 0xff4a4238 //暖白底上的深字 RGB(74,66,56)
#define BAR_H 50
#define VID_NUM 2
#define THUMB_W 320 //缩略图尺寸（宽是4的倍数，Show_Any_Bmp不歪）
#define THUMB_H 180

//暂停菜单图：340x140 弹在屏幕中心(240,170)
//图内按钮："继续播放" x 34~164、"退出" x 176~306，y 66~120
#define PMENU_IMG "/root/car/res/system/video/pause_menu.bmp"
#define PMENU_X 240
#define PMENU_Y 170
#define PMENU_GO_L 274 //继续播放热区（绝对坐标）
#define PMENU_GO_R 404
#define PMENU_EX_L 416 //退出热区
#define PMENU_EX_R 546
#define PMENU_BTN_Y0 242
#define PMENU_BTN_Y1 296

//音乐播放页：播放/暂停/上一曲/下一曲（madplay 后台播放），退出时自动停掉 madplay
void Music_UI(void);
//视频列表页：点条目播放，点按暂停弹菜单，左上角返回
void Video_UI(void);

#endif
