#include <genesis.h>
#include "pal.h"

// 常數設定.
#define GROUND_Y 200
#define DINO_Y 180
#define DINO_WIDTH 16
#define DINO_HEIGHT 16
#define OBSTACLE_WIDTH 8
#define OBSTACLE_HEIGHT 16
#define JUMP_HEIGHT 40

#define BONUS_START_X 320
#define BONUS_Y 120
#define BONUS_SPEED 4

#define CLOUD1_START_X 320
#define CLOUD1_Y 16
#define CLOUD2_START_X 400
#define CLOUD2_Y 24
#define CLOUD_SPEED 1

// 恐龍方塊結構.
typedef struct {
    s16 x, y;
    u16 width, height;
    u8 isJumping;
    s16 jumpCounter;
} Dino;
// 障礙物結構.
typedef struct {
    s16 x, y;
    u16 width, height;
    u8 isActive;
} Obstacle;
// 寶物結構.
typedef struct {
    s16 x;
    s16 y;
    bool isActive;
} Bonus;
// 雲結構.
typedef struct {
    s16 x;
    s16 y;
} Cloud;

Dino dino;
Obstacle obstacle;
Bonus bonus;
Cloud cloud1;
Cloud cloud2;

// 在main函數開頭添加分數變數.
int score = 0;      
// 在main函數開頭添加最高分數變數.
int highScore = 0;  

void createDino();
void createObstacle();
void updateDino();
void updateObstacle();
bool checkCollision();
void updateScore();
void updateHighScore();

//----------------------------------------------------------------------------
// 雲 - 初始.
//----------------------------------------------------------------------------
void initClouds() {
    cloud1.x = CLOUD1_START_X;
    cloud1.y = CLOUD1_Y;
    cloud2.x = CLOUD2_START_X;
    cloud2.y = CLOUD2_Y;
}

//----------------------------------------------------------------------------
// 雲 - 更新.
//----------------------------------------------------------------------------
void updateCloud(Cloud *cloud) {
    // 清除舊雲位置
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 0), cloud->x / 8, cloud->y / 8);

    cloud->x -= CLOUD_SPEED;

    // 如果雲超出螢幕左邊邊界，將其重置到右側
    if (cloud->x < -16) {
        cloud->x = 320;
    }

    // 繪製新雲位置
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 4), cloud->x / 8, cloud->y / 8);
}

//----------------------------------------------------------------------------
// 初始寶物方塊.
//----------------------------------------------------------------------------
void spawnBonus() {
    bonus.x = BONUS_START_X;
    bonus.y = BONUS_Y;
    bonus.isActive = TRUE;
}

//----------------------------------------------------------------------------
// 更新寶物方塊.
//----------------------------------------------------------------------------
void updateBonus() {
    if (!bonus.isActive) {
        return;
    }

    // 清除舊方塊位置
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 0), bonus.x / 8, bonus.y / 8);

    bonus.x -= BONUS_SPEED;

    // 如果方塊超出螢幕左邊邊界，將 isActive 設為 FALSE
    if (bonus.x < -8) {
        bonus.isActive = FALSE;
        return;
    }

    // 檢查恐龍與方塊之間的碰撞
    if (dino.y + 16 >= bonus.y && dino.y <= bonus.y + 8 && dino.x + 16 >= bonus.x && dino.x <= bonus.x + 8) {
        bonus.isActive = FALSE;
        score += 100;
        return;
    }

    // 繪製方塊
    if (bonus.isActive) {
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 3), bonus.x / 8, bonus.y / 8);
    } else {
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 0), bonus.x / 8, bonus.y / 8);
    }
}

//----------------------------------------------------------------------------
// 初始障礙物.
//----------------------------------------------------------------------------
void initObstacle() {
    obstacle.x = 320;
    obstacle.y = GROUND_Y - 8;
    obstacle.width = 2;
}

