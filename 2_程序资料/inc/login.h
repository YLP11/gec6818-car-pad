#ifndef __LOGIN_H
#define __LOGIN_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// 登录/注册

// 用户数据文件
#define USER_FILE "/root/car/user.txt"

// 提示条图片（贴屏幕顶部居中，白色圆角卡片，绿√成功/红!失败）
#define TOAST_OK "/root/car/res/start/toast_ok.bmp"
#define TOAST_REG_OK "/root/car/res/start/toast_reg_ok.bmp"
#define TOAST_EMPTY "/root/car/res/start/toast_empty.bmp"
#define TOAST_PWD_ERR "/root/car/res/start/toast_pwd_err.bmp"
#define TOAST_EXIST "/root/car/res/start/toast_exist.bmp"
#define TOAST_DIFF "/root/car/res/start/toast_diff.bmp"
#define TOAST_FILE "/root/car/res/start/toast_file.bmp"

// 注册新用户：0成功，-1用户名已存在，-2文件打开失败
int User_Register(const char *name, const char *pwd);
// 登录验证：0成功，-1用户名或密码错误，-2文件打开失败
int User_Login(const char *name, const char *pwd);
// 登录注册页
void Login_Register(void);

// 屏幕键盘

// 键盘图大小
#define KB_W 800
#define KB_H 246
// 键盘在屏幕上的位置（贴屏幕底部）
#define KB_X 0
#define KB_Y 234

// 四个键盘页面
#define PAGE_LOWER 0   // 小写
#define PAGE_CAPITAL 1 // 大写
#define PAGE_NUMBER 2  // 数字
#define PAGE_SYMBOL 3  // 符号

// 特殊按键的功能码
#define KB_SYMBOL 17 // 切换符号键盘
#define KB_NUMBER 18 // 切换数字键盘
#define KB_ABC 19    // 切换英文键盘
#define KB_EXIT 24   // 收起键盘

// 触摸屏有偏差，热区四周各扩大12px用于兜底匹配
#define KEY_PAD 12

// 显示某个键盘页面
void Keyboard_Show(int page);
// 根据触摸坐标返回字符，没点中返回0；点中后按键变暗
char Keyboard_GetChar(int page, int tx, int ty);
// 键盘输入：字符写入buf，按Enter结束；每按一键实时刷新到输入框(bx,by,bw,bh)
// cursor非0：输入框末尾显示"|"表示正在输入
void Keyboard_Input(char *buf, int *len, int max_len, int bx, int by, int bw, int bh, int cursor);

#endif
