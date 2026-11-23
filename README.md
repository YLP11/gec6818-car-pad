# 车载娱乐小平板（GEC6818）

基于粤嵌 GEC6818 开发板（三星 S5P6818，Cortex-A53 八核）的车载娱乐小平板课程项目。设备运行 Linux，以 800×480 电阻触摸屏为唯一交互界面，**不依赖任何 GUI 图形库**，全部界面用 C 语言直接操作 Linux framebuffer 绘制。

## 功能

| 功能 | 说明 |
|---|---|
| 用户系统 | 注册 / 登录，账号存板端 user.txt，屏幕软键盘输入（字母 / 数字切换）|
| 主界面 | 顶部每秒刷新日期时间；四张功能卡片左右滑动切换、点按进入；登出 / 关机图标带二次确认 |
| 电子相册 | 缩略图网格浏览、大图 `< >` 切换、设为壁纸、删除二次确认 |
| 音乐播放 | madplay 子进程播放，播放 / 暂停 / 上一首 / 下一首，左右滑切歌，到头提示 |
| 视频播放 | 列表页缩略图点播，mplayer 直接输出 framebuffer，播放中点按弹暂停菜单 |
| 游戏中心 | 五子棋（双人，胜利横幅）、贪吃蛇（手势控制、暂停图、结束得分）|
| 系统操作 | 登出回登录页；关机播 4 秒关机动画后结束进程 |

## 目录结构

```
1_文档资料/          项目书（.lake 语雀格式）
2_程序资料/
  ├─ src/            源码：main.c login.c ui_main.c album.c media.c game.c
  │                        bmp.c font.c truetype.c touch.c lcddev.c + CMakeLists.txt
  ├─ inc/            头文件（宏定义集中在此）
  └─ res/            资源：BMP 界面图、音乐、视频、字库等
3_视频资料/          演示视频
```

## 编译与部署

交叉编译工具链：`arm-linux-gcc 5.4.0`

```bash
cd 2_程序资料
mkdir -p build && cd build
cmake ../src        # CMakeLists.txt 在 src 目录下
make                # 产出 a.out
scp a.out root@<板子IP>:/root/car
```

板端运行：

```bash
cd /root/car && ./a.out
```

资源需放在板端 `/root/car/res` 目录下，程序按约定路径读取；替换同名文件即可更换界面风格与素材。

## 技术要点

- **framebuffer 绘图**：`open` + `mmap` 映射 `/dev/fb0`，直接写 800×480×4 字节像素
- **BMP 贴图**：自写 24/32 位 BMP 解析，处理行 4 字节对齐与屏幕边缘裁剪
- **触摸识别**：input 子系统读 `/dev/input/event0`，同时识别点按与四方向滑动；按钮统一用「按下那一刻的坐标」判定，并把点击范围扩大到图标四周
- **进程管理**：`popen` 启动 madplay / mplayer 并用 `echo $!` 取 PID，通过 `SIGSTOP / SIGCONT / SIGKILL` 精确暂停、继续与停止（板端 BusyBox 的 killall 不可靠）
- **字库渲染**：truetype 矢量字库解码，支持任意字号中文
- **多线程**：时钟线程、贪吃蛇触摸线程，退出前 `pthread_cancel` + `join` 回收

## 说明

- 开发板无 RTC 电池，断电后系统时间会回到出厂时间，演示前用 `date -s` 校时。
- 板载存储有限，照片较多时可用 NFS 挂载宿主机共享目录存放。
- 音视频素材为开源动画（Big Buck Bunny、Caminandes）与自绘图片，避免版权问题。
