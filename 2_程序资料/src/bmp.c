#include "../inc/bmp.h"
#include "../inc/touch.h"
#include "../inc/lcddev.h"

//显示图片，指定文件名
int Show_Bmp(const char* filename)
{
    //打开图片
    int bmp_fd=open(filename,O_RDONLY);
    if(bmp_fd==-1)
    {
        perror(filename);
        return -1;
    }

    unsigned char bmp_buf[800*480*3]={0};
    //跳过54字节格式头
    lseek(bmp_fd,54,SEEK_SET);
    //读取像素数据
    int n=read(bmp_fd,bmp_buf,sizeof(bmp_buf));
    //判断读取的字节数错误
    if(n!=(int)sizeof(bmp_buf))
    {
        printf("read %s short: %d\n",filename,n);
        close(bmp_fd);
        return -1;
    }

    //将3字节的bmp图片RGB转换为4字节的LCD显示的图片ARGB
    int i,j;
    int lcd_buf[800*480]={0};
    for(i=0,j=0;i<800*480;i++,j+=3)
    {
        lcd_buf[i]=bmp_buf[j+0]<<0 | bmp_buf[j+1]<<8 | bmp_buf[j+2]<<16 | 0x00<<24;
    }

    int x,y;
    for(y=0;y<480;y++)
    {
        for(x=0;x<800;x++)
        {
            lcd->mptr[(y)*800+(x)]=lcd_buf[(479-y)*800+x];
        }
    }

    return 0;
}

//显示开机动画
int Show_Open(char *path)
{
    char bmp_path[64]={0};
    for(int i=0;i<92;i++)
    {
        sprintf(bmp_path,"%s/%03d.bmp",path,i);
        if(Show_Bmp(bmp_path)<0)
            break;//某帧读失败就停下，避免花屏
        usleep(100);//一帧0.1ms
    }
}

//在任意位置，显示任意大小的图片
int Show_Any_Bmp(const char* filename,int bmp_w,int bmp_h,int x0,int y0)
{
    //打开图片
    int bmp_fd=open(filename,O_RDONLY);
    if(bmp_fd==-1)
    {
        perror(filename);
        return -1;
    }

    unsigned char bmp_buf[bmp_w*bmp_h*3];
    memset(bmp_buf, 0, bmp_w * bmp_h * 3);
    //跳过54字节格式头
    lseek(bmp_fd,54,SEEK_SET);
    //读取像素数据
    int n=read(bmp_fd,bmp_buf,sizeof(bmp_buf));
    //判断读取的字节数错误
    if(n!=(int)sizeof(bmp_buf))
    {
        printf("read %s short: %d\n",filename,n);
        close(bmp_fd);
        return -1;
    }

    //将3字节的bmp图片RGB转换为4字节的LCD显示的图片ARGB
    int i,j;
    int lcd_buf[bmp_w*bmp_h];
    memset(lcd_buf, 0, bmp_w * bmp_h * 4);
    for(i=0,j=0;i<bmp_w*bmp_h;i++,j+=3)
    {
        lcd_buf[i]=bmp_buf[j+0]<<0 | bmp_buf[j+1]<<8 | bmp_buf[j+2]<<16 | 0x00<<24;
    }

    int x,y;
    for(y=0;y<bmp_h;y++)//只扫图片本身的行数
    {
        int ly=y+y0;
        if(ly<0||ly>=480)//超出屏幕的行不画
            continue;
        for(x=0;x<bmp_w;x++)
        {
            int lx=x+x0;
            if(lx<0||lx>=800)
                continue;
            lcd->mptr[ly*800+lx]=lcd_buf[(bmp_h-1-y)*bmp_w+x];
        }
    }

    return 0;
}

//图标叠加：白色线条图标（黑底）按亮度混合到屏幕上，黑色=全透明，画在任意壁纸上都无方块底
//图片要求：宽为4的倍数（无行填充）
int Show_Icon(const char* filename,int bmp_w,int bmp_h,int x0,int y0)
{
    int bmp_fd=open(filename,O_RDONLY);
    if(bmp_fd==-1)
    {
        perror(filename);
        return -1;
    }

    unsigned char bmp_buf[bmp_w*bmp_h*3];
    memset(bmp_buf, 0, bmp_w * bmp_h * 3);
    lseek(bmp_fd,54,SEEK_SET);
    int n=read(bmp_fd,bmp_buf,bmp_w*bmp_h*3);
    close(bmp_fd);
    if(n!=(int)(bmp_w*bmp_h*3))
        return -1;

    int x,y;
    for(y=0;y<bmp_h;y++)
    {
        int ly=y0+y;
        if(ly<0||ly>=480)
            continue;
        for(x=0;x<bmp_w;x++)
        {
            int lx=x0+x;
            if(lx<0||lx>=800)
                continue;
            int bi=((bmp_h-1-y)*bmp_w+x)*3;   //BMP自下而上存
            int r=bmp_buf[bi+2], g=bmp_buf[bi+1], b=bmp_buf[bi+0];
            int a=r>g?(r>b?r:b):(g>b?g:b);    //亮度当透明度：白线不透明，黑底全透明
            unsigned int *p=&lcd->mptr[ly*800+lx];
            unsigned int dst=*p;
            int dr=(dst>>16)&0xFF, dg=(dst>>8)&0xFF, db=dst&0xFF;
            dr=(dr*(255-a)+255*a)/255;        //往白色上混合
            dg=(dg*(255-a)+255*a)/255;
            db=(db*(255-a)+255*a)/255;
            *p=(dr<<16)|(dg<<8)|db;
        }
    }
    return 0;
}

