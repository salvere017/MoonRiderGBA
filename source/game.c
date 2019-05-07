/**
 * @file game.c
 * @brief ゲーム本体
 * @date  2019.04.20 作成
 * @author Choe Gyun(choikyun)
 */

/***************************************************
 * Meteorite
 * ver 1.0.0
 * 2019.04.19
 * Choi Gyun
 *****************************************************/

#define GLOBAL_VALUE_DEFINE

#include <gba.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bg.h"
#include "game.h"
#include "graph.h"
#include "music.h"
#include "score.h"
#include "sprite.h"

void init_game();
static void
disp_title();
static void
init_ship();
static void
init_fire();
static void
init_stars();
static void
init_star(int, int, int);
static void
move_ship();
static void
disp_ship();
static void
disp_stars();
static void
disp_fire();
static void
draw_bg();
static void
init_sprite_setting();
static void
init_stage();
static void
move_stars();
static void
check_stage_boundary();
static void
create_new_stars();
static void
copy_affine(int, int);
static void
copy_sp_attr(int, int);
static void
init_lines();
static void
create_new_line();
static void
move_lines();
static void
draw_lines();
static void
draw_line(int);
static VectorType
trans_device_coord(SpriteCharType*);

//debug
void vbaPrint(char* s);

/**********************************************/ /**
 * @brief メインループ
 ***********************************************/
void game()
{
    game_state.key = keysDown();
    game_state.keyr = keysDownRepeat();

    seed++;

    switch (game_state.scene) {
    case GAME_TITLE:
        disp_title();
        //select_mode();
        break;

    case GAME_DEMO:
        break;

    case GAME_MAIN:
        disp_ship();
        disp_fire();
        disp_stars();

        create_new_stars();
        create_new_line();

        move_ship();
        move_stars();
        move_lines();

        draw_bg();
        draw_lines();
        break;

    case GAME_READY:
        break;

    case GAME_PAUSE:
        break;

    case GAME_OVER:
        break;
    }
}

/**********************************************/ /**
 * @brief ゲーム初期化
 ***********************************************/
void init_game()
{
    // ゲームステート初期化
    game_state.scene = GAME_TITLE;
    stage.mode = 0;

    // パラメータ初期化
    score = 0;
    stage.lv = 1;

    // ハイスコアのロード
    //hiscore = load_hiscore();

    // スプライト初期化
    init_sprite_setting();

    // 自機初期化
    init_ship();

    // スター初期化
    init_stars();

    // 地平線
    init_lines();

    // ステージ初期化
    init_stage();
}

/**********************************************/ /**
 * @brief ブロック作成
 ***********************************************/
static void
create_new_stars()
{
    // 一定間隔で出現
    if (--stars.interval <= 0) {

        for (int i = 0; i < APPER_MAX_STARS; i++) {
            if (stars.num < MAX_STARS) {
                stars.num++;

                init_star(
                    MAX_STARS - stars.num,
                    RND(0, STAR_X_STEP_NUM) * STAR_X_STEP + STAR_W / 2 - SCREEN_CENTER,
                    STAR_Y_TARGET
                    /*RND(0, STAR_Y_STEP_NUM) * STAR_Y_STEP + STAR_H / 2 + STAR_Y_TARGET_BLANK*/);
            }
        }

        // 次の出現間隔
        stars.interval = STAR_INTERVAL;
    }
}

/**********************************************/ /**
 * @brief スター移動
 ***********************************************/
static void
move_stars()
{
    int cur, end;
    OBJATTR* sp = OAM + SPRITE_STAR;

    if (!stars.num) {
        return;
    }

    // 前からバッファを操作
    cur = end = MAX_STARS - stars.num;
    int max = stars.num;
    for (int i = 0; i < max; i++, cur++) {
        stars.list[cur].vec.z += stars.list[cur].acc.z;

        if ((stars.list[cur].vec.z >> FIX) < MIN_Z) {
            // スプライトを消去する
            // データ
            CpuSet(
                &stars.list[end],
                &stars.list[cur],
                (COPY32 | (sizeof(SpriteCharType) / 4)));
            // スプライト属性
            //copy_sp_attr(end, cur);
            // アフィンパラメータ
            //copy_affine(end, cur);
            set_affine_setting(SPRITE_STAR + cur, cur, 0);
            // スプライト消去
            erase_sprite(SPRITE_STAR + end);

            stars.num--;
            end++;
        }
    }
}

/**********************************************/ /**
 * @brief スプライト属性のコピー
 *
 *  @param sorc 元
 * @param dest 先
 ***********************************************/
