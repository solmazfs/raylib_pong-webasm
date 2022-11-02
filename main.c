/*******************************************************************************************
*
*   raylib study [main.c] - Pong _ ver 0.9
*
*   Game originally created with raylib 4.2, last time updated with raylib 4.2
*
*   Game licensed under MIT.
*
*   Copyright (c) 2022 Fatih S. Solmaz (@solmazfs)
*
********************************************************************************************/

/*
* -resources:
*   - Font: PICO-8 --> https://www.lexaloffle.com/bbs/?tid=3760
*   - SFX: Generated with rFXGen by raylib
*   - ball bounce: https://gamedev.stackexchange.com/questions/4253/in-pong-how-do-you-calculate-the-balls-direction-when-it-bounces-off-the-paddl
*   - paddle ai: https://github.com/Ricket/richardcarter.org/blob/gh-pages/pong/pongai.js
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
    #define GLSL_VERSION 100
#else
    #define GLSL_VERSION 330
#endif

/* -- TODO:
 *  - rotate ball direction after velocitywanted
 *  - ball trail after smash
 *  - ai helper for players
 *      - after ai helper active player shadow can help work like ai enable
 *      - ai helper will control player shadow
 *  - add wall shadow (+)
 *  - add glow?
 *  - enum for abilities?
 *  - settings menu
 *      - theme
 *      - music on off - bg music | effect
 *      - ai enable
 *  - !fix collision issue
 *      - ball jumps off racket when the speed is high
 *      - sometimes ball hits two or more? times
 *      - score system is buggy because of these issues
*/

#define _WINDOW_W 640
#define _WINDOW_H 360
#define MAX_SCORE 99999

typedef struct Screen {
    int canvas_width,canvas_height;
    float time, time_value;
    bool skip_intro;
    Camera2D camera;
    RenderTexture2D target;
    Texture2D logo_raylib;
    Rectangle source,dest;
    Shader shader;
} Screen;

typedef struct Board {
    int wall_w;
    int font_size;
    bool blink, ai_status;
    Font font;
    Color score_text_color,shadow_color;
    Vector2 human_score_text,computer_score_text;
    Rectangle wall_top,wall_bottom;
    struct Sfx {
        Sound logo_intro,logo_intro_final,start,count,count_last,hit_wall,hit_paddle, hit_paddle_smash,hit_paddle_smash_back,reset;
    } sfx;
    struct Timer {int frame_counter,current_frame,count_timer,blink_timer;} timer;
} Board;

typedef struct Ball {
    int radius;
    float speed,min_speed,max_speed,corner_speed,smash_speed;
    bool reset;
    Vector2 velocity, position, direction;
    Color color;
} Ball;

typedef struct Paddle {
    int score;
    int paddle_width,paddle_height;
    float speed, max_speed;
    bool corner_hit,enable_ai,smash;
    Vector2 orig_pos;
    Vector2 position;
    Vector2 velocity;
    Rectangle rec;
    Color color;
    struct Helper {Vector2 position; Rectangle rec; Color color;} helper;
} Paddle;

typedef enum GameScreen { LOGO = 0, TITLE, START, GAMEPLAY, RESET, ENDING } GameScreen;

typedef struct Context {
    Screen screen;
    GameScreen current_screen;
    Board board;
    Paddle human;
    Paddle computer;
    Ball ball;
} Context;


void LoadResources(Screen *screen, Board *board) {
    // shader
    screen->shader = LoadShader(0, TextFormat("resources/shaders/crt%i.frag",GLSL_VERSION));
    // texture
    screen->logo_raylib = LoadTexture("resources/raylib_logo.png");
    // font
    board->font = LoadFontEx("resources/fonts/PICO-8_wide-upper.ttf",18,0,256);
    GenTextureMipmaps(&board->font.texture);
    SetTextureFilter(board->font.texture, TEXTURE_FILTER_BILINEAR);
    // audio
    board->sfx.logo_intro = LoadSound("resources/sfx/logo_intro.wav");
    board->sfx.logo_intro_final = LoadSound("resources/sfx/logo_intro_final.wav");
    board->sfx.start = LoadSound("resources/sfx/start.wav");
    board->sfx.count = LoadSound("resources/sfx/count.wav");
    board->sfx.count_last = LoadSound("resources/sfx/count_last.wav");
    board->sfx.hit_wall = LoadSound("resources/sfx/hit_wall.wav");
    board->sfx.hit_paddle = LoadSound("resources/sfx/hit_paddle.wav");
    board->sfx.hit_paddle_smash = LoadSound("resources/sfx/hit_paddle_smash.wav");
    board->sfx.hit_paddle_smash_back = LoadSound("resources/sfx/hit_paddle_smash_back.wav");
    board->sfx.reset = LoadSound("resources/sfx/reset.wav");
}

