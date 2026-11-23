#include "../inc/touch.h"

int ts_fd;
//初始化触摸屏
int Touch_Init()
{
    ts_fd = open("/dev/input/event0", O_RDWR);
    if (ts_fd < 0)
    {
        printf("open /dev/input/event0 failed\n");
        return -1;
    }
    return 0;
}
//销毁触摸屏
int Touch_Destroy()
{
    close(ts_fd);
    return 0;
}

//获取坐标值
int get_xy(int *tx,int *ty)
{
    int x, y;
    // 定义输入子系统结构体
    struct input_event ts;
    while (1)
    {
        // 读取触摸屏驱动数据,如果有触摸屏事件产生，将其原始数据读入子系统中
        int ret = read(ts_fd, &ts, sizeof(ts));
        // 解析子系统中的数据内容
        if (ts.type == EV_ABS) // 触摸屏事件
        {
            if (ts.code == ABS_X) // x轴
            {
                x = ts.value * 800 / 1024;
            }
            else if (ts.code == ABS_Y) // y轴
            {
                y = ts.value * 480 / 600;
            }
        }
        // 检测手指离开屏幕的坐标
        if (ts.type == EV_KEY && ts.code == BTN_TOUCH && ts.value == 0)
        {
            *tx = x;
            *ty = y;
            printf("x:%d,y:%d\n", *tx, *ty);
            break;
        }
    }
    return 0;
}

//获取滑动：x0,y0：手指放下的起始坐标 x1,y1:手指抬起的终点坐标
int get_slide(int *x0,int *y0,int *x1,int *y1)
{
    struct input_event ts;
    //上次有效坐标跨调用保存：极快点按可能一个坐标都不报，此时沿用上次位置
    static int last_x=400,last_y=240;
    int x=last_x, y=last_y;
    int pressed=0;     //手指是否按下
    int start_set=0;   //起点是否已记录
    while (1)
    {
        read(ts_fd, &ts, sizeof(ts));
        //触摸屏事件：坐标无条件累积（本屏驱动先报坐标后报按下事件）
        if (ts.type == EV_ABS)
        {
            if (ts.code == ABS_X)
                x = ts.value * 800 / 1024;
            else if (ts.code == ABS_Y)
                y = ts.value * 480 / 600;
            //兜底：按下事件之后才来坐标的驱动顺序，起点还没记就先记
            if (pressed && !start_set)
            {
                *x0 = x;
                *y0 = y;
                start_set = 1;
            }
        }
        //按键事件
        if (ts.type == EV_KEY && ts.code == BTN_TOUCH)
        {
            //手指刚按下：刚累积的坐标就是按下点（按下事件前驱动刚发过坐标）
            if (ts.value == 1)
            {
                pressed = 1;
                if (!start_set)
                {
                    *x0 = x;
                    *y0 = y;
                    start_set = 1;
                }
            }
            //手指离开
            else if (ts.value == 0)
            {
                *x1 = x;
                *y1 = y;
                last_x = x;
                last_y = y;
                break;
            }
        }
    }
    return 0;
}

