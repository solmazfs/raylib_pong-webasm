/*******************************************************************************************
*
*   raylib study [main.c] - Pong _ ver 0.2
*
*   Game originally created with raylib 4.2, last time updated with raylib 4.2
*
*   Game licensed under MIT.
*
*   Copyright (c) 2022 Fatih S. Solmaz (@solmazfs)
*
********************************************************************************************/

// Font: PICO-8 --> https://www.lexaloffle.com/bbs/?tid=3760

// some of referance i used from "Richard Carter @Ricket"
// ball bounce: https://gamedev.stackexchange.com/questions/4253/in-pong-how-do-you-calculate-the-balls-direction-when-it-bounces-off-the-paddl
// paddle ai: https://github.com/Ricket/richardcarter.org/blob/gh-pages/pong/pongai.js

// todo: collision issue

#include <stdio.h>
#include <math.h>
// random
#include <time.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define _SCREEN_W 640
#define _SCREEN_H 360

Camera2D camera;
Font font;

struct Board {
    Vector2 walls;
    Vector2 middle_line_a;
    Vector2 middle_line_b;
    int font_size;
    float text_hx,text_cx,text_y;
    Color text_col;
}board;

struct Ball {
    Vector2 position;
    Vector2 velocity;
    Vector2 center;
    int rand_arr[6],rand_arr_size;
    int size;
    int radius;
    float angle,speed,speed_up;
    Color color;
}ball;

struct Human {
    Vector2 position;
    Vector2 velocity;
    Rectangle rect;
    int paddle_w,paddle_h,offset;
    int score;
    float speed,max_speed;
    bool enable_ai;
} human;

struct Computer {
    Vector2 position;
    Vector2 velocity;
    Rectangle rect;
    int paddle_w,paddle_h,offset;
    int score;
    float speed,max_speed;
} computer;

void update_ball();
void update_human();
Rectangle update_human_paddle();
void update_computer();
Rectangle update_computer_paddle();
void update_gameplay();
void reset();
void draw_board();
void draw_ball();
void draw_human_paddle();
void draw_computer_paddle();
void draw_score();
void UpdateDrawFrame(void);

int main() {
    srand(time(NULL));
    SetWindowState(FLAG_VSYNC_HINT);
    InitWindow(_SCREEN_W,_SCREEN_H,"Pong");
    camera.zoom = 1.0f;

    board.font_size = 18;
    board.text_hx = (_SCREEN_W/2.0f)+12;
    board.text_cx = (_SCREEN_W/2.0f)-MeasureText("Â 0000",board.font_size)-50;
    board.text_y = 28;
    board.text_col = GetColor(0xa0ff9dff);

    board.middle_line_a = (Vector2){_SCREEN_W/2.0f,0};
    board.middle_line_b = (Vector2){_SCREEN_W/2.0f,_SCREEN_H};

    // ball
    ball.rand_arr[0] = -6;
    ball.rand_arr[1] = -5;
    ball.rand_arr[2] = -4;
    ball.rand_arr[3] = 4;
    ball.rand_arr[4] = 5;
    ball.rand_arr[5] = 6;
    ball.rand_arr_size = (sizeof(ball.rand_arr)/sizeof(ball.rand_arr[0]));

    ball.size = 22;
    ball.radius = 10;
    ball.position = (Vector2){_SCREEN_W/2.0f-ball.size/2.0f,_SCREEN_H/2.0f-ball.size/2.0f};
    ball.velocity = ball.position;
    ball.velocity.x = ball.rand_arr[rand() % ball.rand_arr_size]; //GetRandomValue(-4,4); //4.0f;
    ball.velocity.y = ball.rand_arr[rand() % ball.rand_arr_size]; //GetRandomValue(-4,4); //4.0f;
    ball.center = (Vector2){ball.position.x + 12, ball.position.y + 9};
    ball.speed = 48.0f;
    ball.speed_up = 8.0f;
    ball.color = WHITE;
    // human
    human.enable_ai = false;
    human.score = 0;
    human.speed = 600.0f;
    human.max_speed = (float)1000/1000;
    human.paddle_w = 26;
    human.paddle_h = 80;
    human.offset = 4;
    human.position.x = _SCREEN_W-(human.paddle_w+22);
    human.position.y = (_SCREEN_H/2.0f)-(human.paddle_h/2.0f);
    human.rect = (Rectangle){human.position.x,human.position.y,human.paddle_w,human.paddle_h};
    // computer
    computer.score = 0;
    computer.speed = 600.0f;
    computer.max_speed = (float)1000/1000;
    computer.paddle_w = 26;
    computer.paddle_h = 80; // 300 debug
    computer.offset = 4;
    computer.position.x = 22;
    computer.position.y = (_SCREEN_H/2.0f)-(computer.paddle_h/2.0f);
    computer.rect = (Rectangle){computer.position.x,computer.position.y,computer.paddle_w,computer.paddle_h};

    font = LoadFontEx("resources/PICO-8_wide-upper.ttf",board.font_size,0,250);
    GenTextureMipmaps(&font.texture);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);


    #if defined(PLATFORM_WEB)
        emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
    #else
        SetWindowPosition(0,0); // open window at top-left corner
        SetTargetFPS(60);

        while (!WindowShouldClose()) {
            UpdateDrawFrame();
        }
    #endif
    UnloadFont(font);
    CloseWindow();
    return 0;
}

