#include <raylib.h>
#include <stdio.h>

int main(int argc, char*argv[]){
    InitWindow(800,600,"Game");
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(WHITE);
        EndDrawing();
    }
    CloseWindow();
}