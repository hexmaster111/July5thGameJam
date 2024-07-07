
/*******************************************************************************************
 *
 *   raylib [core] example - 2D Camera platformer
 *
 *   Example originally created with raylib 2.5, last time updated with raylib 3.0
 *
 *   Example contributed by arvyy (@arvyy) and reviewed by Ramon Santamaria (@raysan5)
 *
 *   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
 *   BSD-like license that allows static linking with closed source software
 *
 *   Copyright (c) 2019-2024 arvyy (@arvyy)
 *
 ********************************************************************************************/

#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#define G 800
#define PLAYER_JUMP_SPD 450.0f
#define PLAYER_HOR_SPD 200.0f

enum
{
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};

typedef struct Player
{
    Vector2 position;
    float speed;
    bool canJump;

    int direction;
    int anamationIdx;
    int anamationTime;
} Player;

typedef struct EnvItem
{
    Rectangle rect;
    int blocking;
    Color color;
    int textureId;
    int gravity;

    // process vars for things -- dont set in ctor
    float currFallSpeed;
} EnvItem;

//----------------------------------------------------------------------------------
// Module functions declaration
//----------------------------------------------------------------------------------
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta);
void UpdateWorld(EnvItem *envItems, int envItemsLength, float delta);
void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "game");
    Texture2D tilesTexture = LoadTexture("Tiles-and-EnemiesT.png");

    printf("tiles w %d\n", tilesTexture.width / 8);

    Texture2D playerTexture = LoadTexture("PlayerT.png");

    Player player = {0};
    player.position = (Vector2){400, 280};
    player.speed = 0;
    player.canJump = false;
    player.direction = DIRECTION_RIGHT;

// Convert tile tiles to px
#define TW(x) \
    (x * 8)
// xy to flat index, 26 tiles per col
#define TSS(x, y) ((x) + ((y) * 26))
    // clang-format off

    /*
     * Texture
     *   -1 : use color 
     * 
     * Gravity
     *   -1 : solid
    */

    EnvItem envItems[] = {
        /*x      y  width   height    SOLID       COLOR       TEXTURE   GRAVITY*/
        {{0,     0, TW(125), TW(50)},     0, {27,24,24,255},        -1, -1},
        {{0,   400, TW(125), TW(25)},     1,           GRAY, TSS(0,16), -1},
        {{300, 200,  TW(50),  TW(2)},     1,           GRAY, TSS(2, 2), -1},
        {{250, 300,  TW(12),  TW(2)},     1,           GRAY,         2, -1},
        {{850, 100,  TW(37),  TW(2)},     1,           GRAY,         2, -1},
        {{650, 300,  TW(12),  TW(2)},     1,           GRAY,         2, -1},
        {{500, 300,   TW(2),  TW(2)},     0,         YELLOW, TSS(7,11),  1}
        
    };
    // clang-format on

