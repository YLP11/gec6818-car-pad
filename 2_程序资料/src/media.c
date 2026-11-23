// 音视频：音乐用 madplay 后台播放，视频用 mplayer 直播到 framebuffer
// 两者都是"后台进程 + SIGSTOP/SIGCONT 暂停 + SIGKILL 停止"的管理方式
#include "../inc/media.h"
#include "../inc/bmp.h"
#include "../inc/touch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// ==================== 音乐 ====================

static const char *g_files[SONG_NUM] = {
    MUSIC_DIR "/song1.mp3", // 太聪明 - 陈绮贞
    MUSIC_DIR "/song2.mp3", // 愿与愁
    MUSIC_DIR "/song3.mp3", // 玻璃
};
static const char *g_play[SONG_NUM] = {
    // 播放中显示
    MUSIC_DIR "/play1.bmp",
    MUSIC_DIR "/play2.bmp",
    MUSIC_DIR "/play3.bmp",
};
static const char *g_pause[SONG_NUM] = {
    // 暂停/未播放显示
    MUSIC_DIR "/pause1.bmp",
    MUSIC_DIR "/pause2.bmp",
    MUSIC_DIR "/pause3.bmp",
};

static int cur = 0;     // 当前歌
static int playing = 0; // 0=没在放 1=播放中
static int paused = 0;  // 播放中是否暂停

static pid_t g_mad_pid = -1; // madplay 进程号：直接对它发信号，比 killall 可靠

// 弹提示条1秒
static void Show_Toast(const char *img)
{
    Show_Any_Bmp(img, TOAST_W, TOAST_H, (800 - TOAST_W) / 2, (480 - TOAST_H) / 2);
    sleep(1);
}

// 播放第 i 首歌
static void Play(int i)
{
    char cmd[200];
    FILE *pp;
    system("killall mplayer madplay 2>/dev/null"); // 先杀旧的，防残留多实例
    g_mad_pid = -1;
    // 后台启动 madplay：echo $! 拿到进程号；输出全部丢弃，不刷串口
    snprintf(cmd, sizeof(cmd), "madplay %s >/dev/null 2>&1 & echo $!", g_files[i]);
    pp = popen(cmd, "r");
    if (pp != NULL)
    {
        int pid = 0;
        if (fscanf(pp, "%d", &pid) == 1 && pid > 0)
            g_mad_pid = pid;
        pclose(pp);
    }
    cur = i;
    playing = 1;
    paused = 0;
}

// 播放/暂停切换：没在放就开始放当前的歌
// SIGSTOP/SIGCONT 是内核级的，进程无法忽略，一定有效
static void Toggle(void)
{
    if (!playing)
    {
        Play(cur);
        return;
    }
    if (g_mad_pid > 0)
        kill(g_mad_pid, paused ? SIGCONT : SIGSTOP);
    else // 万一没拿到进程号，退回 killall（数字信号最通用）
        system(paused ? "killall -18 madplay 2>/dev/null"
                      : "killall -19 madplay 2>/dev/null");
    paused = !paused;
}

// 停掉 madplay（退出音乐页时必须调）
static void Stop(void)
{
    if (g_mad_pid > 0)
        kill(g_mad_pid, SIGKILL);
    system("killall madplay 2>/dev/null");
    g_mad_pid = -1;
    playing = 0;
    paused = 0;
}

// 上一首/下一首切歌：只换歌不播放，停在未播放状态，点播放键才开始放
static void Switch(int i)
{
    Stop(); // 杀掉正在放的进程
    cur = i;
}

// 整页重绘：播放中显示 play 图，否则显示 pause 图
static void Show_Page(void)
{
    if (playing && !paused)
        Show_Any_Bmp(g_play[cur], 800, 480, 0, 0);
    else
        Show_Any_Bmp(g_pause[cur], 800, 480, 0, 0);
}

void Music_UI(void)
{
    int x0, y0, x1, y1;
    Show_Page();
    while (1)
    {
        get_slide(&x0, &y0, &x1, &y1);
        int dx = x1 - x0;
        // 用按下点(x0,y0)判定：快速双击时抬起点可能失真
        // 返回键
        if (x0 < BACK_W && y0 < BACK_H)
        {
            Stop();
            return;
        }
        // 左滑：下一首
        if (dx < -60)
        {
            if (cur == SONG_NUM - 1)
                Show_Toast(TOAST_LAST);
            else
                Switch(cur + 1);
            Show_Page();
            continue;
        }
        // 右滑：上一首
        if (dx > 60)
        {
            if (cur == 0)
                Show_Toast(TOAST_FIRST);
            else
                Switch(cur - 1);
            Show_Page();
            continue;
        }
        // 底部热区整条都响应
        if (y0 >= BTN_BAND_Y && x0 >= BTN_PREV_L && x0 <= BTN_NEXT_R)
        {
            if (x0 < BTN_PREV_R) // 上一首
            {
                if (cur == 0)
                    Show_Toast(TOAST_FIRST); // 已是第一首，不循环
                else
                    Switch(cur - 1);
            }
            else if (x0 < BTN_PLAY_R) // 播放/暂停
                Toggle();
            else // 下一首
            {
                if (cur == SONG_NUM - 1)
                    Show_Toast(TOAST_LAST); // 已是最后一首，不循环
                else
                    Switch(cur + 1);
            }
            Show_Page();
        }
    }
}

// ==================== 视频 ====================

static pid_t g_play_pid = -1; // mplayer 进程号：直接对它发信号，比 killall 可靠

