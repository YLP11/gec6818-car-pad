#include "../inc/bmp.h"
#include "../inc/touch.h"
#include "../inc/font.h"
#include "../inc/lcddev.h"
#include "../inc/login.h"
#include "../inc/ui_main.h"

//全局字库指针，login.c里画输入框文字用
font *f;

int main(void)
{
    //创建lcd设备
    struct LcdDev *lcd = create_lcd("/dev/fb0");
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


