/*******************************************************************************************
*
*   raylib study [main.c] - Pong _ ver 0.5
*
*   Game originally created with raylib 4.2, last time updated with raylib 4.2
*
*   Game licensed under MIT.
*
*   Copyright (c) 2022 Fatih S. Solmaz (@solmazfs)
*
********************************************************************************************/

// Font: PICO-8 --> https://www.lexaloffle.com/bbs/?tid=3760
// SFX: Generated with rFXGen by raylib

// some of referance i used from "Richard Carter @Ricket"
// ball bounce: https://gamedev.stackexchange.com/questions/4253/in-pong-how-do-you-calculate-the-balls-direction-when-it-bounces-off-the-paddl
// paddle ai: https://github.com/Ricket/richardcarter.org/blob/gh-pages/pong/pongai.js

// known issues: collision with paddle, 2 times play sound

// TODO:
//  - crt shader

#include <stdio.h>
#include <math.h>
// random
#include <time.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#define _SCREEN_W 640
#define _SCREEN_H 360
//#define GLSL_VERSION 330


#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

Camera2D camera;

enum STATES {START_SCREEN, START, READY, PLAY};

struct Screen { int width, height; } screen;

struct Sfx {
    Sound start, count, count_last, hit_wall, hit_paddle, reset;
} sfx;

struct Board {
    Font font;
    int font_size;
    int frame_counter, game_state, current_frame;
    float text_hx,text_cx,text_y;
    bool blink;
    Color text_col;
    Vector2 walls;
    Vector2 middle_line_a;
    Vector2 middle_line_b;
}board;

struct Ball {
    Vector2 position;
    Vector2 velocity;
    Vector2 center;
    Vector2 hit_position;
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

void update_start_screen();
void update_ball();
void update_human();
Rectangle update_human_paddle();
void update_computer();
Rectangle update_computer_paddle();
void update_gameplay();
void update_game_start();
void count_ready();
void draw_start_screen();
void draw_start_count();
void draw_board();
void draw_ball();
void draw_ball_blink();
void draw_human_paddle();
void draw_computer_paddle();
void draw_score();
void UpdateDrawFrame(void);

int main() {
    srand(time(NULL));
    SetWindowState(FLAG_VSYNC_HINT);
    InitWindow(_SCREEN_W,_SCREEN_H,"Pong");

    // screen size
    screen.width = GetScreenWidth();
    screen.height = GetScreenHeight();
    camera.zoom = 1.0f;
    // shader test
    //screen.shader = LoadShader("resources/shaders/scan.vert", "resources/shaders/scan.frag");

    board.font_size = 18;
    // resources --> font, audio
    InitAudioDevice();
    sfx.start = LoadSound("resources/sfx/start.wav");
    sfx.count = LoadSound("resources/sfx/count.wav");
    sfx.count_last = LoadSound("resources/sfx/count_last.wav");
    sfx.hit_wall = LoadSound("resources/sfx/hit_wall.wav");
    sfx.hit_paddle = LoadSound("resources/sfx/hit_paddle.wav");
    sfx.reset = LoadSound("resources/sfx/reset.wav");
    board.font = LoadFontEx("resources/PICO-8_wide-upper.ttf",board.font_size,0,250);
    GenTextureMipmaps(&board.font.texture);
    SetTextureFilter(board.font.texture, TEXTURE_FILTER_BILINEAR);

    board.text_hx = (screen.width/2.0f)+12;
    board.text_cx = (screen.width/2.0f)-MeasureText("Â 0000",board.font_size)-50;
    board.text_y = 28;
    board.text_col = GREEN; //GetColor(0xa0ff9dff); // #a0ff9d

    board.middle_line_a = (Vector2){screen.width/2.0f,0};
    board.middle_line_b = (Vector2){screen.width/2.0f,screen.height};
    board.current_frame = 3;

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
    ball.position = (Vector2){screen.width/2.0f-ball.size/2.0f,screen.height/2.0f-ball.size/2.0f};
    ball.velocity = Vector2Zero();
    ball.center = (Vector2){ball.position.x + 12, ball.position.y + 9};
    ball.speed = 48.0f;
    ball.speed_up = 0;
    ball.color = WHITE;
    // human
    human.enable_ai = false;
    human.score = 0;
    human.speed = 600.0f;
    human.max_speed = (float)1000/1000;
    human.paddle_w = 26;
    human.paddle_h = 80;
    human.offset = 4;
    human.position.x = screen.width-(human.paddle_w+22);
    human.position.y = (screen.height/2.0f)-(human.paddle_h/2.0f);
    human.rect = (Rectangle){human.position.x,human.position.y,human.paddle_w,human.paddle_h};
    // computer
    computer.score = 0;
    computer.speed = 600.0f;
    computer.max_speed = (float)1000/1000;
    computer.paddle_w = 26;
    computer.paddle_h = 80;
    computer.offset = 4;
    computer.position.x = 22;
    computer.position.y = (screen.height/2.0f)-(computer.paddle_h/2.0f);
    computer.rect = (Rectangle){computer.position.x,computer.position.y,computer.paddle_w,computer.paddle_h};

    #if defined(PLATFORM_WEB)
        emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
    #else
        SetWindowPosition(0,0); // open window at top-left corner
        SetTargetFPS(60);

        while (!WindowShouldClose()) {
            UpdateDrawFrame();
        }
    #endif
    //UnloadShader(screen.shader);
    UnloadFont(board.font);
    UnloadSound(sfx.start);
    UnloadSound(sfx.count);
    UnloadSound(sfx.count_last);
    UnloadSound(sfx.hit_wall);
    UnloadSound(sfx.hit_paddle);
    UnloadSound(sfx.reset);
    CloseWindow();
    return 0;
}

void UpdateDrawFrame(void) {
    switch (board.game_state) {
        case START_SCREEN:
            update_start_screen();
            break;
        case START:
            update_ball();
            update_human();
            update_computer();
            update_gameplay();
            update_game_start();
            break;
        case READY:
            update_ball();
            update_human();
            update_computer();
            update_gameplay();
            count_ready();
            break;
        case PLAY:
            update_ball();
            update_human();
            update_computer();
            update_gameplay();

            break;
        default: break;
    }
    BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);
        //BeginShaderMode(screen.shader);
            switch (board.game_state) {
                case START_SCREEN:
                    draw_start_screen();
                    break;
                case START:
                    draw_board();
                    draw_score();
                    draw_human_paddle();
                    draw_computer_paddle();
                    draw_start_count();
                    break;
                case READY:
                    draw_board();
                    draw_score();
                    draw_human_paddle();
                    draw_computer_paddle();
                    draw_ball_blink();
                    break;
                case PLAY:
                    draw_board();
                    draw_score();
                    draw_human_paddle();
                    draw_computer_paddle();
                    draw_ball();
                    break;
                default: break;
            }
            //DrawFPS(4,4);
        //EndShaderMode();
        EndMode2D();
    EndDrawing();
}

