#include "../inc/game.h"
#include "../log/log.h"
#include "../inc/bmp.h"
#include "../inc/touch.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// 公共界面元素（和相册同款风格）
#define BAR_H 50  // 标题栏高
#define BACK_W 72 // 返回键宽（图实际72宽，传70会歪！）
#define BACK_H 50
#define BACK_IMG "/root/car/res/system/photo/ui_back.bmp"
#define COL_BG 0x00F7F2EA   // 整屏底色 暖白 RGB(247,242,234)
#define COL_BAR 0x000A0C12  // 标题栏 RGB(10,12,18)（保持和其他页面统一）
#define COL_LINE 0x00787062 // 线/棋盘线 暖灰 RGB(120,112,98)
#define COL_CARD 0x00EEE6D8 // 菜单卡片 浅暖灰 RGB(238,230,216)
#define COL_TXTD 0xff4a4238 // 暖白底上的深字 RGB(74,66,56)
// 字库画布色是0xRRGGBB00：RGB(10,12,18)→0x0A0C1200，RGB(247,242,234)→0xF7F2EA00，RGB(238,230,216)→0xEEE6D800
#define TXT_BAR 0x0A0C1200
#define TXT_BG 0xF7F2EA00
#define TXT_CARD 0xEEE6D800

// 画标题栏：标题文字+左上角返回键（各页面共用）
static void Page_Head(const char *title)
{
    lcd->draw_rect(0, 0, 800, BAR_H, COL_BAR);
    Lcd_DrawText(lcd, f, 340, 9, 140, 34, TXT_BAR, 0, 0, (char *)title, 0xffffffff, 24);
    Show_Any_Bmp(BACK_IMG, BACK_W, BACK_H, 0, 0);
}

// 点击是否落在返回键区（热区90x60，比图大，避开电阻屏贴边盲区）
static int Hit_Back(int x, int y)
{
    return x < 90 && y < 60;
}

// 游戏中心菜单
void Game_UI(void)
{
    int x0, y0, x1, y1;
    while (1)
    {
        lcd->draw_rect(0, 0, 800, 480, COL_BG);
        Page_Head("游戏中心");
        // 两张菜单卡片
        lcd->draw_rect(160, 130, 480, 100, COL_CARD);
        lcd->draw_rect(160, 270, 480, 100, COL_CARD);
        Lcd_DrawText(lcd, f, 160, 130, 480, 100, TXT_CARD, 186, 28, "五子棋", COL_TXTD, 36);
        Lcd_DrawText(lcd, f, 160, 270, 480, 100, TXT_CARD, 186, 28, "贪吃蛇", COL_TXTD, 36);
        get_slide(&x0, &y0, &x1, &y1);
        int dx = x1 - x0, dy = y1 - y0;
        if (dx * dx + dy * dy < 200) // 点击（按钮判定用按下点：快速点按时抬起点可能失真）
        {
            if (Hit_Back(x0, y0))
                return;
            // 进入五子棋页面
            if (x0 >= 160 && x0 < 640 && y0 >= 130 && y0 < 230)
                Gomoku_UI();
            // 进入贪吃蛇页面
            else if (x0 >= 160 && x0 < 640 && y0 >= 270 && y0 < 370)
                Snake_UI();
        }
    }
}

// 五子棋
#define B_SIZE 15 // 15路棋盘
#define CELL 28   // 格距
#define B_X0 204  // 棋盘左上角（392宽居中）
#define B_Y0 56
#define STONE_S 24 // 棋子贴图边长（图自带棋盘底色边缘，中心对准交叉点）
#define STONE_B "/root/car/res/system/game/stone_black.bmp"
#define STONE_W "/root/car/res/system/game/stone_white.bmp"
#define WIN_W 400 // 胜利横幅尺寸，显示在屏幕正中
#define WIN_H 120
#define WIN_B "/root/car/res/system/game/win_black.bmp"
#define WIN_IMG "/root/car/res/system/game/win_white.bmp"
#define GAMEOVER_IMG "/root/car/res/system/game/gameover.bmp" // 贪吃蛇结束横幅 400x140
#define PAUSE_IMG "/root/car/res/system/game/pause.bmp"       // 贪吃蛇暂停提示 400x120

