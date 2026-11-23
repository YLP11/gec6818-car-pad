#ifndef __GAME_H
#define __GAME_H

//游戏中心：菜单里选五子棋或贪吃蛇
void Game_UI(void);
//五子棋（双人轮流落子，连五获胜）
void Gomoku_UI(void);
//贪吃蛇（滑动控制方向，吃食物变长加速）
void Snake_UI(void);

#endif