Vector2 move_ball(Screen *screen, Board *board, Ball *ball, Paddle *human, Paddle *computer);
Vector2 move_human_paddle(Screen *screen, Board *board, Paddle *human, Ball *ball);
Vector2 move_computer_paddle(Screen *screen, Board *board, Paddle *human, Ball *ball);
void draw_logo(Screen *screen, Board *board);
void draw_title(Screen *screen, Board *board);
void draw_board(Screen *screen, Board *board);
void draw_ai_status(Board *board, Paddle *human);
void draw_smash_status(Screen *screen, Board *board, Paddle *human, Paddle *computer);
void draw_ball(Board *board, Ball *ball);
void draw_human_paddle(Board *board, Paddle *human);
void draw_computer_paddle(Board *board, Paddle *computer);
void draw_score(Board *board, Paddle *human, Paddle *computer);
void UpdateDrawFrame(Screen*, GameScreen*, Board*, Paddle*, Paddle*, Ball*);
void UpdateWeb(Context *arg);

Vector2 random_angle() {
    // getting -1 | 1 to the up or bottom then random angle between 45/95 degree
    // passing them an array
    Vector2 result = {0};
    int rand = GetRandomValue(0,1) * 2 - 1;
    int rand_angle = GetRandomValue(45,95);
    result.x = sinf(rand*(rand_angle*PI/180)) * 0.8;
    result.y = cosf(rand*(rand_angle*PI/180)) * 0.8;
    return result;
}

int generate_rand() {
    int x = GetRandomValue(0,1);
    int y = GetRandomValue(0,1);
    return ((x << 1) ^ y) == 0;
}