static int board[B_SIZE][B_SIZE]; // 0空 1黑 2白

// 判断是否连成五子：从落子点出发四个方向数连子,r:行 c:列 who:黑/白棋
static int Check_Win(int r, int c, int who)
{
    static const int dr[4] = {0, 1, 1, 1};
    static const int dc[4] = {1, 1, 0, -1};
    int d;
    // 扫描四个方向
    for (d = 0; d < 4; d++)
    {
        int n = 1, i;
        for (i = 1; i < 5; i++)
        {
            int rr = r + dr[d] * i, cc = c + dc[d] * i;
            if (rr < 0 || rr >= B_SIZE || cc < 0 || cc >= B_SIZE || board[rr][cc] != who)
                break;
            n++;
        }
        for (i = 1; i < 5; i++)
        {
            int rr = r - dr[d] * i, cc = c - dc[d] * i;
            if (rr < 0 || rr >= B_SIZE || cc < 0 || cc >= B_SIZE || board[rr][cc] != who)
                break;
            n++;
        }
        if (n >= 5)
            return 1;
    }
    return 0;
}

// 整页重绘：标题栏+状态+棋盘线+棋子，over:是否结束，turn:获胜方
static void Gomoku_Show(int over, int turn)
{
    int i, j;
    char buf[24];
    lcd->draw_rect(0, 0, 800, 480, COL_BG);
    Page_Head("五子棋");
    if (over)
        sprintf(buf, "%s获胜! 点按重开", turn == 1 ? "黑方" : "白方");
    else
        sprintf(buf, "%s落子", turn == 1 ? "黑方" : "白方");
    Lcd_DrawText(lcd, f, 580, 9, 200, 34, TXT_BAR, 10, 0, buf, 0xffffffff, 22);
    // 棋盘线
    for (i = 0; i < B_SIZE; i++)
    {
        draw_line_c(B_X0, B_Y0 + i * CELL, B_X0 + (B_SIZE - 1) * CELL, B_Y0 + i * CELL, COL_LINE);
        draw_line_c(B_X0 + i * CELL, B_Y0, B_X0 + i * CELL, B_Y0 + (B_SIZE - 1) * CELL, COL_LINE);
    }
    // 棋子
    for (i = 0; i < B_SIZE; i++)
        for (j = 0; j < B_SIZE; j++)
        {
            if (board[i][j] == 1)
                Show_Any_Bmp(STONE_B, STONE_S, STONE_S, B_X0 + j * CELL - 12, B_Y0 + i * CELL - 12);
            else if (board[i][j] == 2)
                Show_Any_Bmp(STONE_W, STONE_S, STONE_S, B_X0 + j * CELL - 12, B_Y0 + i * CELL - 12);
        }
    // 胜利时屏幕正中弹出横幅（turn 就是获胜方：赢的时候不换手）
    if (over)
        Show_Any_Bmp(turn == 1 ? WIN_B : WIN_IMG, WIN_W, WIN_H, 200, 180);
}

// 五子棋：双人轮流落子，连成5子获胜
void Gomoku_UI(void)
{
    int x0, y0, x1, y1;
    int turn = 1; // 1黑先手 2白
    int over = 0;
    log_info("进入五子棋");
    memset(board, 0, sizeof(board));
    Gomoku_Show(over, turn);
    while (1)
    {
        get_slide(&x0, &y0, &x1, &y1);
        int dx = x1 - x0, dy = y1 - y0;
        if (dx * dx + dy * dy >= 200) // 滑动不响应
            continue;
        if (Hit_Back(x0, y0))
            return;
        // 结束后点棋盘任意处重新开局
        if (over)
        {
            memset(board, 0, sizeof(board));
            turn = 1;
            over = 0;
            Gomoku_Show(over, turn);
            continue;
        }
        // 算出离按下位置最近的交叉点（按下点判定）
        int c = (x0 - B_X0 + CELL / 2) / CELL;
        int r = (y0 - B_Y0 + CELL / 2) / CELL;
        if (r < 0 || r >= B_SIZE || c < 0 || c >= B_SIZE || board[r][c] != 0)
            continue;
        board[r][c] = turn;
        if (Check_Win(r, c, turn))
        {
            over = 1;
            Gomoku_Show(over, turn);
        }
        else
        {
            // 轮换黑棋和白棋
            turn = 3 - turn;
            Gomoku_Show(over, turn);
        }
    }
}