static void
copy_sp_attr(int sorc, int dest)
{
    OBJATTR* s = OAM + sorc;
    OBJATTR* d = OAM + dest;

    d->attr0 = s->attr0;
    d->attr1 = s->attr1;
    d->attr2 = s->attr2;
}

/**********************************************/ /**
 * @brief アフィンパラメータのコピー
 *
 *  @param sorc 元
 * @param dest 先
 ***********************************************/
static void
copy_affine(int sorc, int dest)
{
    OBJAFFINE* s = OAM + sorc;
    OBJAFFINE* d = OAM + dest;

    d->pa = s->pa;
    d->pb = s->pb;
    d->pc = s->pc;
    d->pd = s->pd;
}

/**********************************************/ /**
 * @brief スプライト初期化
 ***********************************************/
static void
init_sprite_setting()
{
    //// 自機 32*32 dot
    set_sprite_form(SPRITE_SHIP, OBJ_SIZE(2), OBJ_SQUARE, OBJ_256_COLOR);
    set_sprite_tile(SPRITE_SHIP, TILE_SHIP1);

    //// 炎 16*16 dot
    set_sprite_form(SPRITE_FIRE, OBJ_SIZE(1), OBJ_SQUARE, OBJ_256_COLOR);
    set_sprite_tile(SPRITE_FIRE, TILE_FIRE1);

    //// スター 64*64 dot
    for (int i = 0; i < MAX_STARS; i++) {
        set_sprite_form(SPRITE_STAR + i, OBJ_SIZE(Sprite_64x64), OBJ_SQUARE, OBJ_256_COLOR);
        set_sprite_tile(SPRITE_STAR + i, TILE_STAR1);
        set_affine_setting(SPRITE_STAR + i, i, 0);
        set_scale(i, 1, 1);
    }
}

/**********************************************/ /**
 * @brief リスタート
 ***********************************************/
static void
restart()
{
    game_state.scene = GAME_MAIN;

    // ランダマイズ
    srand(REG_TM0CNT + seed);
    // seedをSRAMに保存
    SRAMWrite32(SRAM_SEED, seed);

    // ステージ初期化
    init_stage();
}

/**********************************************/ /**
 * @brief ステージ初期化
 ***********************************************/
static void
init_stage()
{
    // ステージの座標
    stage.center.x = 0;
    stage.center.y = 0;
}

/**********************************************/ /**
 * @brief 自機初期化
 ***********************************************/
static void
init_ship()
{
    // 加速度
    ship.acc.x = ship.acc.y = 0;

    // 自機スプライト
    ship.chr = SPRITE_SHIP;
    ship.vec.x = SHIP_X << FIX;
    ship.vec.y = SHIP_Y << FIX;
    ship.vec.z = SHIP_Z << FIX;
    ship.center.x = SHIP_W / 2;
    ship.center.y = SHIP_H / 2;
    ship.target.x = SHIP_X;
    ship.target.y = SHIP_Y;
    ship.rect.w = SHIP_W;
    ship.rect.h = SHIP_H;
    ship.fix.x = 0;
    ship.fix.y = 0;
}

/**********************************************/ /**
 * @brief 炎初期化
 ***********************************************/
static void
init_fire()
{
    fire.sprite.chr = SPRITE_FIRE;
    fire.sprite.vec.x = FIRE_X << FIX;
    fire.sprite.vec.y = FIRE_Y << FIX;
    fire.sprite.vec.z = FIRE_Z << FIX;
    fire.sprite.center.x = FIRE_W / 2;
    fire.sprite.center.y = FIRE_H / 2;
    fire.sprite.target.x = FIRE_X;
    fire.sprite.target.y = FIRE_Y;
    fire.sprite.rect.w = FIRE_W;
    fire.sprite.rect.h = FIRE_H;
    fire.sprite.fix.x = 0;
    fire.sprite.fix.y = 0;

    // アニメーション
    fire.anime.frame = 0;
    fire.anime.max_frame = 2;
    fire.anime.interval = fire.anime.interval_rel = FIRE_INTERVAL;
    fire.anime.is_start = true;
}

/**********************************************/ /**
 * @brief すべてのスター初期化
 ***********************************************/
static void
init_stars()
{
    for (int i = 0; i < MAX_STARS; i++) {
        // スプライトの割当
        stars.list[i].chr = SPRITE_STAR;
        stars.list[i].center.x = STAR_SP_W / 2;
        stars.list[i].center.y = STAR_SP_H / 2;
        stars.list[i].rect.w = STAR_W;
        stars.list[i].rect.h = STAR_H;
        stars.list[i].fix.x = 0;
        stars.list[i].fix.y = 0;
    }

    stars.interval = 5 * 60;
    stars.num = 0;
}