static const char *g_vfiles[VID_NUM] = {VIDEO_DIR "/video1.avi", VIDEO_DIR "/video2.avi"};
static const char *g_names[VID_NUM] = {"大雄兔历险记", "小驼羊"};
static const char *g_thumbs[VID_NUM] = {VIDEO_DIR "/thumb1.bmp", VIDEO_DIR "/thumb2.bmp"};

// mplayer 参数：
//   -vo fbdev          解码画面直接输出到 framebuffer（视频本身800x480，全屏播放）
//   -loop 0            无限循环播放
//   -noconsolecontrols 后台运行时不读键盘，避免出错退出
static void Player_Start(int i)
{
    char cmd[300];
    FILE *pp;
    // 播放视频前先终止音乐
    system("killall madplay mplayer 2>/dev/null");
    g_play_pid = -1;
    // 后台启动 mplayer：echo $! 拿到进程号
    snprintf(cmd, sizeof(cmd),
             "mplayer -vo fbdev -loop 0 -noconsolecontrols -quiet %s >/dev/null 2>&1 & echo $!",
             g_vfiles[i]);
    pp = popen(cmd, "r");
    if (pp != NULL)
    {
        int pid = 0;
        if (fscanf(pp, "%d", &pid) == 1 && pid > 0)
            g_play_pid = pid;
        pclose(pp);
    }
}

// 暂停/继续：SIGSTOP/SIGCONT 让画面和声音一起停走，天然同步（内核级信号必定生效）
static void Player_Pause(int pause)
{
    if (g_play_pid > 0)
        kill(g_play_pid, pause ? SIGSTOP : SIGCONT);
    else // 万一没拿到进程号，退回 killall 数字信号
        system(pause ? "killall -19 mplayer 2>/dev/null"
                     : "killall -18 mplayer 2>/dev/null");
}

// 退出播放页时停掉 mplayer
static void Player_Stop(void)
{
    if (g_play_pid > 0)
        kill(g_play_pid, SIGKILL);
    system("killall mplayer 2>/dev/null");
    g_play_pid = -1;
}

// 播放页：点列表第 i 个进来；播放中无任何横栏，点按暂停弹菜单
static void Play_Page(int i)
{
    int x0, y0, x1, y1;
    Player_Start(i);
    sleep(1); // 给 mplayer 一点启动时间
    while (1)
    {
        get_slide(&x0, &y0, &x1, &y1);
        int dx = x1 - x0, dy = y1 - y0;
        // 左滑：重播一遍
        if (dx < -60)
        {
            Player_Start(i);
            sleep(1);
        }
        // 单击：暂停并弹出菜单（画面声音一起停，视频冻结后菜单不会被盖掉）
        else if (dx < 40 && dx > -40 && dy < 40 && dy > -40)
        {
            int done = 0;
            Player_Pause(1);
            Show_Any_Bmp(PMENU_IMG, 340, 140, PMENU_X, PMENU_Y);
            while (!done)
            {
                get_slide(&x0, &y0, &x1, &y1);
                int ddx = x1 - x0, ddy = y1 - y0;
                // 按钮判定用按下点(x0,y0)：快速点按时抬起点会失真
                if (x0 >= PMENU_GO_L - 12 && x0 <= PMENU_GO_R + 12 && y0 >= PMENU_BTN_Y0 - 10 && y0 <= PMENU_BTN_Y1 + 10)
                    done = 1; // 继续播放
                else if (x0 >= PMENU_EX_L - 12 && x0 <= PMENU_EX_R + 12 && y0 >= PMENU_BTN_Y0 - 10 && y0 <= PMENU_BTN_Y1 + 10)
                {
                    Player_Stop();
                    return; // 退出回列表
                }
                else if (ddx * ddx + ddy * ddy < 200 &&
                         !(x0 >= PMENU_X && x0 <= PMENU_X + 340 && y0 >= PMENU_Y && y0 <= PMENU_Y + 140))
                    done = 2; // 点菜单外：也继续播放
            }
            Player_Pause(0);
        }
    }
}

// 视频列表页
static void List_Show(void)
{
    int i;
    lcd->draw_rect(0, 0, 800, 480, COL_BG);
    lcd->draw_rect(0, 0, 800, BAR_H, COL_BAR);
    Lcd_DrawText(lcd, f, 340, 10, 140, 34, TXT_BAR, 0, 0, "视频", 0xffffffff, 24);
    Show_Any_Bmp(BACK_IMG, 72, 50, 0, 0);
    for (i = 0; i < VID_NUM; i++)
    {
        int ry = 64 + i * 208; // 条目区 (50, ry, 700, 196)
        lcd->draw_rect(50, ry, 700, 196, COL_ITEM);
        Show_Any_Bmp(g_thumbs[i], THUMB_W, THUMB_H, 70, ry + 8);
        Lcd_DrawText(lcd, f, 410, ry + 72, 300, 44, TXT_BG, 30, 0,
                     (char *)g_names[i], COL_TXTD, 28);
    }
}

void Video_UI(void)
{
    int x0, y0, x1, y1;
    while (1)
    {
        List_Show();
        get_slide(&x0, &y0, &x1, &y1);
        int dx = x1 - x0, dy = y1 - y0;
        if (dx * dx + dy * dy >= 200) // 滑动不响应
            continue;
        // 左上角返回（热区90x60，和相册一致，避开电阻屏贴边盲区；按下点判定）
        if (x0 < 90 && y0 < 60)
            return;
        // 点条目：进入播放页（按下点判定）
        int i = (y0 - 64) / 208;
        int off = (y0 - 64) % 208;
        if (i >= 0 && i < VID_NUM && off < 196 && x0 >= 50 && x0 < 750)
            Play_Page(i);
    }
}
