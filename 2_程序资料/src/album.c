#include "../inc/album.h"
#include "../inc/bmp.h"
#include "../inc/touch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

// 相册目录（photo里 full_xx.bmp=大图 thumb_xx.bmp=缩略图，成对存在）
#define PHOTO_DIR "/root/car/res/system/photo"
#define MAX_PHOTOS 64

// 网格布局
#define THUMB_W 236
#define THUMB_H 172
#define GRID_X0 23 // 第一列X
#define GRID_Y0 58 // 第一行Y（标题栏下面）
#define COL_GAP 23
#define ROW_GAP 20
#define VIS_ROW 2 // 一屏可见行数
#define BAR_H 50  // 标题栏高

// 大图页按钮点击区（电阻屏边缘点不到，热区往里多留一些）
#define BTN_BACK_L 0  // 返回按钮区 x0~90 y0~60
#define BTN_BACK_R 90
#define BTN_DEL_L 160 // 删除按钮区（底部条上）
#define BTN_DEL_R 320
#define BTN_BG_L 460 // 设为背景按钮区
#define BTN_BG_R 660
#define ARROW_Y 170   // 左右箭头Y（高96）
#define ARROW_L_R 90  // 左箭头点击区右边界
#define ARROW_R_L 710 // 右箭头点击区左边界

static char g_full[MAX_PHOTOS][128]; // 大图文件路径列表
static int g_count = 0;

// 扫描相册目录：收集 full_*.bmp 按文件名排序
static int Cmp_Str(const void *a, const void *b)
{
    return strcmp((char *)a, (char *)b);
}
// 文件名 full_ 后面的编号（去前导0），用于两套命名去重
static void Num_Key(const char *name, char *key, int size)
{
    const char *p = name + 5; // 跳过"full_"
    while (*p == '0')
        p++;
    snprintf(key, size, "%s", p);
}
// 获取目录下所有的图片
static void Scan_Photos(void)
{
    DIR *dp;
    struct dirent *e;
    g_count = 0;
    dp = opendir(PHOTO_DIR);
    if (dp == NULL)
        return;
    while ((e = readdir(dp)) != NULL)
    {
        if (strncmp(e->d_name, "full_", 5) == 0 && g_count < MAX_PHOTOS)
        {
            // 编号去重：full_1和full_01算同一张，防止两套命名混传出现重复
            char key[32], other[32];
            int dup = 0, i;
            Num_Key(e->d_name, key, sizeof(key));
            for (i = 0; i < g_count; i++)
            {
                Num_Key(strrchr(g_full[i], '/') + 1, other, sizeof(other));
                if (strcmp(key, other) == 0)
                {
                    dup = 1;
                    break;
                }
            }
            if (dup)
                continue;
            snprintf(g_full[g_count], 128, "%s/%s", PHOTO_DIR, e->d_name);
            g_count++;
        }
    }
    closedir(dp);
    qsort(g_full, g_count, 128, Cmp_Str);
}

// 由大图路径得到缩略图路径（兼容补零/不补零两套命名：找不到自动换一种拼法）
static void Thumb_Path(const char *full, char *out)
{
    char *p;
    strcpy(out, full);
    p = strstr(out, "/full_");
    if (p == NULL)
        return;
    memcpy(p, "/thumb_", 7);
    // 推导名不存在时，尝试另一种编号拼法（thumb_01 <-> thumb_1）
    if (access(out, F_OK) != 0)
    {
        char alt[128];
        char *q = strstr(out, "thumb_");
        const char *num = q + 6;
        if (num[0] == '0')
            snprintf(alt, sizeof(alt), "%.*sthumb_%s", (int)(q - out), out, num + 1);
        else
            snprintf(alt, sizeof(alt), "%.*sthumb_0%s", (int)(q - out), out, num);
        if (access(alt, F_OK) == 0)
            strcpy(out, alt);
    }
}

// 文件复制（设为背景就是把大图复制成 main_bg.bmp）
static void Copy_File(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb");
    FILE *out = fopen(dst, "wb");
    char buf[4096];
    int n;
    if (in == NULL || out == NULL)
    {
        if (in)
            fclose(in);
        if (out)
            fclose(out);
        return;
    }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
}

// 显示相册网格页
static void Album_Show(int scroll)
{
    int i;
    // 整屏深色底 + 标题栏（fb颜色格式0x00BBGGRR：RGB(20,22,30)=0x1E161400）
    lcd->draw_rect(0, 0, 800, 480, 0x1E161400);
    lcd->draw_rect(0, 0, 800, BAR_H, 0x261A1600);
    // 标题"相册"
    Lcd_DrawText(lcd, f, 340, 9, 120, 34, 0x261A1600, 0, 0, "相册", 0xffffffff, 24);
    // 左上角返回按钮
    Show_Any_Bmp(PHOTO_DIR "/ui_back.bmp", 72, 50, 0, 0);
    // 缩略图网格:一页两行，一行3张图片
    for (i = 0; i < VIS_ROW * 3; i++)
    {
        int idx = scroll * 3 + i;
        int row = i / 3, col = i % 3;
        char t[128];
        if (idx >= g_count)
            break;
        // 大图路径改成小图路径
        Thumb_Path(g_full[idx], t);
        // 显示小图片
        Show_Any_Bmp(t, THUMB_W, THUMB_H,
                     GRID_X0 + col * (THUMB_W + COL_GAP),
                     GRID_Y0 + row * (THUMB_H + ROW_GAP));
    }
}