int main() {
    SetRandomSeed(time(NULL));
    SetWindowState(FLAG_VSYNC_HINT);
    InitWindow(_WINDOW_W,_WINDOW_H,"PONG - Smash!");
    InitAudioDevice();

    // Screen
    Screen screen = {0};
    screen.canvas_width = GetScreenWidth();
    screen.canvas_height = GetScreenHeight();
    screen.target = LoadRenderTexture(screen.canvas_width,screen.canvas_height);
    screen.source = (Rectangle){0,0,screen.target.texture.width,-screen.target.texture.height};
    screen.dest = (Rectangle){0,0,_WINDOW_W,_WINDOW_H};
    screen.camera = (Camera2D){0};
    screen.camera.offset = (Vector2){screen.canvas_width/2.0f,screen.canvas_height/2.0f};
    screen.camera.target = screen.camera.offset;
    screen.camera.zoom = 1.0f;

    // Board
    Board board = {0};
    LoadResources(&screen, &board);
    board.ai_status = false;
    board.timer.frame_counter = 0;
    board.timer.current_frame = 0;
    board.timer.count_timer = 0;
    board.timer.blink_timer = 0;
    board.font_size = board.font.baseSize;
    board.wall_w = board.font_size;
    board.wall_top = (Rectangle){0,0,screen.canvas_width,board.wall_w};
    board.wall_bottom = (Rectangle){0,screen.canvas_height-board.wall_w,screen.canvas_width,board.wall_w};
    board.score_text_color = LIGHTGRAY;
    board.shadow_color = GetColor(0x0000FF24);
    board.human_score_text = (Vector2){(screen.canvas_width/2.0f)+18,28.0f};
    board.computer_score_text = (Vector2){(screen.canvas_width/2.0f)-((MeasureTextEx(board.font,"À 99999",board.font_size,0).x)+18),28.0f};
    // Ball
    Ball ball = {0};
    ball.reset = false;
    ball.radius = board.font_size/2.0f;
    ball.min_speed = 480.0f;
    ball.max_speed = 730.0f;
    ball.corner_speed = 1.0f;
    ball.speed = ball.min_speed;
    ball.smash_speed = 1.0f;
    ball.color = WHITE;
    ball.direction = random_angle();
    ball.velocity = ball.direction;
    ball.position = (Vector2){screen.canvas_width/2.0f,screen.canvas_height/2.0f};
    // Human
    Paddle human = {0};
    human.score = 0;
    human.enable_ai = false;
    human.speed = 1030.0f;
    human.max_speed = (float)1000/1000;
    human.paddle_width = board.font_size+8;
    human.paddle_height = 80;
    human.velocity = Vector2Zero();
    human.orig_pos = (Vector2){(screen.canvas_width)-(human.paddle_width*2),(screen.canvas_height/2.0f)-(human.paddle_height/2.0f)};
    human.position = human.orig_pos;
    human.rec = (Rectangle){human.position.x,human.position.y,human.paddle_width,human.paddle_height};
    human.helper.position = human.position;
    human.helper.rec = human.rec;
    human.helper.color = GetColor(0xC724B121);
    // Computer
    Paddle computer = {1};
    computer.score = 0;
    computer.enable_ai = true;
    computer.speed = 1030.0f;//630.0f;
    computer.max_speed = (float)1000/1000;
    computer.paddle_width = board.font_size+8;
    computer.paddle_height = 80;
    computer.velocity = Vector2Zero();
    computer.orig_pos = (Vector2){computer.paddle_width,(screen.canvas_height/2.0f)-(computer.paddle_height/2.0f)};
    computer.position = computer.orig_pos;
    computer.rec = (Rectangle){computer.position.x,computer.position.y,computer.paddle_width,computer.paddle_height};
    computer.helper.position = computer.position;
    computer.helper.rec = human.rec;
    computer.helper.color = GetColor(0xC724B121);
    // screen shader
    float screen_size[2] = {screen.canvas_width,screen.canvas_height};
    SetShaderValue(screen.shader, GetShaderLocation(screen.shader, "resolution"), &screen_size, SHADER_UNIFORM_VEC2);
    screen.time = GetShaderLocation(screen.shader, "time");
    screen.time_value = 0;

    GameScreen current_screen = LOGO;

    Context ctx = {0};
    ctx.screen = screen;
    ctx.current_screen = current_screen;
    ctx.board = board;
    ctx.human = human;
    ctx.computer = computer;
    ctx.ball = ball;

    //void (*Update)(Screen*, GameScreen*, Board*, Paddle*, Paddle*, Ball*) = {UpdateDrawFrame};

    #if defined(PLATFORM_WEB)
        emscripten_set_main_loop_arg((void *)UpdateWeb, &ctx, 0, 1);
    #else
    SetWindowPosition(0,0);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateDrawFrame(&screen, &current_screen, &board, &human,&computer,&ball);
    }
    #endif
    UnloadRenderTexture(screen.target);
    UnloadShader(screen.shader);
    UnloadFont(board.font);
    StopSoundMulti();
    UnloadSound(board.sfx.logo_intro);
    UnloadSound(board.sfx.logo_intro_final);
    UnloadSound(board.sfx.start);
    UnloadSound(board.sfx.count);
    UnloadSound(board.sfx.count_last);
    UnloadSound(board.sfx.hit_wall);
    UnloadSound(board.sfx.hit_paddle);
    UnloadSound(board.sfx.hit_paddle_smash);
    UnloadSound(board.sfx.hit_paddle_smash_back);
    UnloadSound(board.sfx.reset);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

// web main loop - emscripten
void UpdateWeb(Context *arg) {
    UpdateDrawFrame(&arg->screen,&arg->current_screen,&arg->board,&arg->human,&arg->computer,&arg->ball);
}

