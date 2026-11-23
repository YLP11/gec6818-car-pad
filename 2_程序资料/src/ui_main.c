#include "../inc/ui_main.h"
#include "../inc/bmp.h"
#include "../inc/touch.h"
#include "../inc/album.h"
#include "../inc/media.h"
#include "../inc/game.h"
#include "../inc/game.h"
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

//卡片尺寸和布局（含外圈8px立体阴影+圆角，阴影已烘进卡片图里）
#define CARD_W   216
#define CARD_H   342
#define CARD_Y   67    //贴图Y（卡体实际在75，上移了阴影圈）
#define CARD_GAP 36    //卡与卡之间的空隙
#define CARD_NUM 4     //卡片数量
#define CARD_CX  292   //焦点卡左上角X（200宽卡体居中在300）

//四张功能卡片图：相册 音乐 视频播放 车载游戏
static const char *card_img[CARD_NUM]=
{
    "/root/car/res/system/main/card_album.bmp",
    "/root/car/res/system/main/card_music.bmp",
    "/root/car/res/system/main/card_video.bmp",
    "/root/car/res/system/main/card_game.bmp",
};

//显示日期+时间到顶部正中（背景图顶部已画好深色横栏 RGB(10,12,18)，画布色与栏一致所以无缝）
//注意：字库画布色是0xRRGGBB00，RGB(10,12,18)要写成0x0A0C1200
static void Show_Time(void)
{
    time_t t=time(NULL);
    struct tm *tm=localtime(&t);
    char buf[32];
    sprintf(buf,"%d-%02d-%02d  %02d:%02d:%02d",
            1900+tm->tm_year,tm->tm_mon+1,tm->tm_mday,
            tm->tm_hour,tm->tm_min,tm->tm_sec);
    Lcd_DrawText(lcd,f,230,7,340,32,0x0A0C1200,40,0,buf,0xffffffff,20);
}

//时间线程：每秒刷新一次，时间不用触摸也会自己走
static void *Time_Thread(void *arg)
{
    while(1)
    {
        Show_Time();
        sleep(1);
    }
    return NULL;
}

//显示主界面：背景+四张卡片
static void Main_Show(int focus)
{
    int i;
    Show_Bmp("/root/car/res/system/main/main_bg.bmp");
    //顶栏和角落图标运行时描绘：壁纸换成任意照片后它们依然在
    lcd->draw_rect(0, 0, 800, 44, 0x000A0C12);       //顶部时间横栏 RGB(10,12,18)
    lcd->draw_rect(0, 43, 800, 1, 0x003A404E);       //栏底细亮线 RGB(58,64,78)
    Show_Icon("/root/car/res/system/main/ui_logout.bmp", 56, 56, 17, 421);
    Show_Icon("/root/car/res/system/main/ui_power.bmp", 56, 56, 725, 421);
    Show_Time();
    for(i=0;i<CARD_NUM;i++)
    {
        int x=CARD_CX+(i-focus)*(CARD_W+CARD_GAP);
        int sx=0,w=CARD_W;    //sx:从卡片图内部开始取图的X
        //整张卡在屏幕外，不画
        if(x>=800 || x+CARD_W<=0)
            continue;  
         //左边超出屏幕：只画屏幕内的部分       
        if(x<0)              
        {
            sx=-x;//从卡片的图片的-x像素开始取
            w=CARD_W+x;
            x=0;
        }
        //右边超出屏幕：截断
        if(x+w>800)           
            w=800-x;
        Show_Part_Bmp(card_img[i],CARD_W,CARD_H,x,CARD_Y,sx,0,w,CARD_H);
    }
}

//确认对话框（和相册删除确认同款 420x180，弹在屏幕中心 190,150）
//图内按钮："取消" x 236~386、"确认" x 414~564，y 246~308；点框外也算取消
//返回1=确认 0=取消
static int Confirm_Box(const char *img)
{
    int x0,y0,x1,y1;
    Show_Any_Bmp(img,420,180,190,150);
    while(1)
    {
        get_slide(&x0,&y0,&x1,&y1);
        //按钮判定用按下点(x0,y0)：快速点按时抬起点可能失真
        if(x0>=414 && x0<=564 && y0>=246 && y0<=308)
            return 1;
        if(x0>=236 && x0<=386 && y0>=246 && y0<=308)
            return 0;
        if(!(x0>=190 && x0<=610 && y0>=150 && y0<=330))
            return 0;
    }
}

//主界面：左右滑切换卡片，点击卡片进入对应功能；登出回登录界面，关机关机
int Main_UI(void)
{
    int focus=0;
    int x0,y0,x1,y1;
    pthread_t tid;
    //启动时间线程，时间每秒自动刷新
    pthread_create(&tid,NULL,Time_Thread,NULL);
    while(1)
    {
        Main_Show(focus);
        get_slide(&x0,&y0,&x1,&y1);
        int dx=x1-x0;
        //左滑：切换到下一张
        if(dx<-60)
        {
            if(focus<CARD_NUM-1)
                focus++;
        }
        //右滑：切换到上一张
        else if(dx>60)
        {
            if(focus>0)
                focus--;
        }
        //点击左下角登出图标：先弹确认框（按下点判定，热区大幅加宽防电阻屏偏差）
        else if(x0>=10 && x0<=110 && y0>=415 && y0<=478)
        {
            if(Confirm_Box("/root/car/res/system/main/ui_confirm_logout.bmp"))
            {
                //主动停止时间线程
                pthread_cancel(tid);
                pthread_join(tid,NULL);
                return 0;    //登出，main.c里重新调Login_Register登录注册函数
            }
        }
        //点击右下角关机图标：先弹确认框（按下点判定，热区大幅加宽防电阻屏偏差）
        else if(x0>=690 && x0<=790 && y0>=415 && y0<=478)
        {
            if(Confirm_Box("/root/car/res/system/main/ui_confirm_power.bmp"))
            {
                //先停时间线程：不然动画播放期间它每秒还在盖时间条，画面会闪
                pthread_cancel(tid);
                pthread_join(tid,NULL);
                //关机：先停掉后台播放器，再播关机动画视频
                system("killall madplay mplayer 2>/dev/null");
                system("mplayer -vo fbdev -noconsolecontrols -quiet "
                       "/root/car/res/system/main/power_off.avi >/dev/null 2>&1");
                memset(lcd->mptr,0,800*480*sizeof(int));
                //杀掉本程序进程，关机后不再回到主界面
                kill(getpid(),SIGKILL);
            }
        }
        //点击卡片（按下点判定）：点中的卡片进入对应功能
        else if(y0>=CARD_Y && y0<=CARD_Y+CARD_H)
        {
            int i,hit=-1;
            for(i=0;i<CARD_NUM;i++)
            {
                int cx=CARD_CX+(i-focus)*(CARD_W+CARD_GAP);
                if(x0>=cx && x0<cx+CARD_W)
                {
                    hit=i;
                    break;
                }
            }
            if(hit==focus)
            {
                //四张卡片对应的页面
                if(hit>=0 && hit<=3)
                {
                    //先停时间线程：不然它每秒把顶栏刷成时间，盖掉子页面标题
                    pthread_cancel(tid);
                    pthread_join(tid,NULL);
                    if(hit==0)
                        Album_UI();
                    else if(hit==1)
                        Music_UI();
                    else if(hit==2)
                        Video_UI();
                    else
                        Game_UI();
                    //回主界面，恢复时间线程
                    pthread_create(&tid,NULL,Time_Thread,NULL);
                }
            }
            else if(hit>=0)
            {
                focus=hit;    //点两侧露出的卡片直接切过去
            }
        }
    }
}