/**********************************************/ /**
 * @brief スター初期化
 ***********************************************/
static void
init_star(int num, int x, int y)
{
    // 加速度
    stars.list[num].acc.z = STAR_SPEED;

    // スプライト
    stars.list[num].target.x = x; // X座標目標値
    stars.list[num].target.y = y; // Y座標目標値
    stars.list[num].vec.z = MAX_Z << FIX;

    set_affine_setting(SPRITE_STAR + num, num, 0);

    // ブロック or リング
    stars.list[num].type = NORMAL;
    if (!RND(0, 2)) {
        stars.list[num].type = RING;
    }
}

/**********************************************/ /**
 * @brief 地平線初期化
 ***********************************************/
static void
init_lines()
{
    lines.num = 0;
    lines.z = LINE_Z << FIX;
    for (int i = 0; i < MAX_LINES; i++) {
        lines.list[i].vec.x = 0;
        lines.list[i].vec.y = 0;
        lines.list[i].vec.z = MAX_Z << FIX;
        lines.list[i].target.x = 0;
        lines.list[i].target.y = LINE_Y_TARGET;
        lines.list[i].center.x = 0;
        lines.list[i].center.y = 0;
        lines.list[i].fix.y = 0;
        lines.list[i].fix.y = 0;
        lines.list[i].rect.w = 0;
        lines.list[i].rect.h = 0;
    }
}

/**********************************************/ /**
 * @brief 新規ライン作成
 ***********************************************/
static void
create_new_line()
{
    // ブロックと同じ加速度ですすむ
    lines.z += STAR_SPEED;
    if ((lines.z >> FIX) % LINE_INTERVAL != 0) {
        return;
    }
    lines.z = LINE_Z << FIX;

    if (lines.num < MAX_LINES) {
        lines.list[lines.num].vec.y = 0;
        lines.list[lines.num].vec.z = MAX_Z << FIX;
        lines.num++;
    }
}

/**********************************************/ /**
 * @brief ライン移動
 ***********************************************/
static void
move_lines()
{
    if (!lines.num) {
        return;
    }

    int cur, end;

    cur = end = lines.num - 1;
    int max = lines.num;

    for (int i = 0; i < max; i++, cur--) {
        lines.list[cur].vec.z += STAR_SPEED;

        if ((lines.list[cur].vec.z >> FIX) < MIN_Z) {
            // 消去
            CpuSet(
                &lines.list[end],
                &lines.list[cur],
                (COPY32 | (sizeof(SpriteCharType) / 4)));
            lines.num--;
            end--;
        }
    }
}

/**********************************************/ /**
 * @brief ライン描画
 ***********************************************/
static void
draw_lines()
{
    VectorType v;

    for (int i = 0; i < lines.num; i++) {
        v = trans_device_coord(&lines.list[i]);
        draw_line(v.y);
    }
}

/**********************************************/ /**
 * @brief ライン描画
 ***********************************************/
static void
draw_line(int y)
{
    int x = 0;
    u16* dst = current_frame + (y * SCREEN_WIDTH) / 2;

    for (int i = 0; i < LINE_W / 2; i++) {
        *(dst + i) = LINE_COLOR;
    }
}

/**********************************************/ /**
 * @brief 3D座標からデバイス座標に変換
 * 
 * @param *sp スプライト
 * @return 3D座標
 ***********************************************/
static VectorType
trans_device_coord(SpriteCharType* sp)
{
    VectorType v;
    int z = sp->vec.z >> FIX; // Z軸整数値のみ
    int tx = sp->target.x + sp->fix.x; // 目標値の補正
    int ty = sp->target.y + sp->fix.y;

    // sc1 - sc0 でスケールを求める
    float sc0 = ((float)tx / z) * MIN_Z + 0.5f;
    float sc1 = (((float)tx + sp->rect.w) / z) * MIN_Z + 0.5f;

    v.x = sc0 + FIX_STAGE_X - sp->center.x;
    v.y = ((float)ty / z) * MIN_Z + FIX_STAGE_Y - sp->center.y + 0.5f;

    v.scale = ((sc1 - sc0) / sp->rect.w) * 100 + 0.5f;
    if (v.scale <= 0) {
        v.scale = 1;
    }

    return v;
}

/**********************************************/ /**
 * @brief 自機移動
 ***********************************************/