void UpdateDrawFrame(Screen *screen, GameScreen *current_screen, Board *board, Paddle *human, Paddle *computer, Ball *ball) {
    screen->time_value = (float)GetTime();
    SetShaderValue(screen->shader,screen->time,&screen->time_value, SHADER_UNIFORM_FLOAT);

    // UPDATE
    switch(*current_screen) {
        case LOGO:
            {
                board->timer.frame_counter++;
                if (board->timer.frame_counter == 1) {
                    PlaySound(board->sfx.logo_intro);
                }
                if (IsKeyPressed(KEY_SPACE) && board->timer.frame_counter > 1) {
                    board->timer.frame_counter = 80;
                    screen->skip_intro = true;
                }
                if (board->timer.frame_counter == 80) {
                    PlaySound(board->sfx.logo_intro_final);
                }
                if (screen->skip_intro && board->timer.frame_counter > 99) {
                    board->timer.frame_counter = 0;
                    screen->skip_intro = false;
                    *current_screen = TITLE;
                }
                if (!screen->skip_intro && (board->timer.frame_counter/120)%2) {
                    board->timer.frame_counter = 0;
                    *current_screen = TITLE;
                }
            }break;
        case TITLE:
            {
                board->timer.frame_counter++;
                if (!board->blink && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))) {
                    PlaySound(board->sfx.start);
                    board->blink = true;
                }
                if (board->blink) {
                    board->timer.current_frame--;
                }
                if (board->timer.current_frame < -60 ) {
                    board->blink = false;
                    board->timer.frame_counter = 0;
                    board->timer.current_frame = 3;
                    *current_screen = START;
                }
            }break;
        case START:
            {
                board->timer.frame_counter++;
                if ( ((board->timer.frame_counter/60)%2) ) {
                    PlaySound(board->sfx.count);
                    board->timer.current_frame--;
                    board->timer.frame_counter = 0;
                }
                if ( board->timer.current_frame < 0 ) {
                    PlaySound(board->sfx.count_last);
                    ball->velocity = ball->direction;
                    board->timer.frame_counter = 0;
                    board->timer.current_frame = 3;
                    *current_screen = GAMEPLAY;
                }
            }break;
        case GAMEPLAY:
            {
                // DEBUG --> control camera
                //screen->camera.target = ball->position;
                //screen->camera.offset = (Vector2){screen->canvas_width/2.0f,screen->canvas_height/2.0f};
                //if (IsKeyPressed(KEY_Q)) {
                //    screen->camera.zoom -= 0.6f;
                //}
                //if (IsKeyPressed(KEY_E)) {
                //    screen->camera.zoom += 0.6f;
                //}
                // !code order necessary
                ball->position = move_ball(screen, board, ball,human,computer);
                if (board->ai_status) board->timer.frame_counter++;
                if ((board->timer.frame_counter/30)%2) {
                    board->timer.frame_counter = 0;
                    board->ai_status = false;
                }
                human->position = move_human_paddle(screen, board, human, ball);
                computer->position = move_computer_paddle(screen, board, computer, ball);

                if (ball->position.x < 0 ) {
                    PlaySound(board->sfx.reset);
                    human->score += 10;
                    human->score = Clamp(human->score,0,MAX_SCORE);
                    ball->direction.x = -fabs(random_angle().x);
                    ball->velocity = Vector2Zero();
                    ball->position = (Vector2){screen->canvas_width/2.0f,screen->canvas_height/2.0f};
                    human->velocity = Vector2Zero();
                    computer->velocity = Vector2Zero();
                    *current_screen = RESET;
                }
                if (ball->position.x > screen->canvas_width ) {
                    PlaySound(board->sfx.reset);
                    computer->score += 10;
                    computer->score = Clamp(computer->score,0,MAX_SCORE);
                    ball->direction.x = fabs(random_angle().x);
                    ball->velocity = Vector2Zero();
                    ball->position = (Vector2){screen->canvas_width/2.0f,screen->canvas_height/2.0f};
                    human->velocity = Vector2Zero();
                    computer->velocity = Vector2Zero();
                    *current_screen = RESET;
                }

            }break;
        case RESET:
            {
                board->timer.blink_timer++;
                human->position.x = Lerp(human->position.x,human->orig_pos.x,0.1);
                human->position.y = Lerp(human->position.y,human->orig_pos.y,0.1);
                computer->position.x = Lerp(computer->position.x,computer->orig_pos.x,0.1);
                computer->position.y = Lerp(computer->position.y,computer->orig_pos.y,0.1);
                if ((board->timer.blink_timer/80)%2) {
                    board->timer.blink_timer = 0;
                    board->timer.frame_counter = 0; // remove after debug
                    ball->direction.y = random_angle().y;
                    ball->velocity = ball->direction;
                    ball->speed = ball->min_speed;
                    ball->corner_speed = 1.0f;
                    ball->smash_speed = 1.0f;
                    human->corner_hit = false;
                    computer->corner_hit = false;
                    human->smash = false;
                    computer->smash = false;
                    *current_screen = GAMEPLAY;
                }
            }break;
        case ENDING:
            {printf("ENDING SCREEN\n");}break;
        default: break;
    }
    // DRAW
    BeginTextureMode(screen->target);
        ClearBackground(DARKGRAY);
        //DrawFPS(40,40);
        switch(*current_screen) {
            case LOGO:
                {
                    draw_logo(screen,board);
                }break;
            case TITLE:
                {
                    draw_title(screen,board);
                }break;
            case START:
                {
                    draw_board(screen,board);
                    draw_score(board,human,computer);
                    draw_human_paddle(board,human);
                    draw_computer_paddle(board,computer);
                    float x = floor((board->font_size+board->timer.frame_counter)/2.0f)-10;
                    float y = floor((board->font_size+board->timer.frame_counter)/2.0f)+10;
                    Vector2 pos = {(screen->canvas_width/2.0f)-x, screen->canvas_height/2.0f-y};
                    DrawRectangle(pos.x,pos.y,board->font_size+board->timer.frame_counter,board->font_size+board->timer.frame_counter,DARKGRAY);
                    DrawTextEx(board->font, TextFormat("%i",board->timer.current_frame),pos,board->font_size+board->timer.frame_counter,0,LIGHTGRAY);
                }break;
            case GAMEPLAY:
                {
                    draw_board(screen, board);
                    // INFO --> AI Status
                    draw_ai_status(board, human);
                    draw_smash_status(screen,board,human,computer);
                    // human
                    draw_human_paddle(board, human);
                    // comp
                    draw_computer_paddle(board, computer);
                    // ball
                    draw_ball(board, ball);
                    draw_score(board, human, computer);
                }break;
            case RESET:
                {
                    draw_board(screen, board);
                    draw_ai_status(board, human);
                    draw_human_paddle(board, human);
                    draw_computer_paddle(board, computer);
                    draw_score(board, human, computer);
                    draw_ball(board, ball);
                }break;
            case ENDING:
                {printf("ENDING SCREEN\n");}break;
            default: break;
        }
    EndTextureMode();

    BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(screen->camera);
            BeginShaderMode(screen->shader);
                DrawTexturePro(screen->target.texture,screen->source,screen->dest,Vector2Zero(),0,WHITE);
            EndShaderMode();
        EndMode2D();
    EndDrawing();
}

