// 登录注册 + 屏幕键盘：登录/注册流程和软键盘输入都在这里
#include "../inc/login.h"
#include "../log/log.h"
#include "../inc/bmp.h"
#include "../inc/touch.h"

// ==================== 用户注册/登录验证 ====================

// 注册新用户：0成功，-1用户名已存在，-2文件打开失败
int User_Register(const char *name, const char *pwd)
{
    char n[32], p[32];

    // 先检查用户名是否已存在
    FILE *fp = fopen(USER_FILE, "r");
    if (fp != NULL)
    {
        while (fscanf(fp, "%31s %31s", n, p) == 2)
        {
            if (strcmp(n, name) == 0)
            {
                printf("用户名已存在\n");
                fclose(fp);
                return -1;
            }
        }
        fclose(fp);
    }

    // 追加写入新用户
    fp = fopen(USER_FILE, "a");
    if (fp == NULL)
        return -2;
    fprintf(fp, "%s %s\n", name, pwd);
    fclose(fp);
    return 0;
}

// 登录验证：0成功，-1用户名或密码错误，-2文件打开失败
int User_Login(const char *name, const char *pwd)
{
    char n[32], p[32];

    FILE *fp = fopen(USER_FILE, "r");
    if (fp == NULL)
        return -2;

    while (fscanf(fp, "%31s %31s", n, p) == 2)
    {
        if (strcmp(n, name) == 0 && strcmp(p, pwd) == 0)
        {
            printf("登录成功\n");
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return -1;
}

// 显示提示条
static void Toast_Show(const char *img)
{
    Show_Any_Bmp(img, 400, 64, 200, 20);
    usleep(1500000);
}

// 把输入内容画进输入框
static void Draw_Input(char *buf, int len, int bx, int by)
{
    if (len <= 0)
        return;
    char disp[32] = {0};
    memcpy(disp, buf, len);
    Lcd_DrawText(lcd, f, bx, by, 164, 22, 0xffffffff, 2, 2, disp, 0x000000, 22);
}

// 注册页
static int Page_Register(char *out_name, char *out_pwd)
{
    char name[32] = {0}, pwd[32] = {0};
    int name_len = 0, pwd_len = 0;
    int ret;
    int reg_ok = 0; // 是否已注册成功
    int tx, ty;

    while (1)
    {
        // 显示注册界面
        Show_Bmp("/root/car/res/start/register.bmp");
        Draw_Input(name, name_len, 322, 168);
        Draw_Input(pwd, pwd_len, 322, 212);
        get_xy(&tx, &ty);
        // 点击用户名框（与登录页相同：x315~492 y163~197）
        if (tx >= 315 && tx <= 492 && ty >= 163 && ty <= 197)
        {
            Keyboard_Input(name, &name_len, 31, 322, 168, 164, 22, 1);
        }
        // 点击密码框（y208~241）
        else if (tx >= 315 && tx <= 492 && ty >= 208 && ty <= 241)
        {
            Keyboard_Input(pwd, &pwd_len, 31, 322, 212, 164, 22, 1);
        }
        // 点击注册按钮
        else if (tx >= 315 && tx <= 492 && ty >= 253 && ty <= 285)
        {
            if (name_len == 0 || pwd_len == 0)
            {
                Toast_Show(TOAST_EMPTY);
                continue;
            }
            // 验证注册的用户名
            ret = User_Register(name, pwd);
            if (ret == 0)
            {
                // 注册成功：只弹提示留在注册页，账号密码记下来等返回登录时带回
                Toast_Show(TOAST_REG_OK);
                log_info("注册成功:%s", name);
                strcpy(out_name, name);
                strcpy(out_pwd, pwd);
                reg_ok = 1;
                continue;
            }
            else if (ret == -1)
            {
                Toast_Show(TOAST_EXIST);
                log_warn("注册失败,用户名已存在:%s", name);
            }
            else
            {
                Toast_Show(TOAST_FILE);
            }
        }
        // 点击返回登录（同登录页"创建账号"位置：y318~345）
        // 注册成功过：账号密码带回登录页；没注册：清空登录框
        else if (tx >= 360 && tx <= 445 && ty >= 318 && ty <= 345)
        {
            return reg_ok;
        }
    }
}

// 登录注册：点击输入框弹键盘输入，点登录按钮验证，点创建账号进注册页
void Login_Register(void)
{
    char name[32] = {0}, pwd[32] = {0};
    int name_len = 0, pwd_len = 0;
    int ret;
    int tx, ty;

    while (1)
    {
        // 显示登录界面
        Show_Bmp("/root/car/res/start/login.bmp");
        // 已输入的内容显示到输入框
        Draw_Input(name, name_len, 322, 168);
        Draw_Input(pwd, pwd_len, 322, 212);
        get_xy(&tx, &ty);
        // 点击用户名框,显示键盘
        if (tx >= 315 && tx <= 492 && ty >= 163 && ty <= 197)
        {
            Keyboard_Input(name, &name_len, 31, 322, 168, 164, 22, 1);
        }
        // 点击密码框
        else if (tx >= 315 && tx <= 492 && ty >= 208 && ty <= 241)
        {
            Keyboard_Input(pwd, &pwd_len, 31, 322, 212, 164, 22, 1);
        }
        // 点击登录按钮
        else if (tx >= 315 && tx <= 492 && ty >= 253 && ty <= 285)
        {
            if (name_len == 0 || pwd_len == 0)
            {
                Toast_Show(TOAST_EMPTY);
                continue;
            }
            // 验证登录信息
            ret = User_Login(name, pwd);
            // 登录成功
            if (ret == 0)
            {
                Toast_Show(TOAST_OK);
                log_info("登录成功:%s", name);
                return;
            }
            // 打开文件错误
            if (ret == -2)
            {
                Toast_Show(TOAST_FILE);
                continue;
            }
            // 用户名和密码错误
            log_warn("登录失败,账号或密码错误:%s", name);
            Toast_Show(TOAST_PWD_ERR);
        }
        // 点击创建账号,进入创建页面
        else if (tx >= 360 && tx <= 445 && ty >= 318 && ty <= 345)
        {
            // 注册成功返回1：注册的账号密码已填进登录框，直接点登录即可
            if (Page_Register(name, pwd) == 0)
            {
                // 点了"返回登录"：登录框清空
                name[0] = 0;
                name_len = 0;
                pwd[0] = 0;
                pwd_len = 0;
            }
        }
    }
}

// ==================== 屏幕键盘 ====================

static const char *kb_img[4] =
    {
        "/root/car/res/start/keyboard_lowercase.bmp",
        "/root/car/res/start/keyboard_capital.bmp",
        "/root/car/res/start/keyboard_number.bmp",
        "/root/car/res/start/keyboard_symbol.bmp",
};
static const char *kb_black[4] =
    {
        "/root/car/res/start/keyboard_lowercase_black.bmp",
        "/root/car/res/start/keyboard_capital_black.bmp",
        "/root/car/res/start/keyboard_number_black.bmp",
        "/root/car/res/start/keyboard_symbol_black.bmp",
};

// 一个按键：在键盘图内的位置、大小、对应的字符
struct Key
{
    int x;   // 按键左上角X（键盘图内）
    int y;   // 按键左上角Y
    int w;   // 按键宽
    int h;   // 按键高
    char ch; // 按下的字符
};

// 小写键盘（坐标是从图片里逐像素量出来的）
static struct Key kb_lower[] =
    {
        // 右上角收起箭头
        {600, 0, 80, 40, KB_EXIT},
        // 第一行 q~p
        {129, 40, 51, 46, 'q'},
        {183, 40, 52, 46, 'w'},
        {238, 40, 51, 45, 'e'},
        {292, 40, 51, 45, 'r'},
        {346, 40, 51, 45, 't'},
        {400, 40, 52, 45, 'y'},
        {455, 40, 51, 45, 'u'},
        {509, 40, 51, 45, 'i'},
        {563, 40, 51, 45, 'o'},
        {617, 40, 52, 45, 'p'},
        // 第二行 a~l
        {156, 90, 52, 45, 'a'},
        {211, 90, 51, 45, 's'},
        {265, 90, 51, 45, 'd'},
        {319, 90, 51, 45, 'f'},
        {373, 90, 52, 45, 'g'},
        {428, 90, 51, 45, 'h'},
        {482, 90, 51, 45, 'j'},
        {536, 90, 51, 45, 'k'},
        {590, 90, 52, 46, 'l'},
        // 第三行 shift + z~m + 退格
        {129, 139, 69, 45, '\t'},
        {211, 139, 51, 45, 'z'},
        {265, 139, 51, 45, 'x'},
        {319, 139, 52, 46, 'c'},
        {374, 139, 51, 46, 'v'},
        {428, 139, 51, 46, 'b'},
        {482, 139, 51, 46, 'n'},
        {536, 139, 52, 48, 'm'},
        {600, 139, 70, 45, '\b'},
        // 第四行 !?# + 123 + 逗号 + 空格 + 句号 + 回车
        {129, 188, 79, 46, KB_SYMBOL},
        {210, 188, 52, 46, KB_NUMBER},
        {265, 188, 52, 46, ','},
        {319, 188, 160, 46, ' '},
        {482, 188, 52, 46, '.'},
        {591, 188, 78, 46, '\n'},
};

// 大写键盘（布局和小写完全一样）
static struct Key kb_capital[] =
    {
        {600, 0, 80, 40, KB_EXIT},
        {129, 40, 51, 46, 'Q'},
        {183, 40, 52, 46, 'W'},
        {238, 40, 51, 45, 'E'},
        {292, 40, 51, 45, 'R'},
        {346, 40, 51, 45, 'T'},
        {400, 40, 52, 46, 'Y'},
        {455, 40, 51, 45, 'U'},
        {509, 40, 51, 45, 'I'},
        {563, 40, 51, 45, 'O'},
        {617, 40, 52, 45, 'P'},
        {156, 90, 52, 45, 'A'},
        {211, 90, 51, 45, 'S'},
        {265, 90, 51, 45, 'D'},
        {319, 90, 51, 45, 'F'},
        {373, 90, 52, 45, 'G'},
        {428, 90, 51, 45, 'H'},
        {482, 90, 51, 45, 'J'},
        {536, 90, 51, 45, 'K'},
        {590, 90, 52, 46, 'L'},
        {129, 139, 69, 45, '\t'},
        {211, 139, 51, 45, 'Z'},
        {265, 139, 51, 45, 'X'},
        {319, 139, 52, 46, 'C'},
        {374, 139, 51, 46, 'V'},
        {428, 139, 51, 46, 'B'},
        {482, 139, 51, 46, 'N'},
        {536, 139, 52, 48, 'M'},
        {600, 139, 70, 45, '\b'},
        {129, 188, 79, 46, KB_SYMBOL},
        {210, 188, 52, 46, KB_NUMBER},
        {265, 188, 52, 46, ','},
        {319, 188, 160, 46, ' '},
        {482, 188, 52, 46, '.'},
        {591, 188, 78, 46, '\n'},
};

// 数字键盘（4行5列）
static struct Key kb_number[] =
    {
        {600, 0, 80, 40, KB_EXIT}, // 右上角收起箭头（原来漏了，导致数字键盘点收起没反应）
        {129, 40, 93, 46, '/'},
        {225, 40, 115, 46, '1'},
        {342, 40, 115, 46, '2'},
        {460, 40, 115, 46, '3'},
        {577, 40, 93, 46, '\b'},
        {129, 89, 93, 46, ':'},
        {225, 89, 115, 46, '4'},
        {342, 89, 115, 46, '5'},
        {460, 89, 115, 46, '6'},
        {577, 89, 93, 46, '.'},
        {129, 138, 94, 46, '-'},
        {225, 138, 115, 46, '7'},
        {342, 138, 115, 46, '8'},
        {460, 138, 116, 46, '9'},
        {577, 138, 93, 46, '@'},
        {129, 187, 94, 47, KB_SYMBOL},
        {224, 187, 116, 47, KB_ABC},
        {342, 187, 115, 47, '0'},
        {459, 187, 116, 47, ' '},
        {577, 187, 93, 47, '\n'},
};

// 符号键盘（第三行最左边是空白键，没有功能，不放进表里）
static struct Key kb_symbol[] =
    {
        {600, 0, 80, 40, KB_EXIT},
        {129, 40, 51, 46, '1'},
        {183, 40, 52, 46, '2'},
        {238, 40, 51, 45, '3'},
        {292, 40, 51, 45, '4'},
        {346, 40, 51, 45, '5'},
        {400, 40, 52, 46, '6'},
        {455, 40, 51, 45, '7'},
        {509, 40, 51, 45, '8'},
        {563, 40, 51, 45, '9'},
        {617, 40, 52, 46, '0'},
        {129, 90, 51, 45, '@'},
        {183, 90, 52, 46, '#'},
        {238, 90, 51, 45, '$'},
        {292, 90, 51, 45, '_'},
        {346, 90, 51, 45, '&'},
        {400, 90, 52, 45, '-'},
        {455, 90, 51, 45, '+'},
        {509, 90, 51, 45, '('},
        {563, 90, 51, 45, ')'},
        {617, 90, 52, 46, '/'},
        {211, 139, 51, 45, '*'},
        {265, 139, 51, 45, '\"'},
        {319, 139, 52, 46, '\''},
        {374, 139, 51, 46, ':'},
        {428, 139, 51, 46, ';'},
        {482, 139, 51, 46, '!'},
        {536, 139, 52, 48, '?'},
        {600, 139, 70, 45, '\b'},
        {129, 188, 79, 46, KB_ABC},
        {210, 188, 52, 46, KB_NUMBER},
        {265, 188, 52, 46, ','},
        {319, 188, 215, 46, ' '},
        {537, 188, 53, 46, '.'},
        {591, 188, 79, 46, '\n'},
};

static struct Key *kb_page[4] = {kb_lower, kb_capital, kb_number, kb_symbol};
static int kb_len[4] =
    {
        sizeof(kb_lower) / sizeof(kb_lower[0]),
        sizeof(kb_capital) / sizeof(kb_capital[0]),
        sizeof(kb_number) / sizeof(kb_number[0]),
        sizeof(kb_symbol) / sizeof(kb_symbol[0]),
};

// 显示某个键盘页面
void Keyboard_Show(int page)
{
    Show_Any_Bmp(kb_img[page], KB_W, KB_H, KB_X, KB_Y);
}

// 根据触摸坐标返回字符，没点中返回0；点中后从变暗图抠出该按键贴上去
char Keyboard_GetChar(int page, int tx, int ty)
{
    int i;
    struct Key *ks = kb_page[page];
    // 坐标换算到键盘图内
    int kx = tx - KB_X;
    int ky = ty - KB_Y;
    // 先精确匹配：手指落在按键真实区域内就直接命中
    for (i = 0; i < kb_len[page]; i++)
    {
        if (kx >= ks[i].x && kx < ks[i].x + ks[i].w &&
            ky >= ks[i].y && ky < ks[i].y + ks[i].h)
        {
            // 贴变暗图的对应小块，按键变灰
            Show_Part_Bmp(kb_black[page], KB_W, KB_H,
                          KB_X + ks[i].x, KB_Y + ks[i].y,
                          ks[i].x, ks[i].y, ks[i].w, ks[i].h);
            return ks[i].ch;
        }
    }
    // 兜底：扩大热区里挑中心最近的键（触摸屏偏一点也能中，比如按"123"偏到隔壁）
    int best = -1;
    long bestd = 1 << 30;
    for (i = 0; i < kb_len[page]; i++)
    {
        if (kx >= ks[i].x - KEY_PAD && kx < ks[i].x + ks[i].w + KEY_PAD &&
            ky >= ks[i].y - KEY_PAD && ky < ks[i].y + ks[i].h + KEY_PAD)
        {
            long dx = kx - (ks[i].x + ks[i].w / 2);
            long dy = ky - (ks[i].y + ks[i].h / 2);
            long d = dx * dx + dy * dy;
            if (d < bestd)
            {
                bestd = d;
                best = i;
            }
        }
    }
    if (best >= 0)
    {
        Show_Part_Bmp(kb_black[page], KB_W, KB_H,
                      KB_X + ks[best].x, KB_Y + ks[best].y,
                      ks[best].x, ks[best].y, ks[best].w, ks[best].h);
        return ks[best].ch;
    }
    return 0;
}

// 把当前输入内容画到输入框里,参数：字符串，长度，写入lcd的横坐标,纵坐标，画布宽，画布高，是否加光标
static void Input_Refresh(char *buf, int len, int bx, int by, int bw, int bh, int cursor)
{
    char disp[34] = {0};
    memcpy(disp, buf, len);
    // 正在输入：末尾加"|"光标（buf满时放不下就不加）
    if (cursor && len < 32)
        disp[len] = '|';
    Lcd_DrawText(lcd, f, bx, by, bw, bh, 0xffffffff, 2, 2, disp, 0x000000, 22);
}

// 键盘输入：字符写入buf，按Enter结束；每按一键实时刷新到输入框
void Keyboard_Input(char *buf, int *len, int max_len, int bx, int by, int bw, int bh, int cursor)
{
    int page = PAGE_LOWER;
    Keyboard_Show(page);
    // 刚弹键盘就先刷一次，空框也显示"|"光标
    Input_Refresh(buf, *len, bx, by, bw, bh, cursor);
    while (1)
    {
        int tx, ty;
        get_xy(&tx, &ty);
        char c = Keyboard_GetChar(page, tx, ty);
        // 兜底：键盘顶部右上角（收起箭头）任何页面都收起，不依赖按键表
        if (ty - KB_Y < 40 && tx - KB_X > 560)
            c = KB_EXIT;
        // 判断按下的是哪个键
        // 回车键
        if (c == KB_EXIT || c == '\n')
        {
            break;
        }
        // 删除键
        else if (c == '\b')
        {
            if (*len > 0)
                buf[--(*len)] = 0;
        }
        // shift键,大小写切换
        else if (c == '\t')
        {
            page = (page == PAGE_LOWER) ? PAGE_CAPITAL : PAGE_LOWER;
        }
        // 特殊符号键
        else if (c == KB_SYMBOL)
        {
            page = PAGE_SYMBOL;
        }
        // 数字键
        else if (c == KB_NUMBER)
        {
            page = PAGE_NUMBER;
        }
        // 小写
        else if (c == KB_ABC)
        {
            page = PAGE_LOWER;
        }
        // 其他键，就获取数据放到buf里
        else if (c != 0 && *len < max_len)
        {
            buf[(*len)++] = c;
        }
        usleep(50000);       // 变暗停留50ms
        Keyboard_Show(page); // 重画正常键盘，按键弹起
        // 实时把输入内容刷新到输入框
        Input_Refresh(buf, *len, bx, by, bw, bh, cursor);
    }
}