#undef TSS
#undef TW

    int envItemsLength = sizeof(envItems) / sizeof(envItems[0]);

    for (size_t i = 0; i < envItemsLength; i++)
    {
        envItems[i].currFallSpeed = 0;
    }

    Camera2D camera = {0};
    camera.target = player.position;
    camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    bool hitboxdebug = true;

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        float deltaTime = GetFrameTime();

        UpdatePlayer(&player, envItems, envItemsLength, deltaTime);
        UpdateWorld(&envItems, envItemsLength, deltaTime);

        camera.zoom += ((float)GetMouseWheelMove() * 0.05f);

        if (camera.zoom > 3.0f)
            camera.zoom = 3.0f;
        else if (camera.zoom < 0.25f)
            camera.zoom = 0.25f;

        if (IsKeyPressed(KEY_R))
        {
            camera.zoom = 1.0f;
            player.position = (Vector2){400, 280};
            envItems[6].rect.y = 300;
            envItems[6].currFallSpeed = 0;
        }

        UpdateCameraPlayerBoundsPush(&camera, &player, envItems,
                                     envItemsLength, deltaTime, screenWidth, screenHeight);

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(LIGHTGRAY);

        BeginMode2D(camera);

        for (int i = 0; i < envItemsLength; i++)
        {
            if (envItems[i].textureId == -1)
                DrawRectangleRec(envItems[i].rect, envItems[i].color);
            else
            {
                int tileSheetSize = 8;
                Rectangle src = {0, 0, tileSheetSize, tileSheetSize};
                const int sprites_per_row = 26;

                int row = envItems[i].textureId / sprites_per_row;
                int col = envItems[i].textureId % sprites_per_row;

                src.x = col * tileSheetSize;
                src.y = row * tileSheetSize;

                int tileSize = 16;
                int tilesWide = envItems[i].rect.width / tileSize;

                Rectangle drawingPos = {0, envItems[i].rect.y, tileSize, tileSize};

                for (size_t tw = 0; tw < tilesWide; tw++)
                {
                    drawingPos.x = tw * tileSize + envItems[i].rect.x;

                    DrawTexturePro(tilesTexture, src, drawingPos, (Vector2){0, 0}, 0, WHITE);
                }

                if (hitboxdebug)
                    DrawRectangleRec(envItems[i].rect, envItems[i].color);
            }
        }

        Rectangle playerRect = {player.position.x - 20, player.position.y - 40, 40.0f, 40.0f};
        // DrawRectangleRec(playerRect, RED);

        Rectangle source = {0, 0, 16, 16};

        if (player.direction == DIRECTION_LEFT)
        {
            source.y += 16;
        }

        source.x = player.anamationIdx * 16;

        DrawTexturePro(playerTexture, source, playerRect, (Vector2){0, 0}, 0, WHITE);

        DrawCircleV(player.position, 5.0f, GOLD);

        EndMode2D();

        DrawText("Controls:", 20, 20, 10, BLACK);
        DrawText("- Right/Left to move", 40, 40, 10, DARKGRAY);
        DrawText("- Space to jump", 40, 60, 10, DARKGRAY);
        DrawText("- Mouse Wheel to Zoom in-out, R to reset zoom", 40, 80, 10, DARKGRAY);
        DrawText(TextFormat("Player xy%f,%f", player.position.x, player.position.y),
                 40, 100, 10, DARKGRAY);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

void UpdateWorld(EnvItem *envItems, int envItemsLength, float delta)
{
    for (size_t i = 0; i < envItemsLength; i++)
    {
        // This item has gravity
        if (envItems[i].gravity != -1)
        {
            bool hitObstacle = false;
            for (int j = 0; j < envItemsLength; j++)
            {
                EnvItem *ei = envItems + j;
                Vector2 p = (Vector2){envItems[i].rect.x, envItems[i].rect.y};

                if (ei->blocking &&
                    ei->rect.x <= p.x &&
                    ei->rect.x + ei->rect.width >= p.x &&
                    ei->rect.y >= p.y &&
                    ei->rect.y <= p.y + envItems[i].currFallSpeed * delta)
                {
                    hitObstacle = true;
                    envItems[i].currFallSpeed = 0.0f;
                    envItems[i].rect.y = ei->rect.y;
                    break;
                }

                if (!hitObstacle)
                {
                    envItems[i].rect.y += envItems[i].currFallSpeed * delta;
                    envItems[i].currFallSpeed += 10 * delta;
                }
            }
        }
    }
}

void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta)
{
    // walking anamation update
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT))
    {
        player->anamationTime++;
        if (player->anamationTime > 5)
        {
            player->anamationIdx++;
            player->anamationTime = 0;
            if (player->anamationIdx >= 8)
            {
                player->anamationIdx = 0;
            }
        }
    }
    else
        player->anamationIdx = 0;

    if (IsKeyDown(KEY_LEFT))
    {
        player->position.x -= PLAYER_HOR_SPD * delta;
        player->direction = DIRECTION_LEFT;
    }
    if (IsKeyDown(KEY_RIGHT))
    {
        player->position.x += PLAYER_HOR_SPD * delta;
        player->direction = DIRECTION_RIGHT;
    }
    if (IsKeyDown(KEY_SPACE) && player->canJump)
    {
        player->speed = -PLAYER_JUMP_SPD;
        player->canJump = false;
    }

    bool hitObstacle = false;
    for (int i = 0; i < envItemsLength; i++)
    {
        EnvItem *ei = envItems + i;
        Vector2 *p = &(player->position);
        if (ei->blocking &&
            ei->rect.x <= p->x &&
            ei->rect.x + ei->rect.width >= p->x &&
            ei->rect.y >= p->y &&
            ei->rect.y <= p->y + player->speed * delta)
        {
            hitObstacle = true;
            player->speed = 0.0f;
            p->y = ei->rect.y;
            break;
        }
    }

    if (!hitObstacle)
    {
        player->position.y += player->speed * delta;
        player->speed += G * delta;
        player->canJump = false;
    }
    else
        player->canJump = true;
}

