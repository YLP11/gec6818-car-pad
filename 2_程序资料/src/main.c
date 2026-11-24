#include "../inc/bmp.h"
#include "../inc/touch.h"
#include "../inc/font.h"
#include "../inc/lcddev.h"
#include "../inc/login.h"
#include "../inc/ui_main.h"
#include "../log/log.h"
#include <pthread.h>
#include <sys/stat.h>

//全局字库指针，login.c里画输入框文字用
font *f;

//日志互斥锁：时间线程/贪吃蛇线程会和主线程并发写日志，防止内容交错
static pthread_mutex_t log_mtx = PTHREAD_MUTEX_INITIALIZER;
static void log_lock(bool lock, void *udata)
{
    if (lock)
        pthread_mutex_lock((pthread_mutex_t *)udata);
    else
        pthread_mutex_unlock((pthread_mutex_t *)udata);
}

//日志初始化：日志文件写到板子的 /root/car/log/log.txt
void Log_Init(void)
{
    FILE *fp = fopen("/root/car/log/log.txt", "a");
    if (fp == NULL)
    {
        //目录不存在就先建目录再试一次
        mkdir("/root/car/log", 0777);
        fp = fopen("/root/car/log/log.txt", "a");
    }
    log_set_lock(log_lock, &log_mtx);
    log_set_level(LOG_INFO);      //INFO级别以下的debug/trace不记录
    if (fp != NULL)
        log_add_fp(fp, LOG_INFO); //INFO及以上级别写入文件
}

int main(void)
{
    Log_Init();
    log_info("======== 程序启动 ========");
    //创建lcd设备（lcddev.c内部保存设备指针，绘制函数直接使用）
    create_lcd("/dev/fb0");
    if(Touch_Init()<0)
    {
        return -1;
    }

    //初始化字库
    f = fontLoad("simfang.ttf");
        
    //显示开机动画
    Show_Open("/mnt/sd/car/open_su7");


    //登录 -> 主界面循环：主界面登出后回到登录界面
    while(1)
    {
        Login_Register();
        Main_UI();    //返回=点了登出，重新登录
    }
}