// UPDATE
Vector2 move_ball(Screen *screen, Board *board, Ball *ball, Paddle *human, Paddle *computer) {
    ball->position.x += (ball->velocity.x * GetFrameTime() * (ball->speed * ball->smash_speed)) * ball->corner_speed;
    ball->position.y += (ball->velocity.y * GetFrameTime() * (ball->speed * ball->smash_speed)) * ball->corner_speed;
    ball->speed = Clamp(ball->speed,ball->min_speed,ball->max_speed);
    // DEBUG
    //if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    //    ball->position = GetMousePosition();
    //    if (GetMousePosition().y > human->position.y + human->paddle_height/2.0f) {
    //        ball->velocity.x = 0;
    //        ball->velocity.y = -1;
    //    } else {
    //        ball->velocity.x = 0;
    //        ball->velocity.y = 1;

    //    }
    //}

    // human
    if (CheckCollisionCircleRec(ball->position,ball->radius,human->rec)) {
        ball->speed *= 1.03f; /* slowly increasing ball speed */
        if (!human->corner_hit) {
            human->score++;
            human->position.x += 6.0f; /* knokback */
        } 
        // smash
        if (human->enable_ai && generate_rand()) human->smash = true;
        if (human->smash && !computer->smash) {
            PlaySoundMulti(board->sfx.hit_paddle_smash);
            ball->smash_speed = GetRandomValue(35,45) * (PI/180) * 2.1f;
            computer->smash = false;
        } else if (computer->smash && ball->smash_speed > 1.0f) {
            // hit back smash hit comes from computer
            PlaySoundMulti(board->sfx.hit_paddle_smash_back);
            human->score += 3; /* total 4 */
            ball->smash_speed = GetRandomValue(25,35) * (PI/180) * 1.6;
            computer->smash = false;
        } else {
            PlaySoundMulti(board->sfx.hit_paddle);
            ball->smash_speed = 1.0f;
        }

        float a = (human->position.y+(human->paddle_height/2.0f)) - (ball->position.y - ball->radius/2.0f);
        float b = (a/(human->paddle_height/2.0f));
        float c = (b * (45*PI/180));
        if (ball->position.x > human->position.x) {
            human->corner_hit = true;
            ball->corner_speed = 3.0f;
            if (ball->position.y < human->position.y + human->paddle_height/2.0f) {
                ball->velocity = (Vector2){0.3,-0.3};
            } else {ball->velocity = (Vector2){0.3,0.3};}
        } else {
            ball->velocity.x = -cos(c);
            ball->velocity.y = -sin(c);
        }
    }

    // computer
    if (CheckCollisionCircleRec(ball->position,ball->radius,computer->rec)) {
        ball->speed *= 1.03f; /* slowly incr ball spd */
        if (!computer->corner_hit) {
            computer->score++;
            computer->position.x -= 6.0f; /* knockback */
        }
        // smash
        if (generate_rand() && !human->smash) {
            PlaySoundMulti(board->sfx.hit_paddle_smash);
            ball->smash_speed = GetRandomValue(35,45) * (PI/180) * 2.1f; //1.6f;
            computer->smash = true;
        } else if (human->smash && ball->smash_speed > 1.0f) {
            // hit back smash comes from human
            PlaySoundMulti(board->sfx.hit_paddle_smash_back);
            human->score += 3; /* total 4 */
            ball->smash_speed = GetRandomValue(25,35) * (PI/180) * 1.6f;
            human->smash = false;
        } else {
            PlaySoundMulti(board->sfx.hit_paddle);
            ball->smash_speed = 1.0f;
        }
        // collision logic very challenging here, i've tryed my best
        float a = (computer->position.y+(computer->paddle_height/2.0f)) - (ball->position.y+ball->radius/2.0f);
        float b = (a/(computer->paddle_height/2.0f));
        float c = (b * (45*PI/180) );
        if (ball->position.x < computer->position.x + computer->paddle_width) {
            computer->corner_hit = true;
            ball->corner_speed = 3.0f;
            if (ball->position.y < computer->position.y + computer->paddle_height/2.0f) {
                ball->velocity = (Vector2){-0.3,-0.3};
            } else {ball->velocity = (Vector2){-0.3,0.3};}
        } else {
            ball->velocity.x = cos(c);
            ball->velocity.y = -sin(c);
        }
    }
    if (ball->position.y < board->wall_w+ball->radius) {
        PlaySound(board->sfx.hit_wall);
        Vector2 top = (Vector2){board->wall_top.x,board->wall_top.y+board->font_size};
        Vector2 final_t = Vector2Reflect(ball->velocity,Vector2Normalize(top));
        ball->velocity.x = final_t.x;
        ball->velocity.y = fabs(final_t.y);
    }
    if (ball->position.y > screen->canvas_height-(board->wall_w+ball->radius)) {
        PlaySound(board->sfx.hit_wall);
        Vector2 bottom = (Vector2){board->wall_bottom.x,board->wall_bottom.y};
        Vector2 final_b = Vector2Reflect(ball->velocity,Vector2Normalize(bottom));
        ball->velocity.x = final_b.x;
        ball->velocity.y = -fabs(final_b.y);
    }
    return ball->position;
}