void UpdateDrawFrame(void) {
    update_ball();
    update_human();
    update_computer();
    update_gameplay();
    BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);
            // DrawTextEx(font,"ÀÁÂÃÄÅ\nÆÇÈÉÊË\nÌÍÎÏ\nÐÑÒÓÔÕÖ\n×ØÙ",(Vector2){68,68},font.baseSize,0,RED);
            draw_board();
            draw_ball();
            draw_human_paddle();
            draw_computer_paddle();
            draw_score();
            // DrawFPS(4,4);
        EndMode2D();
    EndDrawing();
}

void update_ball() {
    ball.position.x += ball.velocity.x * GetFrameTime() * ball.speed;
    ball.position.y += ball.velocity.y * GetFrameTime() * ball.speed;

    ball.center = (Vector2){ball.position.x + 12, ball.position.y + 9};
    // top
    if (ball.position.y <=board.font_size) {
        ball.position.y = board.font_size;
        ball.velocity.y *= -1;
    }
    // bottom
    if (ball.position.y >= _SCREEN_H-board.font_size-ball.size) {
        ball.position.y = GetScreenHeight()-board.font_size-ball.size;
        ball.velocity.y *= -1;
    }
    // human
    if (CheckCollisionCircleRec(ball.center,ball.radius,human.rect)) {
        float a = (human.position.y+(human.paddle_h/2.0f)) - (ball.center.y);
        float b = (a/(human.paddle_h/2.0f));
        float c = (b * (7*PI/36)); // 35deg
        ball.speed_up *= 1.06f;
        ball.speed_up = Clamp(ball.speed_up,8,23); // max speed
        ball.velocity.x = floor(-cos(c) * ball.speed_up);
        ball.velocity.y = floor(-sin(c) * ball.speed_up);
    }
    // computer
    if (CheckCollisionCircleRec(ball.center,ball.radius,computer.rect)) {
        float a = (computer.position.y+(computer.paddle_h/2.0f)) - (ball.center.y);
        float b = (a/(computer.paddle_h/2.0f));
        float c = (b * (7*PI/36)); // 35deg
        ball.speed_up *= 1.06f;
        ball.speed_up = Clamp(ball.speed_up,8,23); // max speed
        ball.velocity.x = floor(cos(c) * ball.speed_up);
        ball.velocity.y = floor(-sin(c) * ball.speed_up);
    }
}

void update_human() {
    human.rect = update_human_paddle();
    if (IsKeyDown(KEY_DOWN) && !human.enable_ai) {
        human.position.y += human.speed * GetFrameTime();
    }
    if (IsKeyDown(KEY_UP) && !human.enable_ai) {
        human.position.y -= human.speed * GetFrameTime();
    }
}

void update_computer() {
    computer.rect = update_computer_paddle();
}

Rectangle update_human_paddle() {
    if (IsKeyPressed(KEY_SPACE)) {
        human.enable_ai = !human.enable_ai;
    }
    if (human.enable_ai) {
       if (ball.velocity.x > 0 && ball.position.x + (ball.radius/2.0f) > _SCREEN_W/2.0f ) {
           if (ball.position.y + (ball.radius/2.0f) != human.position.y + (human.paddle_h/2.0f)) {
               float timetilcol = ((_SCREEN_W-human.offset-human.paddle_w)-ball.position.x)/ball.velocity.x;
               float distancewanted = (human.position.y+(human.paddle_h/2.0f)) - (ball.position.y+(ball.radius/2.0f));
               float velocitywanted = -distancewanted/timetilcol;
               if (velocitywanted > human.max_speed) {
                   human.velocity.y = human.max_speed;
               } else if (velocitywanted < -human.max_speed) {
                   human.velocity.y = -human.max_speed;
               } else {human.velocity.y = velocitywanted;}
           } else {human.velocity.y = 0;}
       } else {human.velocity.y = 0;}
    }
    human.position.y += human.velocity.y * human.speed * GetFrameTime();
    human.position.y = Clamp(human.position.y,22,_SCREEN_H-human.paddle_h-22);
    return (Rectangle){human.position.x,human.position.y,human.paddle_w,human.paddle_h};
}