//----------------------------------------------------------------------------
// 亂數障礙物高低.
//----------------------------------------------------------------------------
void spawnObstacle() {
    obstacle.x = 320;

    // 生成隨機高度（1到5格）
    obstacle.height = 1 + (random() % 5);
    
    // 繪製障礙物
    for (int i = 0; i < obstacle.height; i++) {
        VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, FALSE, FALSE, FALSE, 1), obstacle.x / 8, (obstacle.y - i * 8) / 8, obstacle.width, 1);
    }
}

//----------------------------------------------------------------------------
// 程式進入點.
//----------------------------------------------------------------------------
int main() {
    // 初始化螢幕
    VDP_init();

    // 設置解析度
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();

    // 設定背景顏色.
    PAL_setColor (0, RGB24_TO_VDPCOLOR(0xa1adff));
    // 設定障礙物顏色.
    PAL_setColor (1, RGB24_TO_VDPCOLOR(0xde5718));
    // 設定恐龍顏色.
    PAL_setColor (2, RGB24_TO_VDPCOLOR(0x0600ff));
    // 設定寶物顏色.
    PAL_setColor (3, RGB24_TO_VDPCOLOR(0xff0000));
    // 設定雲顏色.
    PAL_setColor (4, RGB24_TO_VDPCOLOR(0xc9c9c9));

    // 加入搖桿判斷.
    u8 value;
    value = JOY_getPortType(PORT_1);
    switch (value)
    {
        case PORT_TYPE_MENACER:
            JOY_setSupport(PORT_1, JOY_SUPPORT_MENACER);
            break;
        case PORT_TYPE_JUSTIFIER:
            JOY_setSupport(PORT_1, JOY_SUPPORT_JUSTIFIER_BOTH);
            break;
        case PORT_TYPE_MOUSE:
            JOY_setSupport(PORT_1, JOY_SUPPORT_MOUSE);
            break;
        case PORT_TYPE_TEAMPLAYER:
            JOY_setSupport(PORT_1, JOY_SUPPORT_TEAMPLAYER);
            break;
    }
    
    createDino();
    createObstacle();

    // 初始化雲.
    initClouds(); 

    while(1)
    {        
        SYS_doVBlankProcess();

        // 更新恐龍與障礙物.    
        updateDino();
        updateObstacle();
        updateScore();        // 更新分數
        updateHighScore();
        updateBonus();        // 更新方塊

        updateCloud(&cloud1); // 更新雲1
        updateCloud(&cloud2); // 更新雲2

        if (checkCollision()) {            
            // 當遊戲結束時，顯示文字提示            
            VDP_drawTextBG( BG_A, "Game Over!", 15, 12);

            while (1) {
                SYS_doVBlankProcess();
                
                u16 keys = JOY_readJoypad(JOY_1);
                if (keys & BUTTON_START){
                    // 清除畫面.
                    VDP_clearPlane	(BG_A, TRUE);
                    // 初始遊戲.
                    createDino();
                    createObstacle();
                    // 初始分數.
                    score = 0;

                    break;
                }
                VDP_waitVSync();
            }
        }
        VDP_waitVSync();
    }
    return 0;
}

//----------------------------------------------------------------------------
// 建立恐龍方塊.
//----------------------------------------------------------------------------
void createDino() {
    dino.x = 32;
    dino.y = DINO_Y;
    dino.width = DINO_WIDTH;
    dino.height = DINO_HEIGHT;
    dino.isJumping = FALSE;
    dino.jumpCounter = 0;
}

//----------------------------------------------------------------------------
// 建立障礙物.
//----------------------------------------------------------------------------
void createObstacle() {
    obstacle.x = 320;
    obstacle.y = GROUND_Y - OBSTACLE_HEIGHT;
    obstacle.width = OBSTACLE_WIDTH;
    obstacle.height = 1;
    obstacle.isActive = TRUE;
}