Vector2 move_human_paddle(Screen *screen, Board *board, Paddle *human, Ball *ball) {
    if (IsKeyPressed(KEY_P) && !board->ai_status) {
        board->ai_status = true;
        human->enable_ai = !human->enable_ai;
    }
    if ((IsKeyDown(KEY_UP) || IsKeyDown(KEY_RIGHT)) && !human->enable_ai && !human->corner_hit) {
        human->velocity.y = -Lerp(0,4,0.16);
    }
    if ((IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT)) && !human->enable_ai && !human->corner_hit) {
        human->velocity.y = Lerp(0,4,0.16);
    }
    if (IsKeyDown(KEY_LEFT_SHIFT) && !human->corner_hit) {
        // TODO : with ai help?
        human->velocity.y = Lerp(human->velocity.y,0,0.8);
    }
    if (IsKeyPressed(KEY_SPACE) && !human->smash && !human->corner_hit) {
        human->smash = true;
    }
    if (human->corner_hit) {human->velocity = Vector2Zero();}
    if (human->enable_ai) {
        // check if the ball has crossed the line
        if (!human->corner_hit && ball->velocity.x > 0 && ball->position.x > screen->canvas_width/2.0f ) {
            // check if the y-position of the ball is not in the middle of the paddle
            if (ball->position.y != human->position.y + (human->paddle_height/2.0f)) {
               float timetilcol = ((screen->canvas_width-human->paddle_width)-ball->position.x)/ball->velocity.x;
               float distancewanted = (human->position.y+(human->paddle_height/2.0f)) - (ball->position.y);
               float velocitywanted = -distancewanted/timetilcol;
               if (velocitywanted > human->max_speed) {
                   // TODO --> rotate ball?
                   human->velocity.y = human->max_speed;
               } else if (velocitywanted < -human->max_speed) {
                   human->velocity.y = -human->max_speed;
               } else {human->velocity.y = velocitywanted;}
            } else {human->velocity.y = 0;}
        } else {human->velocity = Vector2Zero();}
    }
    // TODO : ai helper --> beneficial usage also speed?
    human->velocity.y = Lerp(human->velocity.y,0,0.2);
    human->position.x = Lerp(human->position.x, human->orig_pos.x,0.2);
    human->position.y += human->velocity.y * human->speed * GetFrameTime();
    human->position.y = Clamp(human->position.y,board->font_size+2,screen->canvas_height-(human->paddle_height+board->font_size));
    human->helper.position = Vector2Lerp(human->helper.position,human->position,0.3f);
    human->helper.position.x += 3;
    human->rec = (Rectangle){human->position.x,human->position.y,human->paddle_width,human->paddle_height};
    human->helper.rec = (Rectangle){human->helper.position.x,human->helper.position.y,human->paddle_width,human->paddle_height};
    return human->position;
}