// 贪吃蛇
#define S_CELL 20 // 格子大小
#define S_COLS 40 // 横向40格正好800宽
#define S_ROWS 21 // 纵向21格，从y50到y470
#define S_Y0 50
// 方向编码：0上 1下 2左 3右（上下、左右互为相反，正好异或1）
static const int DX[4] = {0, 0, -1, 1};
static const int DY[4] = {-1, 1, 0, 0};

static int g_sx[900], g_sy[900];        // 蛇身坐标，0是蛇头
static int g_len, g_dir, g_score, g_ms; // 长度、方向、得分、步进间隔
static int g_fx, g_fy;                  // 食物坐标
static volatile int g_over, g_pause, g_exit, g_restart;
static volatile int g_pend_dir = -1; // 触摸线程写，主循环步进时应用

// 判断某格是否被蛇占着,为了不把食物放在蛇身上
static int Hit_Snake(int x, int y)
{
    int i;
    for (i = 0; i < g_len; i++)
        if (g_sx[i] == x && g_sy[i] == y)
            return 1;
    return 0;
}

// 重开一局
static void Snake_Reset(void)
{
    g_len = 3;
    g_sx[0] = 10;
    g_sy[0] = 10;
    g_sx[1] = 9;
    g_sy[1] = 10;
    g_sx[2] = 8;
    g_sy[2] = 10;
    g_dir = 3; // 初始向右
    g_pend_dir = -1;
    g_score = 0;
    g_ms = 280; // 初始步进间隔，越吃越快
    g_over = 0;
    g_pause = 0;
    g_restart = 0;
    do
    {
        g_fx = rand() % S_COLS;
        g_fy = rand() % S_ROWS;
    } while (Hit_Snake(g_fx, g_fy));
}

// 绘制游戏区：边框+食物+蛇+得分+提示
static void Snake_Draw(void)
{
    char buf[32];
    int i;
    lcd->draw_rect(0, S_Y0, 800, 480 - S_Y0, COL_BG);
    // 四条边框
    draw_line_c(0, S_Y0, 799, S_Y0, COL_LINE);
    draw_line_c(0, 479, 799, 479, COL_LINE);
    draw_line_c(0, S_Y0, 0, 479, COL_LINE);
    draw_line_c(799, S_Y0, 799, 479, COL_LINE);
    // 食物
    draw_fill_circle(g_fx * S_CELL + 10, S_Y0 + g_fy * S_CELL + 10, 8, 0x00F44336);
    // 蛇身：从尾往头画，蛇头颜色最亮
    for (i = g_len - 1; i >= 0; i--)
    {
        int cx = g_sx[i] * S_CELL, cy = S_Y0 + g_sy[i] * S_CELL;
        lcd->draw_rect(cx + 1, cy + 1, S_CELL - 2, S_CELL - 2, i == 0 ? 0x0081C783 : 0x004CAF50);
    }
    // 得分画在标题栏右侧
    sprintf(buf, "得分:%d", g_score);
    Lcd_DrawText(lcd, f, 600, 9, 160, 34, TXT_BAR, 20, 0, buf, 0xffffffff, 22);
    // 如果处于暂停、游戏没结束，显示暂停图片
    if (g_pause && !g_over)
        Show_Any_Bmp(PAUSE_IMG, 400, 120, 200, 180);
    //游戏结束
    if (g_over)
    {
        Show_Any_Bmp(GAMEOVER_IMG, 400, 140, 200, 170);
        // 得分画在横幅正中间：画布收窄到160宽，再按得分数位把文字水平居中
        sprintf(buf, "得分:%d", g_score);
        int digits = 1, s = g_score;
        while (s >= 10)
        {
            s /= 10;
            digits++;
        }
        int fw = 58 + 13 * digits; // "得分:"约58px，每位数字约13px
        Lcd_DrawText(lcd, f, 320, 224, 160, 34, 0x181C2600, (160 - fw) / 2, 0, buf, 0xE4C85A00, 24);
    }
}