Rectangle update_computer_paddle() {
    if (ball.velocity.x < 0 && ball.position.x + (ball.radius/2.0f) < _SCREEN_W/2.0f ) {
        if (ball.position.y + (ball.radius/2.0f) != computer.position.y + (computer.paddle_h/2.0f)) {
            float timetilcol = ((computer.offset+computer.paddle_w)-ball.position.x)/ball.velocity.x;
            float distancewanted = (computer.position.y+(computer.paddle_h/2.0f)) - (ball.position.y+(ball.radius/2.0f));
            float velocitywanted = -distancewanted/timetilcol;
            if (velocitywanted > computer.max_speed) {
                computer.velocity.y = computer.max_speed;
            } else if (velocitywanted < -computer.max_speed) {
                computer.velocity.y = -computer.max_speed;
            } else {computer.velocity.y = velocitywanted;}
        } else {computer.velocity.y = 0;}
    } else {computer.velocity.y = 0;}
    computer.position.y += computer.velocity.y * computer.speed * GetFrameTime();
    computer.position.y = Clamp(computer.position.y,22,_SCREEN_H-computer.paddle_h-22);
    return (Rectangle){computer.position.x,computer.position.y,computer.paddle_w,computer.paddle_h};
}

void update_gameplay() {
    if (ball.position.x <= -board.font_size*2) {
        human.score++;
        human.score = Clamp(human.score,0,9999);
        reset();
    }
    if (ball.position.x >= _SCREEN_W+board.font_size) {
        computer.score++;
        computer.score = Clamp(computer.score,0,9999);
        reset();
    }
}

// RESET
void reset() {
    // todo: ball direction according to winner
    human.position.y = (_SCREEN_H/2.0f)-(human.paddle_h/2.0f);
    computer.position.y = (_SCREEN_H/2.0f)-(human.paddle_h/2.0f);
    ball.position.x = GetScreenWidth() / 2.0f - ball.size/2.0f;
    ball.position.y = GetScreenHeight() / 2.0f - ball.size/2.0f;
    ball.velocity.x = ball.rand_arr[rand() % ball.rand_arr_size];
    ball.velocity.y = ball.rand_arr[rand() % ball.rand_arr_size];
    ball.speed_up = 8.0f;
}

void draw_board() {
    // walls
    for (int i=0; i<2; i++) {
        for (int j=3;j<_SCREEN_W-26; j+=29) {
            DrawTextEx(font, "À",(Vector2){j,(_SCREEN_H-board.font_size)*i},board.font_size,0,RAYWHITE);
        }
    }
    // middle line
    for (int i=19; i<_SCREEN_H-38; i+=board.font_size) {
        DrawTextEx(font, ".",(Vector2){(_SCREEN_W/2.0f)-(board.font_size/2.0f-4),i},board.font_size,0,RAYWHITE);
    }
}

void draw_ball() {
    DrawTextEx(font, "Æ",ball.position,board.font_size,0,ball.color);
}

void draw_human_paddle() {
    for (int i=0; i<human.paddle_h; i+=20) {
        DrawTextEx(font, "À", (Vector2){human.position.x,human.position.y+i},board.font_size,0,WHITE);
    }
}

void draw_computer_paddle() {
    for (int i=0; i<computer.paddle_h; i+=20) {
        DrawTextEx(font, "À", (Vector2){computer.position.x,computer.position.y+i},board.font_size,0,WHITE);
    }
}

// score --> computer & human
void draw_score() {
    DrawTextEx(font, TextFormat("Ì %04d",human.score),(Vector2){board.text_hx,board.text_y},board.font_size,0,board.text_col);
    DrawTextEx(font, TextFormat("Â %04d",computer.score),(Vector2){board.text_cx,board.text_y},board.font_size,0,board.text_col);
}