// RESET
void reset_ball() {
    // todo: ball direction according to winner
    human.position.y = (screen.height/2.0f)-(human.paddle_h/2.0f);
    computer.position.y = (screen.height/2.0f)-(human.paddle_h/2.0f);
    ball.position.x = screen.width / 2.0f - ball.size/2.0f;
    ball.position.y = screen.height / 2.0f - ball.size/2.0f;
    ball.velocity = Vector2Zero();
    board.game_state = READY;
}

// SERVE
void serve_ball() {
    ball.speed_up = 8.0f;
    ball.velocity.x = ball.rand_arr[rand() % ball.rand_arr_size];
    ball.velocity.y = ball.rand_arr[rand() % ball.rand_arr_size];
}

void update_start_screen() {
    board.frame_counter++;
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        board.blink = true;
        PlaySound(sfx.start);
    }
    if (board.blink) {
        board.current_frame--;
    }
    if (board.current_frame < -60 ) {
        board.current_frame = 3;
        board.frame_counter = 0;
        board.game_state = START;
    }
}

void update_ball() {
    ball.position.x += ball.velocity.x * GetFrameTime() * ball.speed;
    ball.position.y += ball.velocity.y * GetFrameTime() * ball.speed;

    ball.center = (Vector2){ball.position.x + 12, ball.position.y + 9};
    // top
    if (ball.position.y <=board.font_size) {
        PlaySound(sfx.hit_wall);
        ball.position.y = board.font_size;
        ball.velocity.y *= -1;
    }
    // bottom
    if (ball.position.y >= screen.height-board.font_size-ball.size) {
        PlaySound(sfx.hit_wall);
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
        ball.hit_position = ball.position; // ?
        human.score++;
        PlaySound(sfx.hit_paddle);
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
        ball.hit_position = ball.position; // ?
        computer.score++;
        PlaySound(sfx.hit_paddle);
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
       if (ball.velocity.x > 0 && ball.position.x + (ball.radius/2.0f) > screen.width/2.0f ) {
           if (ball.position.y + (ball.radius/2.0f) != human.position.y + (human.paddle_h/2.0f)) {
               float timetilcol = ((screen.width-human.offset-human.paddle_w)-ball.position.x)/ball.velocity.x;
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
    human.position.y = Clamp(human.position.y,22,screen.height-human.paddle_h-22);
    return (Rectangle){human.position.x,human.position.y,human.paddle_w,human.paddle_h};
}

Rectangle update_computer_paddle() {
    if (ball.velocity.x < 0 && ball.position.x + (ball.radius/2.0f) < screen.width/2.0f ) {
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
    computer.position.y = Clamp(computer.position.y,22,screen.height-computer.paddle_h-22);
    return (Rectangle){computer.position.x,computer.position.y,computer.paddle_w,computer.paddle_h};
}

void update_gameplay() {
    if (ball.position.x <= -board.font_size*2) {
        human.score+=10;
        human.score = Clamp(human.score,0,9999);
        PlaySound(sfx.reset);
        reset_ball();
    }
    if (ball.position.x >= screen.width+board.font_size) {
        computer.score+=10;
        computer.score = Clamp(computer.score,0,9999);
        PlaySound(sfx.reset);
        reset_ball();
    }
}

// update: handles with [game start] countdown timer
void update_game_start() {
    board.frame_counter++;
    if ( ((board.frame_counter/60)%2) ) {
        board.current_frame--;
        PlaySound(sfx.count);
        board.frame_counter = 0;
    }
   if ( board.current_frame < 0 ) {
       PlaySound(sfx.count_last);
       serve_ball();
       board.frame_counter = 0;
       board.current_frame = 3;
       board.game_state = PLAY;
   }
}

// quick timer
void count_ready() {
    board.frame_counter++;
    if ( ((board.frame_counter/60)%2) ) {
        serve_ball();
        board.frame_counter = 0;
        board.game_state = PLAY;
    }
}

void draw_start_screen() {
    Vector2 pos = {(screen.width/2.0f)-(MeasureText("PRESS START",18)/2.0f),screen.height/2.0f};
    if (!board.blink) {
        if ( ((board.frame_counter/30)%2) ) {
            DrawTextEx(board.font,"PRESS START",pos,18,0,WHITE);
        }
    } else {
        if ( ((board.frame_counter/6)%2) ) {
            DrawTextEx(board.font,"PRESS START",pos,18,0,GREEN);
        }
    }
}

void draw_board() {
    // walls
    for (int i=0; i<2; i++) {
        for (int j=3;j<screen.width-26; j+=29) {
            DrawTextEx(board.font, "À",(Vector2){j,(screen.height-board.font_size)*i},board.font_size,0,RAYWHITE);
        }
    }
    // middle line
    for (int i=19; i<screen.height-38; i+=board.font_size) {
        DrawTextEx(board.font, ".",(Vector2){(screen.width/2.0f)-(board.font_size/2.0f-4),i},board.font_size,0,RAYWHITE);
    }
}

// draw: handles with [game start] countdown text
void draw_start_count() {
    float x = floor((board.font_size+board.frame_counter)/2.0f)-10;
    float y = floor((board.font_size+board.frame_counter)/2.0f)+10;
    Vector2 pos = {(screen.width/2.0f)-x, screen.height/2.0f-y};
    DrawRectangle(pos.x,pos.y,board.font_size+board.frame_counter,board.font_size+board.frame_counter,BLACK);
    DrawTextEx(board.font, TextFormat("%i",board.current_frame),pos,board.font_size+board.frame_counter,0,GREEN);
}

void draw_ball() {
    DrawTextEx(board.font, "Æ",ball.position,board.font_size,0,ball.color);
}
void draw_ball_blink() {
    // text blinking
    if (((board.frame_counter/10)%2)) {
        DrawTextEx(board.font,"Æ",ball.position,board.font_size,0,ball.color);
    }
}

void draw_human_paddle() {
    for (int i=0; i<human.paddle_h; i+=20) {
        DrawTextEx(board.font, "À", (Vector2){human.position.x,human.position.y+i},board.font_size,0,WHITE);
    }
}

void draw_computer_paddle() {
    for (int i=0; i<computer.paddle_h; i+=20) {
        DrawTextEx(board.font, "À", (Vector2){computer.position.x,computer.position.y+i},board.font_size,0,WHITE);
    }
}

// score --> computer & human
void draw_score() {
    DrawTextEx(board.font, TextFormat("Ì %04d",human.score),(Vector2){board.text_hx,board.text_y},board.font_size,0,board.text_col);
    DrawTextEx(board.font, TextFormat("Â %04d",computer.score),(Vector2){board.text_cx,board.text_y},board.font_size,0,board.text_col);
}