// 走一步：应用方向→移动→判定→重绘
static void Snake_Step(void)
{
    int i, eat;
    // 如果新方向不是当前方向的反方向，才允许切换（0/1、2/3互为相反）
    if (g_pend_dir >= 0 && (g_pend_dir ^ g_dir) != 1)
        g_dir = g_pend_dir;
    g_pend_dir = -1;
    int nx = g_sx[0] + DX[g_dir], ny = g_sy[0] + DY[g_dir];
    // 撞墙判断
    if (nx < 0 || nx >= S_COLS || ny < 0 || ny >= S_ROWS)
    {
        g_over = 1;
        Snake_Draw();
        return;
    }
    eat = (nx == g_fx && ny == g_fy);
    // 咬到自己（不吃时尾巴会挪走，尾巴那格不算）
    for (i = 0; i < g_len; i++)
    {
        if (i == g_len - 1 && !eat)
            continue;
        if (g_sx[i] == nx && g_sy[i] == ny)
        {
            g_over = 1;
            Snake_Draw();
            return;
        }
    }
    // 身体整体后移一格；吃到食物则长度+1
    int end = eat ? g_len : g_len - 1;
    for (i = end; i > 0; i--)
    {
        g_sx[i] = g_sx[i - 1];
        g_sy[i] = g_sy[i - 1];
    }
    if (eat)
    {
        g_len++;
        g_score++;
        if (g_ms > 120)
            g_ms -= 8;
        do
        {
            g_fx = rand() % S_COLS;
            g_fy = rand() % S_ROWS;
        } while (Hit_Snake(g_fx, g_fy));
    }
    g_sx[0] = nx;
    g_sy[0] = ny;
    Snake_Draw();
}

// 触摸线程：后台一直收滑动改方向，点按暂停/重开，点返回键退出
static void *Snake_Touch(void *arg)
{
    int x0, y0, x1, y1;
    while (1)
    {
        get_slide(&x0, &y0, &x1, &y1);
        int dx = x1 - x0, dy = y1 - y0;
        if (dx * dx + dy * dy < 200) // 点按（按下点判定）
        {
            if (Hit_Back(x0, y0))
            {
                g_exit = 1;
                break;
            }
            if (g_over)
                g_restart = 1;
            else
                g_pause = !g_pause;
        }
        else // 滑动：按主方向变向
        {
            if (abs(dx) > abs(dy))
                g_pend_dir = dx > 0 ? 3 : 2; // 左右
            else
                g_pend_dir = dy > 0 ? 1 : 0; // 上下
        }
    }
    return NULL;
}

// 贪吃蛇：滑动控制方向，吃食物变长加速
void Snake_UI(void)
{
    pthread_t tid;
    int acc = 0, prev_over = 0, prev_pause = 0;
    g_exit = 0;
    log_info("进入贪吃蛇");
    srand(time(NULL));
    Snake_Reset();
    lcd->draw_rect(0, 0, 800, 480, COL_BG);
    Page_Head("贪吃蛇");
    Snake_Draw();
    pthread_create(&tid, NULL, Snake_Touch, NULL);
    // 主循环每10ms走一格计时，触摸全部交给线程
    while (!g_exit)
    {
        // 重新开始
        if (g_restart)
        {
            Snake_Reset();
            Snake_Draw();
            prev_over = 0;
            prev_pause = 0;
            acc = 0;
        }
        else
        {
            if (!g_over && !g_pause)
            {
                acc += 10;
                if (acc >= g_ms)
                {
                    acc = 0;
                    Snake_Step();
                }
            }
            // 暂停/结束状态切换时重绘一次提示
            if (g_over != prev_over || g_pause != prev_pause)
                Snake_Draw();
        }
        prev_over = g_over;
        prev_pause = g_pause;
        usleep(10000);
    }
    pthread_cancel(tid);
    pthread_join(tid, NULL);
}