//显示图片的某一块区域：从图片的(sx,sy)处取sw×sh大小，贴到屏幕(x0,y0)处
int Show_Part_Bmp(const char* filename,int bmp_w,int bmp_h,int x0,int y0,int sx,int sy,int sw,int sh)
{
    //打开图片
    int bmp_fd=open(filename,O_RDONLY);
    if(bmp_fd==-1)
    {
        perror(filename);
        return -1;
    }

    unsigned char bmp_buf[bmp_w*bmp_h*3];
    memset(bmp_buf, 0, bmp_w*bmp_h*3);
    //跳过54字节格式头
    lseek(bmp_fd,54,SEEK_SET);
    //读取像素数据
    int n=read(bmp_fd,bmp_buf,bmp_w*bmp_h*3);
    close(bmp_fd);
    //判断读取的字节数错误
    if(n!=(int)(bmp_w*bmp_h*3))
    {
        printf("read %s short: %d\n",filename,n);
        return -1;
    }

    int x,y;
    //把抠出来的区域转成4字节
    //sy是视觉坐标(从上往下)，bmp是自下而上存储的：
    //区域的视觉最下行(sy+sh-1)对应存储行(bmp_h-1-(sy+sh-1))
    int basey=bmp_h-1-sy-sh+1;
    int lcd_buf[sw*sh];
    memset(lcd_buf, 0, sw*sh*sizeof(int));
    for(y=0;y<sh;y++)
    {
        for(x=0;x<sw;x++)
        {
            int bix=sx+x;//当前遍历的横坐标
            int biy=basey+y;
            if(bix<0||bix>=bmp_w||biy<0||biy>=bmp_h)
                continue;
            int bi=(biy*bmp_w+bix)*3;
            lcd_buf[y*sw+x]=bmp_buf[bi+0]<<0 | bmp_buf[bi+1]<<8 | bmp_buf[bi+2]<<16;
        }
    }

    //颠倒后贴到屏幕(x0,y0)处
    for(y=0;y<sh;y++)
    {
        int ly=y0+y;
        if(ly<0||ly>=480)
            continue;
        for(x=0;x<sw;x++)
        {
            int lx=x0+x;
            if(lx<0||lx>=800)
                continue;
            lcd->mptr[ly*800+lx]=lcd_buf[(sh-1-y)*sw+x];
        }
    }

    return 0;
}

/**
 * @brief 在LCD屏幕指定位置打印一行文字
 * @param lcd      LCD设备指针，由create_lcd创建
 * @param f        字库指针，fontLoad加载的字体
 * @param lcd_x    文字画布在LCD上的起始X坐标
 * @param lcd_y    文字画布在LCD上的起始Y坐标
 * @param bmp_w    文字画布(bitmap)宽度
 * @param bmp_h    文字画布(bitmap)高度
 * @param bg_color 画布背景色 RGB
 * @param font_x   文字在bitmap内部的X偏移
 * @param font_y   文字在bitmap内部的Y偏移
 * @param str      要打印的字符串
 * @param font_color 文字颜色 RGB
 * @param font_size 字体大小
 * @return 无
 */
void Lcd_DrawText(struct LcdDev *lcd, font *f,
                  int lcd_x, int lcd_y,
                  int bmp_w, int bmp_h,
                  unsigned int bg_color,
                  int font_x, int font_y,
                  char *str,
                  unsigned int font_color,
                  int font_size)
{
    if (lcd == NULL || f == NULL || str == NULL)
    {
        return;
    }
    // 设置字体大小
    fontSetSize(f, font_size);
    // 创建位图画布并初始化背景色
    bitmap *bm = createBitmapWithInit(bmp_w, bmp_h, 3, bg_color);
    if (bm == NULL)
    {
        return;
    }
    // 在bitmap上绘制文字
    fontPrint(f, bm, font_x, font_y, str, font_color, 0);
    // 把画布刷到LCD对应位置
    lcd->draw_rgb(lcd_x, lcd_y, bm->map, bm->width, bm->height);
    // 释放位图
    destroyBitmap(bm);
}