Vector2 move_computer_paddle(Screen *screen, Board *board, Paddle *computer, Ball *ball) {
    if (!computer->corner_hit && ball->velocity.x < 0 && ball->position.x < screen->canvas_width/2.0f ) {
        if (ball->position.y != computer->position.y + (computer->paddle_height/2.0f)) {
            float timetilcol = ((computer->paddle_width)-ball->position.x)/ball->velocity.x;
            float distancewanted = (computer->position.y+(computer->paddle_height/2.0f)) - (ball->position.y);
            float velocitywanted = -distancewanted/timetilcol;
            if (velocitywanted > computer->max_speed) {
                computer->velocity.y = computer->max_speed;
            } else if (velocitywanted < -computer->max_speed) {
                computer->velocity.y = -computer->max_speed;
            } else {
                computer->velocity.y = velocitywanted;
            }
        } else { computer->velocity.y = 0; }
    } else {computer->velocity = Vector2Zero();}
    computer->position.x = Lerp(computer->position.x, computer->orig_pos.x,0.2);
    computer->velocity.y = Lerp(computer->velocity.y,0,0.2);
    computer->position.y += computer->velocity.y * computer->speed * GetFrameTime();
    computer->position.y = Clamp(computer->position.y,board->font_size+2,screen->canvas_height-(computer->paddle_height+computer->paddle_width));
    computer->helper.position = Vector2Lerp(computer->helper.position,computer->position,0.3f);
    computer->helper.position.x -= 3;
    computer->rec = (Rectangle){computer->position.x,computer->position.y,computer->paddle_width,computer->paddle_height};
    computer->helper.rec = (Rectangle){computer->helper.position.x,computer->helper.position.y,computer->paddle_width,computer->paddle_height};
    return computer->position;
}

// DRAW
void draw_logo (Screen *screen, Board *board) {
    // simple - experiment
    float x = screen->canvas_width/2.0f - screen->logo_raylib.width/2.0f;
    float y = screen->canvas_height/2.0f - screen->logo_raylib.height/2.0f;
    float rect_y = screen->logo_raylib.height/2.0f;
    float text_x = x;
    float text_y = y - 16;
    if (board->timer.frame_counter < 60) {
        DrawTexture(screen->logo_raylib, x+sin(board->timer.frame_counter/1.3)*0.9, y, DARKGRAY);
        DrawRectangle(x-4,(y+rect_y-board->timer.frame_counter)+2,screen->logo_raylib.width+8,(rect_y-board->timer.frame_counter)+4,DARKGRAY);
    } else {
        DrawTexture(screen->logo_raylib, x, y, WHITE);
        DrawTextEx(board->font, "POWERED BY",(Vector2){text_x,text_y},8,0,BLACK);
    }
}