//----------------------------------------------------------------------------
// 更新恐龍方塊.
//----------------------------------------------------------------------------
void updateDino() {
    // 清除恐龍之前的位置
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 0), dino.x / 8, dino.y / 8);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 0), dino.x / 8 + 1, dino.y / 8);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 0), dino.x / 8, dino.y / 8 + 1);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 0), dino.x / 8 + 1, dino.y / 8 + 1);

    if (dino.isJumping) {
        dino.jumpCounter++;
        if (dino.jumpCounter < JUMP_HEIGHT) {
            dino.y -= 2;
        } else if (dino.jumpCounter < 2 * JUMP_HEIGHT) {
            dino.y += 2;
        } else {
            dino.jumpCounter = 0;
            dino.isJumping = FALSE;
        }
    }

    // 根據按鍵狀態更新恐龍
    u16 keys = JOY_readJoypad(JOY_1);
    if (keys & BUTTON_A && !dino.isJumping) {
        dino.isJumping = TRUE;
    }

    // 根據恐龍與障礙物的距離自動跳起
    //if (!dino.isJumping && dino.y == DINO_Y && obstacle.x - (dino.x + DINO_WIDTH) < 32) {
    //    dino.isJumping = TRUE;
    //}

    // 恐龍落地後將 dino.isJumping 設定為 FALSE
    if (dino.y > DINO_Y) {
        dino.y = DINO_Y;
        dino.isJumping = FALSE;
    }

    // 繪製新的恐龍位置
    VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 2), dino.x / 8, dino.y / 8, 2, 2);
}

//----------------------------------------------------------------------------
// 更新障礙物.
//----------------------------------------------------------------------------
void updateObstacle() {
    // 清除障礙物之前的位置
    for (int i = 0; i < obstacle.height; i++) {
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 0), obstacle.x / 8, (obstacle.y - i * 8) / 8);
    }

    // 更新障礙物位置，依照分數設定障礙物一動速度.
    if(score>1200){
        obstacle.x-=8;
    }else if(score>500){
        obstacle.x-=4;
    }else if(score>50){
        obstacle.x-=2;
    }else{
        obstacle.x--;
    }

    // 讓恐龍往右每走一格(8像素)就加1分.
    if(obstacle.x%8 == 0){
        score++;
    }

    // 檢查是否需要重新生成障礙物
    if (obstacle.x + 8 * obstacle.width < 0) {
        spawnObstacle();
    }

    // 繪製新的障礙物位置
    for (int i = 0; i < obstacle.height; i++) {
        VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 1), obstacle.x / 8, (obstacle.y - i * 8) / 8, 1, 2);
    }
}

//----------------------------------------------------------------------------
// 判斷恐龍方塊與障礙物碰撞.
//----------------------------------------------------------------------------
bool checkCollision() {
    // 如果恐龍在空中並且足夠高以避免碰撞，則返回FALSE.
    if (dino.isJumping && dino.y <= (obstacle.y - obstacle.height * 8)) {    
        return FALSE;
    }

    // 如果恐龍和障礙物在水平位置上沒有接觸，則返回FALSE.
    if (dino.x + 16 < obstacle.x || dino.x > obstacle.x + obstacle.width * 2) {
        return FALSE;    
    }

    return TRUE;
}

//----------------------------------------------------------------------------
// 更新分數.
//----------------------------------------------------------------------------
void updateScore() {
    static int prevScore = 0;
    
    // 新增修改方塊顯示中就不要再進入執行(!bonus.isActive).
    if (score % 100 == 0 && score != prevScore && !bonus.isActive) {
        spawnBonus();
        prevScore = score;
    }

    char scoreStr[10];
    sprintf(scoreStr, "Score: %d", score);
    VDP_drawText(scoreStr, 2, 0);
}

//----------------------------------------------------------------------------
// 更新最高分數.
//----------------------------------------------------------------------------
void updateHighScore() {
    if (score > highScore) {
        highScore = score;
    }
    char highScoreStr[15];
    sprintf(highScoreStr, "High Score: %d", highScore);    
    VDP_drawText(highScoreStr, 16, 0);
}