void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->offset = (Vector2){width / 2.0f, height / 2.0f};
    camera->target = player->position;
}

void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = player->position;
    camera->offset = (Vector2){width / 2.0f, height / 2.0f};
    float minX = 1000, minY = 1000, maxX = -1000, maxY = -1000;

    for (int i = 0; i < envItemsLength; i++)
    {
        EnvItem *ei = envItems + i;
        minX = fminf(ei->rect.x, minX);
        maxX = fmaxf(ei->rect.x + ei->rect.width, maxX);
        minY = fminf(ei->rect.y, minY);
        maxY = fmaxf(ei->rect.y + ei->rect.height, maxY);
    }

    Vector2 max = GetWorldToScreen2D((Vector2){maxX, maxY}, *camera);
    Vector2 min = GetWorldToScreen2D((Vector2){minX, minY}, *camera);

    if (max.x < width)
        camera->offset.x = width - (max.x - width / 2);
    if (max.y < height)
        camera->offset.y = height - (max.y - height / 2);
    if (min.x > 0)
        camera->offset.x = width / 2 - min.x;
    if (min.y > 0)
        camera->offset.y = height / 2 - min.y;
}

void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static float minSpeed = 30;
    static float minEffectLength = 10;
    static float fractionSpeed = 0.8f;

    camera->offset = (Vector2){width / 2.0f, height / 2.0f};
    Vector2 diff = Vector2Subtract(player->position, camera->target);
    float length = Vector2Length(diff);

    if (length > minEffectLength)
    {
        float speed = fmaxf(fractionSpeed * length, minSpeed);
        camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed * delta / length));
    }
}

void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static float evenOutSpeed = 700;
    static int eveningOut = false;
    static float evenOutTarget;

    camera->offset = (Vector2){width / 2.0f, height / 2.0f};
    camera->target.x = player->position.x;

    if (eveningOut)
    {
        if (evenOutTarget > camera->target.y)
        {
            camera->target.y += evenOutSpeed * delta;

            if (camera->target.y > evenOutTarget)
            {
                camera->target.y = evenOutTarget;
                eveningOut = 0;
            }
        }
        else
        {
            camera->target.y -= evenOutSpeed * delta;

            if (camera->target.y < evenOutTarget)
            {
                camera->target.y = evenOutTarget;
                eveningOut = 0;
            }
        }
    }
    else
    {
        if (player->canJump && (player->speed == 0) && (player->position.y != camera->target.y))
        {
            eveningOut = 1;
            evenOutTarget = player->position.y;
        }
    }
}

void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static Vector2 bbox = {0.2f, 0.2f};

    Vector2 bboxWorldMin = GetScreenToWorld2D((Vector2){(1 - bbox.x) * 0.5f * width, (1 - bbox.y) * 0.5f * height}, *camera);
    Vector2 bboxWorldMax = GetScreenToWorld2D((Vector2){(1 + bbox.x) * 0.5f * width, (1 + bbox.y) * 0.5f * height}, *camera);
    camera->offset = (Vector2){(1 - bbox.x) * 0.5f * width, (1 - bbox.y) * 0.5f * height};

    if (player->position.x < bboxWorldMin.x)
        camera->target.x = player->position.x;
    if (player->position.y < bboxWorldMin.y)
        camera->target.y = player->position.y;
    if (player->position.x > bboxWorldMax.x)
        camera->target.x = bboxWorldMin.x + (player->position.x - bboxWorldMax.x);
    if (player->position.y > bboxWorldMax.y)
        camera->target.y = bboxWorldMin.y + (player->position.y - bboxWorldMax.y);
}