void draw_title(Screen *screen, Board *board) {
    Vector2 title_pos1 = {(screen->canvas_width/2.0f)-(MeasureText("PONG",86)/2.0f),90};
    Vector2 title_pos2 = {(screen->canvas_width/2.0f)+(MeasureText("SMASH!",26)/2.5f),90+96};
    Vector2 launch_pos = {(screen->canvas_width/2.0f)-(MeasureText("PRESS START",board->font_size)/2.0f),screen->canvas_height/1.5f};
    DrawTextEx(board->font, "PONG",title_pos1,86,0,YELLOW);
    DrawTextEx(board->font, "SMASH!",title_pos2,26,0,MAGENTA);
    DrawLineEx((Vector2){title_pos1.x,title_pos2.y +13},(Vector2){title_pos2.x-12,title_pos2.y + 13},8,WHITE);
    if (!board->blink) {
        if ( ((board->timer.frame_counter/30)%2) ) {
            DrawTextEx(board->font,"PRESS START",launch_pos,board->font_size,0,WHITE);
        }
    } else {
        if ( ((board->timer.frame_counter/6)%2) ) {
            DrawTextEx(board->font,"PRESS START",launch_pos,board->font_size,0,WHITE);
        }
    }
}

void draw_board(Screen *screen, Board *board) {
    float y = (screen->canvas_height-board->font_size);
    // walls
    DrawRectangle(0,y*0+4,screen->canvas_width,board->font_size,board->shadow_color);
    DrawRectangle(0,y-4,screen->canvas_width,board->font_size,board->shadow_color);
    for (int i=0; i<2; i++) {
        for (int j=3;j<screen->canvas_width-26; j+=29) {
            DrawTextEx(board->font, "À",(Vector2){j,(screen->canvas_height-board->font_size)*i},board->font_size,0,RAYWHITE);
        }
    }
    // middle line
    for (int i=19; i<screen->canvas_height-38; i+=board->font_size) {
        DrawTextEx(board->font, ".",(Vector2){(screen->canvas_width/2.0f)-(board->font_size/2.0f-4),i},board->font_size,0,RAYWHITE);
    }
}

void draw_ai_status(Board *board, Paddle *human) {
    if (board->ai_status) {
        if (human->enable_ai) {
            DrawTextEx(board->font, "ENABLED", (Vector2){20,40},board->font_size,0,GREEN);
        } else {DrawTextEx(board->font, "DISABLED", (Vector2){20,40},board->font_size,0,GREEN);}
    }
}

void draw_smash_status(Screen *screen, Board *board, Paddle *human, Paddle *computer) {
    if (human->smash | computer->smash) {
        float x = (screen->canvas_width/2.0f) - (MeasureText("SMASH!",board->font_size)/2.0f);
        float y = screen->canvas_height-60;
        DrawTextEx(board->font, "SMASH!", (Vector2){x,y},board->font_size,0,GREEN);
    }
}

void draw_ball(Board *board, Ball *ball) {
    Vector2 center = (Vector2){ball->position.x-ball->radius-4,ball->position.y-(ball->radius)};
    if ( ((board->timer.blink_timer/10)%2) ) {
        DrawTextEx(board->font, "Æ",center,board->font_size,0,ball->color);
    }
    if (board->timer.blink_timer <= 1) {
        DrawTextEx(board->font, "Æ",center,board->font_size,0,ball->color);
    }
}

void draw_human_paddle(Board *board, Paddle *human) {
    human->color = WHITE;
    if (human->smash) human->color = MAGENTA;
    DrawRectangleRec(human->helper.rec,human->helper.color);
    for (int i=0; i<human->paddle_height; i+=20) {
        DrawTextEx(board->font, "À", (Vector2){human->position.x,human->position.y+i},board->font_size,0,human->color);
    }
}

void draw_computer_paddle(Board *board, Paddle *computer) {
    computer->color = WHITE;
    if (computer->smash) computer->color = MAGENTA;
    DrawRectangleRec(computer->helper.rec,computer->helper.color);
    for (int i=0; i<computer->paddle_height; i+=20) {
        DrawTextEx(board->font, "À", (Vector2){computer->position.x,computer->position.y+i},board->font_size,0,computer->color);
    }
}

void draw_score(Board *board, Paddle *human, Paddle *computer) {
    DrawTextEx(board->font, TextFormat("Ì %05d",human->score),board->human_score_text,board->font_size,0,board->score_text_color);
    DrawTextEx(board->font, TextFormat("Â %05d",computer->score),board->computer_score_text,board->font_size,0,board->score_text_color);
}