static void
move_ship()
{
    u16 key = game_state.keyr;

    // デフォルトタイル
    set_sprite_tile(SPRITE_SHIP, TILE_SHIP1);

    // デフォルトの炎の座標
    fire.sprite.vec.x = FIRE_X << FIX;
    fire.sprite.vec.y = FIRE_Y << FIX;

    // 移動
    /*if (key & KEY_UP) {
        ship.acc.y -= SHIP_SPEED;
        if (ship.acc.y < -MAX_SHIP_ACC) {
            ship.acc.y = -MAX_SHIP_ACC;
        }
        set_sprite_tile(SPRITE_SHIP, TILE_SHIP4);
        fire.sprite.vec.y = FIRE_Y_UP << FIX;
    } else if (key & KEY_DOWN) {
        ship.acc.y += SHIP_SPEED;
        if (ship.acc.y > MAX_SHIP_ACC) {
            ship.acc.y = MAX_SHIP_ACC;
        }
        set_sprite_tile(SPRITE_SHIP, TILE_SHIP5);
        fire.sprite.vec.y = FIRE_Y_DOWN << FIX;
    }
    */

    if (key & KEY_LEFT) {
        ship.acc.x -= SHIP_SPEED;
        if (ship.acc.x < -MAX_SHIP_ACC) {
            ship.acc.x = -MAX_SHIP_ACC;
        }
        set_sprite_tile(SPRITE_SHIP, TILE_SHIP3);
        fire.sprite.vec.x = FIRE_X_LEFT << FIX;
    } else if (key & KEY_RIGHT) {
        ship.acc.x += SHIP_SPEED;
        if (ship.acc.x > MAX_SHIP_ACC) {
            ship.acc.x = MAX_SHIP_ACC;
        }
        set_sprite_tile(SPRITE_SHIP, TILE_SHIP2);
        fire.sprite.vec.x = FIRE_X_RIGHT << FIX;
    }

    // 自然減速
    if (!(key & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT))) {
        ship.acc.x += -ship.acc.x * SHIP_FRIC;
        ship.acc.y += -ship.acc.y * SHIP_FRIC;
    }

    //ship.vec.x += ship.acc.x;
    //ship.vec.y += ship.acc.y;

    // 自機とは逆方向にステージを移動
    stage.center.x -= ship.acc.x;

    fire.sprite.vec.x = ship.vec.x;
    fire.sprite.vec.y += ship.vec.y;

    // 境界判定
    check_stage_boundary();
}

/**********************************************/ /**
 * @brief ステージ境界チェック
 ***********************************************/
static void
check_stage_boundary()
{
    /*
    int x = ship.vec.x >> FIX;
    int y = ship.vec.y >> FIX;

    if (y < SHIP_MOVE_MIN_Y) {
        ship.vec.y = SHIP_MOVE_MIN_Y << FIX;
        ship.acc.y /= -2; // 加速度反転
    } else if (y > SHIP_MOVE_MAX_Y) {
        ship.vec.y = SHIP_MOVE_MAX_Y << FIX;
        ship.acc.y /= -2;
    }

    if (x < SHIP_MOVE_MIN_X) {
        ship.vec.x = SHIP_MOVE_MIN_X << FIX;
        ship.acc.x /= -2;
    } else if (x > SHIP_MOVE_MAX_X) {
        ship.vec.x = SHIP_MOVE_MAX_X << FIX;
        ship.acc.x /= -2;
    }
    */

    int x = stage.center.x >> FIX;
    int y = stage.center.y >> FIX;

    if (x < SHIP_MOVE_MIN_X) {
        stage.center.x = SHIP_MOVE_MIN_X << FIX;
        ship.acc.x /= -8;
    } else if (x > SHIP_MOVE_MAX_X) {
        stage.center.x = SHIP_MOVE_MAX_X << FIX;
        ship.acc.x /= -8;
    }
}

/**********************************************/ /**
 * @brief 自機表示
 ***********************************************/
static void
disp_ship()
{
    ship.target.x = ship.vec.x >> FIX;
    ship.target.y = ship.vec.y >> FIX;

    // 座標変換
    VectorType v = trans_device_coord(&ship);

    move_sprite(ship.chr, v.x, v.y);
}

/**********************************************/ /**
 * @brief 炎表示
 ***********************************************/
static void
disp_fire()
{
    // アニメ
    if (fire.anime.is_start && --fire.anime.interval < 0) {
        fire.anime.interval = fire.anime.interval_rel;
        fire.anime.frame = (fire.anime.frame + 1) % fire.anime.max_frame;
        // タイル切り替え
        set_sprite_tile(SPRITE_FIRE, TILE_FIRE1 + (fire.anime.frame * TILE_SIZE_16));
    }

    // ターゲット座標 更新
    fire.sprite.target.x = fire.sprite.vec.x >> FIX;
    fire.sprite.target.y = fire.sprite.vec.y >> FIX;

    // 座標変換
    VectorType v = trans_device_coord(&fire.sprite);

    move_sprite(fire.sprite.chr, v.x, v.y);
}