// 删除确认框（和主界面登出/关机确认同款 420x180，弹屏幕中心 190,150）
// 图内按钮："取消" x 236~386、"确认" x 414~564，y 246~308；点框外也算取消
// 返回1=确认删除 0=取消
static int Del_Confirm(void)
{
    int x0, y0, x1, y1;
    Show_Any_Bmp(PHOTO_DIR "/ui_confirm_del.bmp", 420, 180, 190, 150);
    while (1)
    {
        get_slide(&x0, &y0, &x1, &y1);
        // 按钮判定用按下点(x0,y0)：快速点按时抬起点可能失真
        if (x0 >= 414 && x0 <= 564 && y0 >= 246 && y0 <= 308)
            return 1;
        if (x0 >= 236 && x0 <= 386 && y0 >= 246 && y0 <= 308)
            return 0;
        if (!(x0 >= 190 && x0 <= 610 && y0 >= 150 && y0 <= 330))
            return 0;
    }
}

// 大图查看页：左右切换、删除、设为背景；返回0=正常回网格，删除后也回网格
static void Photo_View(int idx)
{
    int x0, y0, x1, y1;
    while (1)
    {
        // 显示大图
        Show_Any_Bmp(g_full[idx], 800, 480, 0, 0);
        Show_Any_Bmp(PHOTO_DIR "/ui_arrow_l.bmp", 48, 96, 6, ARROW_Y);
        Show_Any_Bmp(PHOTO_DIR "/ui_arrow_r.bmp", 48, 96, 746, ARROW_Y);
        Show_Any_Bmp(PHOTO_DIR "/ui_bar.bmp", 800, 60, 0, 420);
        Show_Any_Bmp(PHOTO_DIR "/ui_back.bmp", 72, 50, 0, 0);
        get_slide(&x0, &y0, &x1, &y1);
        int dx = x1 - x0;
        // 点按判定用按下点(x0,y0)：抬起点在快速点按时可能失真
        // 左上角返回（热区加宽到 x<90 y<60）
        if (x0 < BTN_BACK_R && y0 < 60)
            return;
        // 左右滑动切换（像手机一样，全屏任意位置滑）
        if (dx < -60)
            idx = (idx + 1) % g_count;
        else if (dx > 60)
            idx = (idx - 1 + g_count) % g_count;
        // 点左箭头：上一张
        else if (x0 < ARROW_L_R && y0 >= ARROW_Y && y0 < ARROW_Y + 96)
            idx = (idx - 1 + g_count) % g_count;
        // 点右箭头：下一张
        else if (x0 > ARROW_R_L && y0 >= ARROW_Y && y0 < ARROW_Y + 96)
            idx = (idx + 1) % g_count;
        // 底部按钮条
        else if (y0 >= 420)
        {
            if (x0 >= BTN_DEL_L && x0 <= BTN_DEL_R) // 删除（先弹确认框）
            {
                if (Del_Confirm())
                {
                    char t[128];
                    Thumb_Path(g_full[idx], t);
                    // 大图小图都删除
                    remove(g_full[idx]);
                    remove(t);
                    // 更新存放大图的数组
                    Scan_Photos();
                    return;
                }
                // 取消则继续看图，循环顶部会重绘大图页
            }
            else if (x0 >= BTN_BG_L && x0 <= BTN_BG_R) // 设为背景
            {
                Copy_File(g_full[idx], "/root/car/res/system/main/main_bg.bmp");
                Show_Any_Bmp(PHOTO_DIR "/ui_toast.bmp", 400, 64, 200, 150);
                sleep(1);
            }
        }
    }
}

// 相册主界面：网格浏览，上下滑动翻页，点缩略图看大图，左上角返回
void Album_UI(void)
{
    int scroll = 0;
    int x0, y0, x1, y1;
    // 获取所有图片
    Scan_Photos();
    if (g_count == 0)
        return;
    while (1)
    {
        int dx, dy, max_scroll;
        Album_Show(scroll);
        get_slide(&x0, &y0, &x1, &y1);
        dx = x1 - x0;
        dy = y1 - y0;
        max_scroll = (g_count + 2) / 3 - 1; // 能翻到的最大行
        if (max_scroll < 0)
            max_scroll = 0;
        // 上滑：看下一行
        if (dy < -60 && (dx < 0 ? -dx : dx) < -dy)
        {
            if (scroll < max_scroll)
                scroll++;
        }
        // 下滑：回上一行
        else if (dy > 60 && (dx < 0 ? -dx : dx) < dy)
        {
            if (scroll > 0)
                scroll--;
        }
        // 左上角返回主界面（热区加宽到 x<90 y<60）
        else if (x0 < 90 && y0 < 60)
        {
            return;
        }
        // 点中缩略图：进入大图（用按下点判定）
        else if (y0 >= GRID_Y0)
        {
            int col = (x0 - GRID_X0) / (THUMB_W + COL_GAP);
            int row = (y0 - GRID_Y0) / (THUMB_H + ROW_GAP);
            int tx = (x0 - GRID_X0) % (THUMB_W + COL_GAP);
            int ty = (y0 - GRID_Y0) % (THUMB_H + ROW_GAP);
            if (col >= 0 && col < 3 && row >= 0 && row < VIS_ROW &&
                tx < THUMB_W && ty < THUMB_H)
            {
                int idx = scroll * 3 + row * 3 + col;
                if (idx < g_count)
                {
                    Photo_View(idx);
                    Scan_Photos(); // 大图里可能删了图，重新扫
                    if (g_count == 0)
                        return; // 图全删完了，退回主界面
                    if (scroll > (g_count + 2) / 3 - 1)
                        scroll = (g_count + 2) / 3 - 1;
                    if (scroll < 0)
                        scroll = 0;
                }
            }
        }
    }
}