/**********************************************/ /**
 * @brief ブロック表示
 ***********************************************/
static void
disp_stars()
{
    VectorType v;

    int cur = MAX_STARS - stars.num;
    for (int i = 0; i < stars.num; i++, cur++) {
        // ステージ側が移動するのでターゲット座標の補正
        stars.list[cur].fix.x = stage.center.x >> FIX;

        // 座標変換
        v = trans_device_coord(&stars.list[cur]);

        // アフィンパラメータ設定
        //set_affine_setting(SPRITE_STAR + cur, cur, 0);
        // スケールを設定
        set_scale(cur, v.scale, v.scale);

        // ブロック or リング
        if (stars.list[cur].type == NORMAL) {
            set_sprite_tile(SPRITE_STAR + cur, TILE_STAR1);
        } else {
            set_sprite_tile(SPRITE_STAR + cur, TILE_RING1);
        }

        move_sprite(stars.list[cur].chr + cur,
            v.x,
            v.y);
    }
}

/**********************************************/ /**
 * @brief ステージのBG描画
 ***********************************************/
static void
draw_bg()
{
    flip_frame();
    load_frame_bitmap(DEF_BG_BITMAP);
}

/**********************************************/ /**
 * @brief ポーズ
 ***********************************************/
static void
pause()
{
    /*
    u16 key = game_state.key;

    if (key & KEY_SELECT)
    {
        game_state.scene ^= GAME_PAUSE;
        if (game_state.scene & GAME_PAUSE)
        {
            StopMusic();
            PlaySound(SOUND_ITEM);
        }
        else
        {
            PlayMusic(MUSIC_STAGE + stage_bgm, true);
        }
    }
    */
}

/**********************************************/ /**
 * @brief スコア表示
 ***********************************************/
static void
update_score()
{
    /*
    int i;
    int pos = SCORE_DIGIT * NUM_W - NUM_W;
    int sc = score;

    for (i = 0; i < SCORE_DIGIT; i++)
    {
        disp_num(SCORE_X + pos, SCORE_Y, sc % 10);
        sc /= 10;
        pos -= (NUM_W);
    }
    */
}

/**********************************************/ /**
 * @brief ハイスコア表示
 ***********************************************/
void update_hiscore()
{
    /*
    int i;
    int pos = SCORE_DIGIT * NUM_W - NUM_W;
    int sc = hiscore;

    for (i = 0; i < SCORE_DIGIT; i++)
    {
        disp_num_title(HISCORE_X + pos, HISCORE_Y, sc % 10);
        sc /= 10;
        pos -= (NUM_W);
    }
    */
}

/**********************************************/ /**
 * @brief ワーニングメッセージ表示
 ***********************************************/
static void
disp_warning()
{
}

/**********************************************/ /**
 * @brief ポーズメッセージ表示
 ***********************************************/
static void
disp_pause()
{
}

/**********************************************/ /**
 * @brief 点滅メッセージ用パラメータのリセット
 * @param *m メッセージパラメータポインタ
 ***********************************************/
static void
reset_message(BlinkMessageType* m)
{
}

/**********************************************/ /**
 * @brief 点滅メッセージ用パラメータのリセット
 * @param *m メッセージパラメータポインタ
 ***********************************************/
static void
reset_message_fast(BlinkMessageType* m)
{
}

/**********************************************/ /**
 * @brief BGに数字表示
 * @param x X座標
 * @param y Y座標
 * @param num 表示する数値
 ***********************************************/
static void
disp_num(int x, int y, u16 num)
{
}

/**********************************************/ /**
 *  タイトル ビットマップ転送
 ***********************************************/
void load_title()
{
}

/**********************************************/ /**
 * @brief タイトル
 ***********************************************/
static void
disp_title()
{
    u16 key = game_state.key;

    if (key & KEY_START) {
        restart();
        stage.demo = false;
        //  StopMusic ();
    } else if ((key & KEY_R) && (key & KEY_B)) {
        //  clear_hiscore ();
        //  update_hiscore ();
    }
}

//// デバグ用
//THUMB code
void vbaPrint(char* s)
{
    /*
    asm volatile("mov r0, %0;"
                 "swi 0xff;"
                 : // no ouput
                 : "r"(s)
                 : "r0");
    */
}
