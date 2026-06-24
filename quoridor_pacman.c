#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define MAX_SIZE 15
#define MIN_SIZE 5
#define NUM_GHOSTS 4
#define NUM_PELLETS 4
#define MAX_TEMP_WALLS 128
#define CELL 48
#define BOARD_X 40
#define BOARD_Y 60
#define SIDE_X 560
#define OWNER_PAC 0
#define OWNER_GHOST 1

/*Pantallas del programa*/
typedef enum {
    SCREEN_CONFIG,
    SCREEN_GAME,
    SCREEN_EDITOR,
    SCREEN_END
} Screen;

/*Modo de partida*/
typedef enum {
    MODE_AI = 0,
    MODE_PVP = 1
} GameMode;

/*Turno actual*/
typedef enum {
    TURN_PAC = 0,
    TURN_GHOST = 1
} TurnType;

/*Direcciones de movimiento*/
typedef enum {
    DIR_UP = 0,
    DIR_RIGHT = 1,
    DIR_DOWN = 2,
    DIR_LEFT = 3
} Direction;

/*Tipo de muro: RIGHT bloquea entre (r,c) y (r,c+1), DOWN entre (r,c) y (r+1,c)*/
typedef enum {
    WALL_RIGHT = 0,
    WALL_DOWN = 1
} WallDir;

typedef struct {
    int r;
    int c;
} Pos;

typedef struct {
    int rows;
    int cols;
    Pos pacStart;
    Pos ghostStart[NUM_GHOSTS];
    Pos pelletStart[NUM_PELLETS];
    bool wallRight[MAX_SIZE][MAX_SIZE];
    bool wallDown[MAX_SIZE][MAX_SIZE];
} Map;

typedef struct {
    Pos start;
    Pos pos;
    bool enabled;
    bool alive;
    int difficulty;      /* 1 facil, 2 medio, 3 dificil */
    const char *name;
    Color color;
} Ghost;

typedef struct {
    int r;
    int c;
    int dir;             /* WALL_RIGHT o WALL_DOWN */
    int owner;           /* OWNER_PAC u OWNER_GHOST */
    int life;            /* turnos globales restantes */
    bool active;
} TempWall;

typedef struct {
    GameMode mode;
    bool ghostEnabled[NUM_GHOSTS];
    int ghostDifficulty[NUM_GHOSTS];
    int pacWallsStart;
    int ghostWallsStart;
    int wallLife;
    int mapIndex;        /* 0,1,2 predefinidos; 3 mapa guardado */
} Config;
typedef struct {
    Map map;
    Config config;
    Pos pac;
    Ghost ghosts[NUM_GHOSTS];
    bool pelletAlive[NUM_PELLETS];
    int lives;
    int pelletsEaten;
    int pacWallsHand;
    int ghostWallsHand;
    TempWall tempWalls[MAX_TEMP_WALLS];
    int tempWallCount;
    TurnType turn;
    int currentGhost;
    int pacActions;
    int globalTurn;
    bool gameOver;
    int winner;          /* 0 Pac-Man, 1 fantasmas */
    char message[160];
    float messageTime;
    float aiTimer;
} Game;

typedef enum {
    TOOL_PAC,
    TOOL_GHOST1,
    TOOL_GHOST2,
    TOOL_GHOST3,
    TOOL_GHOST4,
    TOOL_PELLET,
    TOOL_WALL
} EditorTool;

typedef struct {
    Map map;
    EditorTool tool;
    int pelletIndex;
    char message[160];
    float messageTime;
} Editor;

static Screen screen = SCREEN_CONFIG;
static Config config;
static Game game;
static Editor editor;
static int configOption = 0;
static Texture2D menuTexture;
static bool menuTextureLoaded = false;

/*Prototipos*/
static void InitDefaultConfig(Config *cfg);
static void ClearMap(Map *m, int rows, int cols);
static void LoadPredefinedMap(Map *m, int index);
static bool SaveMapToFile(const Map *m, const char *filename);
static bool LoadMapFromFile(Map *m, const char *filename);
static void InitGame(Game *g, const Config *cfg);
static void UpdateConfig(void);
static void DrawConfig(void);
static void UpdateGame(Game *g);
static void DrawGame(const Game *g);
static void UpdateEditor(Editor *e);
static void DrawEditor(const Editor *e);
static void DrawEndScreen(const Game *g);
static bool IsInside(const Map *m, int r, int c);
static bool IsBlocked(const Game *g, int r1, int c1, int r2, int c2);
static bool CanMove(const Game *g, Pos p, Direction d);
static Pos Step(Pos p, Direction d);
static void MovePac(Game *g, Direction d);
static void MoveGhost(Game *g, int ghostIndex, Direction d);
static bool PlaceTempWall(Game *g, int owner, int r, int c, int dir);
static void EndActorTurn(Game *g);
static void AdvanceToNextGhost(Game *g);
static void TickTempWalls(Game *g);
static void ResetPositionsAfterCapture(Game *g);
static void CheckPacPellet(Game *g);
static void CheckPacGhostCollisionAfterPacMove(Game *g);
static void CheckGhostCatchesPac(Game *g, int ghostIndex);
static bool GhostSeesPac(const Game *g, int ghostIndex);
static Direction ChooseGhostDirection(Game *g, int ghostIndex);
static void SetMessage(Game *g, const char *text);
static bool CellFromMouse(const Map *m, Vector2 mouse, int *r, int *c);
static bool WallFromMouse(const Map *m, Vector2 mouse, int *r, int *c, int *dir);
static void DrawBoardBase(const Map *m);
static void DrawWalls(const Map *m, const TempWall *tw, int count);
static void DrawHud(const Game *g);
static int Manhattan(Pos a, Pos b);
static int CountEnabledGhosts(const Config *cfg);
static void InitEditor(Editor *e);
static void EditorMessage(Editor *e, const char *text);

int main(void)
{
    InitWindow(900, 620, "Quoridor Pac-Man - LP1");
    SetExitKey(KEY_NULL); /* evita que ESC cierre toda la ventana */
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    if (FileExists("pacman_paraguay.png")) {
        menuTexture = LoadTexture("pacman_paraguay.png");
        menuTextureLoaded = true;
    } else if (FileExists("imagenes/pacman_paraguay.png")) {
        menuTexture = LoadTexture("imagenes/pacman_paraguay.png");
        menuTextureLoaded = true;
    }

    InitDefaultConfig(&config);
    InitEditor(&editor);

    while (!WindowShouldClose()) {
        if (screen == SCREEN_CONFIG) UpdateConfig();
        else if (screen == SCREEN_GAME) UpdateGame(&game);
        else if (screen == SCREEN_EDITOR) UpdateEditor(&editor);
        else if (screen == SCREEN_END) {
            if (IsKeyPressed(KEY_R)) {
                InitGame(&game, &config);
                screen = SCREEN_GAME;
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                screen = SCREEN_CONFIG;
            }
        }

        BeginDrawing();
        ClearBackground((Color){25, 25, 35, 255});
        if (screen == SCREEN_CONFIG) DrawConfig();
        else if (screen == SCREEN_GAME) DrawGame(&game);
        else if (screen == SCREEN_EDITOR) DrawEditor(&editor);
        else if (screen == SCREEN_END) DrawEndScreen(&game);
        EndDrawing();
    }

    if (menuTextureLoaded) UnloadTexture(menuTexture);
    CloseWindow();
    return 0;
}

/*CONFIGURACION*/

static void InitDefaultConfig(Config *cfg)
{
    int i;
    cfg->mode = MODE_AI;
    for (i = 0; i < NUM_GHOSTS; i++) {
        cfg->ghostEnabled[i] = true;
        cfg->ghostDifficulty[i] = 2;
    }
    cfg->pacWallsStart = 3;
    cfg->ghostWallsStart = 2;
    cfg->wallLife = 4;
    cfg->mapIndex = 0;
}

static void UpdateConfig(void)
{
    if (IsKeyPressed(KEY_DOWN)) configOption = (configOption + 1) % 12;
    if (IsKeyPressed(KEY_UP)) configOption = (configOption + 11) % 12;

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_SPACE)) {
        int delta = IsKeyPressed(KEY_LEFT) ? -1 : 1;
        switch (configOption) {
            case 0:
                config.mode = (config.mode == MODE_AI) ? MODE_PVP : MODE_AI;
                break;
            case 1: case 2: case 3: case 4: {
                int i = configOption - 1;
                config.ghostEnabled[i] = !config.ghostEnabled[i];
                break;
            }
            case 5: case 6: case 7: case 8: {
                int i = configOption - 5;
                config.ghostDifficulty[i] += delta;
                if (config.ghostDifficulty[i] < 1) config.ghostDifficulty[i] = 3;
                if (config.ghostDifficulty[i] > 3) config.ghostDifficulty[i] = 1;
                break;
            }
            case 9:
                config.pacWallsStart += delta;
                if (config.pacWallsStart < 0) config.pacWallsStart = 0;
                if (config.pacWallsStart > 9) config.pacWallsStart = 9;
                break;
            case 10:
                config.ghostWallsStart += delta;
                if (config.ghostWallsStart < 0) config.ghostWallsStart = 0;
                if (config.ghostWallsStart > 9) config.ghostWallsStart = 9;
                break;
            case 11:
                config.wallLife += delta;
                if (config.wallLife < 1) config.wallLife = 1;
                if (config.wallLife > 9) config.wallLife = 9;
                break;
            default: break;
        }
    }

    if (IsKeyPressed(KEY_TAB)) {
        config.mapIndex = (config.mapIndex + 1) % 4;
    }

    if (IsKeyPressed(KEY_E)) {
        InitEditor(&editor);
        screen = SCREEN_EDITOR;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (CountEnabledGhosts(&config) == 0) {
            config.ghostEnabled[0] = true; /* para que siempre exista al menos un fantasma */
        }
        InitGame(&game, &config);
        screen = SCREEN_GAME;
    }
}

static void DrawConfig(void)
{
    const char *ghostNames[NUM_GHOSTS] = {"Blinky", "Inky", "Pinky", "Clyde"};
    Color triColors[3] = {RED, RAYWHITE, SKYBLUE};
    int y = 104;
    int i;

    DrawText("QUORIDOR PAC-MAN", 40, 18, 38, YELLOW);
    DrawText("CONFIGURACION DE LA PARTIDA", 40, 56, 30, YELLOW);

    for (i = 0; i < 12; i++) {
        Color col = triColors[i % 3];
        char text[180];
        int itemY = y + i * 32;

        if (i == configOption) {
            DrawRectangle(52, itemY - 3, 470, 31, (Color){60, 60, 85, 255});
            DrawRectangleLines(52, itemY - 3, 470, 31, YELLOW);
            col = YELLOW;
        }

        if (i == 0) snprintf(text, sizeof(text), "Modo de juego: %s", config.mode == MODE_AI ? "IA vs Jugador" : "Jugador vs Jugador");
        else if (i >= 1 && i <= 4) {
            int g = i - 1;
            snprintf(text, sizeof(text), "%s habilitado: %s", ghostNames[g], config.ghostEnabled[g] ? "SI" : "NO");
        }
        else if (i >= 5 && i <= 8) {
            int g = i - 5;
            snprintf(text, sizeof(text), "Dificultad de %s: %d", ghostNames[g], config.ghostDifficulty[g]);
        }
        else if (i == 9) snprintf(text, sizeof(text), "Muros mano Pac-Man: %d", config.pacWallsStart);
        else if (i == 10) snprintf(text, sizeof(text), "Muros mano fantasmas: %d", config.ghostWallsStart);
        else snprintf(text, sizeof(text), "Vida de muros temporales: %d turnos", config.wallLife);

        DrawText(text, 64, itemY, 24, col);
    }

    DrawText("MAPA Y ESTILO", 570, 18, 30, YELLOW);
    if (config.mapIndex < 3) {
        char mapTxt[80];
        snprintf(mapTxt, sizeof(mapTxt), "Mapa actual: predefinido %d", config.mapIndex + 1);
        DrawText(mapTxt, 570, 60, 24, SKYBLUE);
    } else {
        DrawText("Mapa actual: custom_map.txt", 570, 60, 24, SKYBLUE);
    }

    DrawRectangleRounded((Rectangle){565, 96, 290, 330}, 0.08f, 8, (Color){35, 35, 55, 255});
    DrawRectangleLinesEx((Rectangle){565, 96, 290, 330}, 2, (Color){200, 200, 210, 255});

    if (menuTextureLoaded) {
        float texW = (float)menuTexture.width;
        float texH = (float)menuTexture.height;
        float scaleX = 270.0f / texW;
        float scaleY = 310.0f / texH;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;
        float drawW = texW * scale;
        float drawH = texH * scale;
        Rectangle src = {0, 0, texW, texH};
        Rectangle dst = {575 + (270 - drawW)/2.0f, 106 + (310 - drawH)/2.0f, drawW, drawH};
        DrawTexturePro(menuTexture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        /* Si no se encuentra el PNG, se dibuja un logo simple para no dejar vacio el menu. */
        DrawCircle(710, 250, 95, YELLOW);
        DrawTriangle((Vector2){710, 250}, (Vector2){810, 190}, (Vector2){810, 310}, (Color){25, 25, 35, 255});
        DrawCircle(735, 215, 13, BLACK);
        DrawRectangle(628, 265, 30, 70, RED);
        DrawRectangle(658, 265, 30, 70, RAYWHITE);
        DrawRectangle(688, 265, 30, 70, RED);
        DrawRectangle(718, 265, 30, 70, RAYWHITE);
        DrawRectangle(748, 265, 30, 70, RED);
        DrawText("APF", 690, 292, 24, BLUE);
    }

    DrawText("Chake:Dificultad solo afecta a la IA.", 520, 445, 20, LIGHTGRAY);
    DrawText("1 = NDAHASYI", 570, 475, 18, RED);
    DrawText("2 = HASY", 570, 500, 18, RAYWHITE);
    DrawText("3 = HASYETEREI", 570, 525, 18, SKYBLUE);

    DrawText("Flechas ARRIBA/ABAJO: elegir opcion | IZQ/DER/ESPACIO: cambiar", 40, 558, 18, LIGHTGRAY);
    DrawText("ENTER: jugar | TAB: cambiar mapa | E: editor de mapas", 40, 582, 18, LIGHTGRAY);
}

/* MAPAS*/

static void ClearMap(Map *m, int rows, int cols)
{
    int r, c, i;
    if (rows < MIN_SIZE) rows = MIN_SIZE;
    if (cols < MIN_SIZE) cols = MIN_SIZE;
    if (rows > MAX_SIZE) rows = MAX_SIZE;
    if (cols > MAX_SIZE) cols = MAX_SIZE;

    m->rows = rows;
    m->cols = cols;
    m->pacStart = (Pos){rows/2, cols/2};
    for (i = 0; i < NUM_GHOSTS; i++) m->ghostStart[i] = (Pos){0, i};
    m->pelletStart[0] = (Pos){0, 0};
    m->pelletStart[1] = (Pos){0, cols-1};
    m->pelletStart[2] = (Pos){rows-1, 0};
    m->pelletStart[3] = (Pos){rows-1, cols-1};

    for (r = 0; r < MAX_SIZE; r++) {
        for (c = 0; c < MAX_SIZE; c++) {
            m->wallRight[r][c] = false;
            m->wallDown[r][c] = false;
        }
    }
}

static void LoadPredefinedMap(Map *m, int index)
{
    int r, c;
    if (index == 0) {
        ClearMap(m, 9, 9);
        m->pacStart = (Pos){4, 4};
        m->ghostStart[0]=(Pos){0, 0};
        m->ghostStart[1]=(Pos){0, 8};
        m->ghostStart[2]=(Pos){8, 0};
        m->ghostStart[3]=(Pos){8, 8};
        m->pelletStart[0]=(Pos){1, 4};
        m->pelletStart[1]=(Pos){4, 1};
        m->pelletStart[2]=(Pos){4, 7};
        m->pelletStart[3]=(Pos){7, 4};
        for (r = 2; r <= 6; r++) {
            if (r != 4) m->wallRight[r][3] = true;
            if (r != 4) m->wallRight[r][4] = true;
        }
        for (c = 2; c <= 6; c++) {
            if (c != 4) m->wallDown[3][c] = true;
            if (c != 4) m->wallDown[4][c] = true;
        }
    } else if (index==1) {
        ClearMap(m, 7, 11);
        m->pacStart = (Pos){3, 5};
        m->ghostStart[0]=(Pos){0, 5};
        m->ghostStart[1]=(Pos){6, 5};
        m->ghostStart[2]=(Pos){3, 0};
        m->ghostStart[3]=(Pos){3, 10};
        m->pelletStart[0]=(Pos){1, 1};
        m->pelletStart[1]=(Pos){1, 9};
        m->pelletStart[2]=(Pos){5, 1};
        m->pelletStart[3]=(Pos){5, 9};
        for (c = 1; c < 10; c++) {
            if (c != 5) m->wallDown[1][c] = true;
            if (c != 5) m->wallDown[4][c] = true;
        }
        for (r = 1; r < 6; r++) {
            if (r != 3) m->wallRight[r][2] = true;
            if (r != 3) m->wallRight[r][7] = true;
        }
    } else {
        ClearMap(m, 11, 11);
        m->pacStart = (Pos){5, 5};
        m->ghostStart[0]=(Pos){0, 0};
        m->ghostStart[1]=(Pos){0, 10};
        m->ghostStart[2]=(Pos){10, 0};
        m->ghostStart[3]=(Pos){10, 10};
        m->pelletStart[0]=(Pos){2, 2};
        m->pelletStart[1]=(Pos){2, 8};
        m->pelletStart[2]=(Pos){8, 2};
        m->pelletStart[3]=(Pos){8, 8};
        for (r = 1; r < 10; r++) {
            if (r != 5) {
                m->wallRight[r][4] = true;
                m->wallRight[r][5] = true;
            }
        }
        for (c = 1; c < 10; c++) {
            if (c != 5) {
                m->wallDown[4][c] = true;
                m->wallDown[5][c] = true;
            }
        }
    }
}

static bool SaveMapToFile(const Map *m, const char *filename)
{
    FILE *f = fopen(filename, "w");
    int r, c, i, countR = 0, countD = 0;
    if (!f) return false;

    fprintf(f, "ROWS %d\n", m->rows);
    fprintf(f, "COLS %d\n", m->cols);
    fprintf(f, "PAC %d %d\n", m->pacStart.r, m->pacStart.c);
    for (i = 0; i < NUM_GHOSTS; i++) fprintf(f, "GHOST %d %d %d\n", i, m->ghostStart[i].r, m->ghostStart[i].c);
    for (i = 0; i < NUM_PELLETS; i++) fprintf(f, "PELLET %d %d %d\n", i, m->pelletStart[i].r, m->pelletStart[i].c);

    for (r = 0; r < m->rows; r++) for (c = 0; c < m->cols - 1; c++) if (m->wallRight[r][c]) countR++;
    for (r = 0; r < m->rows - 1; r++) for (c = 0; c < m->cols; c++) if (m->wallDown[r][c]) countD++;

    fprintf(f, "WALLRIGHT %d\n", countR);
    for (r = 0; r < m->rows; r++) for (c = 0; c < m->cols - 1; c++) if (m->wallRight[r][c]) fprintf(f, "%d %d\n", r, c);
    fprintf(f, "WALLDOWN %d\n", countD);
    for (r = 0; r < m->rows - 1; r++) for (c = 0; c < m->cols; c++) if (m->wallDown[r][c]) fprintf(f, "%d %d\n", r, c);

    fclose(f);
    return true;
}

static bool LoadMapFromFile(Map *m, const char *filename)
{
    FILE *f = fopen(filename, "r");
    char tag[32];
    int rows = 9, cols = 9, i, r, c, count;
    if (!f) return false;

    ClearMap(m, 9, 9);
    while (fscanf(f, "%31s", tag) == 1) {
        if (strcmp(tag, "ROWS") == 0) fscanf(f, "%d", &rows);
        else if (strcmp(tag, "COLS") == 0) {
            fscanf(f, "%d", &cols);
            ClearMap(m, rows, cols);
        }
        else if (strcmp(tag, "PAC") == 0) fscanf(f, "%d %d", &m->pacStart.r, &m->pacStart.c);
        else if (strcmp(tag, "GHOST") == 0) {
            fscanf(f, "%d", &i);
            if (i >= 0 && i < NUM_GHOSTS) fscanf(f, "%d %d", &m->ghostStart[i].r, &m->ghostStart[i].c);
            else fscanf(f, "%d %d", &r, &c);
        }
        else if (strcmp(tag, "PELLET") == 0) {
            fscanf(f, "%d", &i);
            if (i >= 0 && i < NUM_PELLETS) fscanf(f, "%d %d", &m->pelletStart[i].r, &m->pelletStart[i].c);
            else fscanf(f, "%d %d", &r, &c);
        }
        else if (strcmp(tag, "WALLRIGHT") == 0) {
            fscanf(f, "%d", &count);
            for (i = 0; i < count; i++) {
                fscanf(f, "%d %d", &r, &c);
                if (r >= 0 && r < m->rows && c >= 0 && c < m->cols - 1) m->wallRight[r][c] = true;
            }
        }
        else if (strcmp(tag, "WALLDOWN") == 0) {
            fscanf(f, "%d", &count);
            for (i = 0; i < count; i++) {
                fscanf(f, "%d %d", &r, &c);
                if (r >= 0 && r < m->rows - 1 && c >= 0 && c < m->cols) m->wallDown[r][c] = true;
            }
        }
    }

    fclose(f);
    return true;
}

/*INICIALIZACION DEL JUEGO*/

static void InitGame(Game *g, const Config *cfg)
{
    static const char *names[NUM_GHOSTS] = {"Blinky", "Inky", "Pinky", "Clyde"};
    static const Color colors[NUM_GHOSTS] = {RED, SKYBLUE, PINK, ORANGE};
    int i;

    memset(g, 0, sizeof(Game));
    g->config = *cfg;

    if (cfg->mapIndex == 3) {
        if (!LoadMapFromFile(&g->map, "custom_map.txt")) LoadPredefinedMap(&g->map, 0);
    } else {
        LoadPredefinedMap(&g->map, cfg->mapIndex);
    }

    g->pac = g->map.pacStart;
    for (i = 0; i < NUM_GHOSTS; i++) {
        g->ghosts[i].start = g->map.ghostStart[i];
        g->ghosts[i].pos = g->map.ghostStart[i];
        g->ghosts[i].enabled = cfg->ghostEnabled[i];
        g->ghosts[i].alive = cfg->ghostEnabled[i];
        g->ghosts[i].difficulty = cfg->ghostDifficulty[i];
        g->ghosts[i].name = names[i];
        g->ghosts[i].color = colors[i];
    }

    for (i = 0; i < NUM_PELLETS; i++) g->pelletAlive[i] = true;
    g->lives = 3;
    g->pelletsEaten = 0;
    g->pacWallsHand = cfg->pacWallsStart;
    g->ghostWallsHand = cfg->ghostWallsStart;
    g->tempWallCount = 0;
    g->turn = TURN_PAC;
    g->currentGhost = 0;
    g->pacActions = 2;
    g->globalTurn = 1;
    g->gameOver = false;
    g->winner = -1;
    g->aiTimer = 0.0f;
    SetMessage(g, "Turno de Pac-Man");
}

/*LOGICA PRINCIPAL*/

static void UpdateGame(Game *g)
{
    if (IsKeyPressed(KEY_R)) {
        InitGame(g, &config);
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        screen = SCREEN_CONFIG;
        return;
    }
    if (g->messageTime > 0) g->messageTime -= GetFrameTime();

    if (g->gameOver) {
        screen = SCREEN_END;
        return;
    }

    if (g->turn == TURN_PAC) {
        if (IsKeyPressed(KEY_UP)) MovePac(g, DIR_UP);
        if (IsKeyPressed(KEY_RIGHT)) MovePac(g, DIR_RIGHT);
        if (IsKeyPressed(KEY_DOWN)) MovePac(g, DIR_DOWN);
        if (IsKeyPressed(KEY_LEFT)) MovePac(g, DIR_LEFT);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int r, c, dir;
            if (WallFromMouse(&g->map, GetMousePosition(), &r, &c, &dir)) {
                if (g->pacActions > 0 && PlaceTempWall(g, OWNER_PAC, r, c, dir)) {
                    g->pacActions--;
                    SetMessage(g, "Pac-Man coloco un muro temporal.");
                    if (g->pacActions <= 0) EndActorTurn(g);
                }
            }
        }

        if (IsKeyPressed(KEY_ENTER)) EndActorTurn(g);
    } else {
        int i = g->currentGhost;
        if (!g->ghosts[i].enabled || !g->ghosts[i].alive) {
            AdvanceToNextGhost(g);
            return;
        }

        if (g->config.mode == MODE_AI) {
            g->aiTimer += GetFrameTime();
            if (g->aiTimer > 0.55f) {
                Direction d = ChooseGhostDirection(g, i);
                MoveGhost(g, i, d);
                g->aiTimer = 0;
                AdvanceToNextGhost(g);
            }
        } else {
            if (IsKeyPressed(KEY_W)) { MoveGhost(g, i, DIR_UP); AdvanceToNextGhost(g); }
            if (IsKeyPressed(KEY_D)) { MoveGhost(g, i, DIR_RIGHT); AdvanceToNextGhost(g); }
            if (IsKeyPressed(KEY_S)) { MoveGhost(g, i, DIR_DOWN); AdvanceToNextGhost(g); }
            if (IsKeyPressed(KEY_A)) { MoveGhost(g, i, DIR_LEFT); AdvanceToNextGhost(g); }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int r, c, dir;
                if (WallFromMouse(&g->map, GetMousePosition(), &r, &c, &dir)) {
                    if (PlaceTempWall(g, OWNER_GHOST, r, c, dir)) {
                        SetMessage(g, "Fantasma coloco un muro temporal.");
                        AdvanceToNextGhost(g);
                    }
                }
            }
            if (IsKeyPressed(KEY_ENTER)) AdvanceToNextGhost(g);
        }
    }
}

static void MovePac(Game *g, Direction d)
{
    Pos np;
    if (g->pacActions <= 0) return;
    if (!CanMove(g, g->pac, d)) {
        SetMessage(g, "Movimiento bloqueado por borde o muro.");
        return;
    }
    np = Step(g->pac, d);
    g->pac = np;
    g->pacActions--;

    CheckPacPellet(g);
    CheckPacGhostCollisionAfterPacMove(g);

    if (g->pelletsEaten >= NUM_PELLETS) {
        g->gameOver = true;
        g->winner = OWNER_PAC;
        screen = SCREEN_END;
        return;
    }

    if (g->pacActions <= 0) EndActorTurn(g);
}

static void MoveGhost(Game *g, int ghostIndex, Direction d)
{
    int steps = 1;
    int s;
    if (!g->ghosts[ghostIndex].enabled || !g->ghosts[ghostIndex].alive) return;
    if (GhostSeesPac(g, ghostIndex)) steps = 2;

    for (s = 0; s < steps; s++) {
        if (!CanMove(g, g->ghosts[ghostIndex].pos, d)) break;
        g->ghosts[ghostIndex].pos = Step(g->ghosts[ghostIndex].pos, d);
        CheckGhostCatchesPac(g, ghostIndex);
        if (g->gameOver) break;
        if (g->ghosts[ghostIndex].pos.r == g->pac.r && g->ghosts[ghostIndex].pos.c == g->pac.c) break;
    }
}

static bool PlaceTempWall(Game *g, int owner, int r, int c, int dir)
{
    int i;
    if (dir == WALL_RIGHT) {
        if (r < 0 || r >= g->map.rows || c < 0 || c >= g->map.cols - 1) return false;
        if (g->map.wallRight[r][c]) return false;
    } else {
        if (r < 0 || r >= g->map.rows - 1 || c < 0 || c >= g->map.cols) return false;
        if (g->map.wallDown[r][c]) return false;
    }

    for (i = 0; i < g->tempWallCount; i++) {
        if (g->tempWalls[i].active && g->tempWalls[i].r == r && g->tempWalls[i].c == c && g->tempWalls[i].dir == dir) return false;
    }

    if (owner == OWNER_PAC) {
        if (g->pacWallsHand <= 0) { SetMessage(g, "Pac-Man no tiene muros en mano."); return false; }
        g->pacWallsHand--;
    } else {
        if (g->ghostWallsHand <= 0) { SetMessage(g, "Los fantasmas no tienen muros en mano."); return false; }
        g->ghostWallsHand--;
    }

    if (g->tempWallCount < MAX_TEMP_WALLS) {
        TempWall *w = &g->tempWalls[g->tempWallCount++];
        w->r = r;
        w->c = c;
        w->dir = dir;
        w->owner = owner;
        w->life = g->config.wallLife;
        w->active = true;
        return true;
    }
    return false;
}

static void EndActorTurn(Game *g)
{
    TickTempWalls(g);
    g->globalTurn++;
    g->turn = TURN_GHOST;
    g->currentGhost = 0;
    g->aiTimer = 0;
    SetMessage(g, "Turno de fantasmas.");
}

static void AdvanceToNextGhost(Game *g)
{
    int attempts = 0;
    g->currentGhost++;
    while (g->currentGhost < NUM_GHOSTS && (!g->ghosts[g->currentGhost].enabled || !g->ghosts[g->currentGhost].alive)) {
        g->currentGhost++;
        attempts++;
        if (attempts > NUM_GHOSTS) break;
    }

    if (g->currentGhost >= NUM_GHOSTS) {
        TickTempWalls(g);
        g->globalTurn++;
        g->turn = TURN_PAC;
        g->pacActions = 2;
        SetMessage(g, "Turno de Pac-Man.");
    }
}

static void TickTempWalls(Game *g)
{
    int i;
    for (i = 0; i < g->tempWallCount; i++) {
        if (g->tempWalls[i].active) {
            g->tempWalls[i].life--;
            if (g->tempWalls[i].life <= 0) {
                if (g->tempWalls[i].owner == OWNER_PAC) g->pacWallsHand++;
                else g->ghostWallsHand++;
                g->tempWalls[i].active = false;
            }
        }
    }
}

static void ResetPositionsAfterCapture(Game *g)
{
    int i;
    g->pac = g->map.pacStart;
    for (i = 0; i < NUM_GHOSTS; i++) {
        if (g->ghosts[i].enabled) {
            g->ghosts[i].pos = g->ghosts[i].start;
            g->ghosts[i].alive = true;
        }
    }
}

static void CheckPacPellet(Game *g)
{
    int i;
    for (i = 0; i < NUM_PELLETS; i++) {
        if (g->pelletAlive[i] && g->pac.r == g->map.pelletStart[i].r && g->pac.c == g->map.pelletStart[i].c) {
            g->pelletAlive[i] = false;
            g->pelletsEaten++;
            g->pacActions++; /* accion extra */
            SetMessage(g,"Pac-Man comio una pac-bola: Gana una accion");
            return;
        }
    }
}

static void CheckPacGhostCollisionAfterPacMove(Game *g)
{
    int i;
    for (i = 0; i < NUM_GHOSTS; i++) {
        if (g->ghosts[i].enabled && g->ghosts[i].alive && g->ghosts[i].pos.r == g->pac.r && g->ghosts[i].pos.c == g->pac.c) {
            g->ghosts[i].alive = false;
            SetMessage(g, "Pac-Man piso un fantasma y lo comio.");
        }
    }
}

static void CheckGhostCatchesPac(Game *g, int ghostIndex)
{
    if (g->ghosts[ghostIndex].enabled && g->ghosts[ghostIndex].alive &&
        g->ghosts[ghostIndex].pos.r == g->pac.r && g->ghosts[ghostIndex].pos.c == g->pac.c) {
        g->lives--;
        if (g->lives <= 0) {
            g->gameOver = true;
            g->winner = OWNER_GHOST;
            screen = SCREEN_END;
        } else {
            ResetPositionsAfterCapture(g);
            SetMessage(g, "Un fantasma atrapo a Pac-Man. Pierde una vida y se reinician posiciones.");
        }
    }
}

/*MOVIMIENTO / COLISIONES */

static bool IsInside(const Map *m, int r, int c)
{
    return r >= 0 && r < m->rows && c >= 0 && c < m->cols;
}

static Pos Step(Pos p, Direction d)
{
    if (d == DIR_UP) p.r--;
    else if (d == DIR_RIGHT) p.c++;
    else if (d == DIR_DOWN) p.r++;
    else if (d == DIR_LEFT) p.c--;
    return p;
}

static bool IsBlocked(const Game *g, int r1, int c1, int r2, int c2)
{
    int wr = -1, wc = -1, wd = -1;
    int i;

    if (!IsInside(&g->map, r1, c1) || !IsInside(&g->map, r2, c2)) return true;

    if (r1 == r2 && c2 == c1 + 1) { wr = r1; wc = c1; wd = WALL_RIGHT; }
    else if (r1 == r2 && c2 == c1 - 1) { wr = r2; wc = c2; wd = WALL_RIGHT; }
    else if (c1 == c2 && r2 == r1 + 1) { wr = r1; wc = c1; wd = WALL_DOWN; }
    else if (c1 == c2 && r2 == r1 - 1) { wr = r2; wc = c2; wd = WALL_DOWN; }
    else return true;

    if (wd == WALL_RIGHT && g->map.wallRight[wr][wc]) return true;
    if (wd == WALL_DOWN && g->map.wallDown[wr][wc]) return true;

    for (i = 0; i < g->tempWallCount; i++) {
        if (g->tempWalls[i].active && g->tempWalls[i].r == wr && g->tempWalls[i].c == wc && g->tempWalls[i].dir == wd) {
            return true;
        }
    }
    return false;
}

static bool CanMove(const Game *g, Pos p, Direction d)
{
    Pos np = Step(p, d);
    if (!IsInside(&g->map, np.r, np.c)) return false;
    return !IsBlocked(g, p.r, p.c, np.r, np.c);
}

static int Manhattan(Pos a, Pos b)
{
    int dr = a.r - b.r;
    int dc = a.c - b.c;
    if (dr < 0) dr = -dr;
    if (dc < 0) dc = -dc;
    return dr + dc;
}

static bool GhostSeesPac(const Game *g, int ghostIndex)
{
    Pos gp = g->ghosts[ghostIndex].pos;
    Pos pp = g->pac;
    int step, c, r;

    if (gp.r == pp.r) {
        step = (pp.c > gp.c) ? 1 : -1;
        for (c = gp.c; c != pp.c; c += step) {
            if (IsBlocked(g, gp.r, c, gp.r, c + step)) return false;
        }
        return true;
    }
    if (gp.c == pp.c) {
        step = (pp.r > gp.r) ? 1 : -1;
        for (r = gp.r; r != pp.r; r += step) {
            if (IsBlocked(g, r, gp.c, r + step, gp.c)) return false;
        }
        return true;
    }
    return false;
}

static Direction ChooseGhostDirection(Game *g, int ghostIndex)
{
    Direction dirs[4] = {DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT};
    Direction valid[4];
    int validCount = 0;
    int i;
    int bestDist = 9999;
    Direction best = DIR_UP;
    int diff = g->ghosts[ghostIndex].difficulty;

    for (i = 0; i < 4; i++) {
        if (CanMove(g, g->ghosts[ghostIndex].pos, dirs[i])) {
            valid[validCount++] = dirs[i];
            int dist = Manhattan(Step(g->ghosts[ghostIndex].pos, dirs[i]), g->pac);
            if (dist < bestDist) {
                bestDist = dist;
                best = dirs[i];
            }
        }
    }
    if (validCount == 0) return DIR_UP;

    if (diff == 1) return valid[rand() % validCount];
    if (diff == 2 && (rand() % 100) < 35) return valid[rand() % validCount];
    return best;
}

static int CountEnabledGhosts(const Config *cfg)
{
    int i, total = 0;
    for (i = 0; i < NUM_GHOSTS; i++) if (cfg->ghostEnabled[i]) total++;
    return total;
}

static void SetMessage(Game *g, const char *text)
{
    strncpy(g->message, text, sizeof(g->message) - 1);
    g->message[sizeof(g->message) - 1] = '\0';
    g->messageTime = 3.0f;
}

/*DIBUJO*/

static void DrawBoardBase(const Map *m)
{
    int r, c;
    for (r = 0; r < m->rows; r++) {
        for (c = 0; c < m->cols; c++) {
            int x = BOARD_X + c * CELL;
            int y = BOARD_Y + r * CELL;
            DrawRectangle(x, y, CELL, CELL, (Color){40, 40, 55, 255});
            DrawRectangleLines(x, y, CELL, CELL, (Color){90, 90, 110, 255});
        }
    }
}

static void DrawWalls(const Map *m, const TempWall *tw, int count)
{
    int r, c, i;
    for (r = 0; r < m->rows; r++) {
        for (c = 0; c < m->cols - 1; c++) {
            if (m->wallRight[r][c]) {
                int x = BOARD_X + (c + 1) * CELL - 4;
                int y = BOARD_Y + r * CELL + 4;
                DrawRectangle(x, y, 8, CELL - 8, DARKBLUE);
            }
        }
    }
    for (r = 0; r < m->rows - 1; r++) {
        for (c = 0; c < m->cols; c++) {
            if (m->wallDown[r][c]) {
                int x = BOARD_X + c * CELL + 4;
                int y = BOARD_Y + (r + 1) * CELL - 4;
                DrawRectangle(x, y, CELL - 8, 8, DARKBLUE);
            }
        }
    }

    for (i = 0; i < count; i++) {
        if (tw[i].active) {
            Color col = (tw[i].owner == OWNER_PAC) ? GOLD : MAROON;
            if (tw[i].dir == WALL_RIGHT) {
                int x = BOARD_X + (tw[i].c + 1) * CELL - 4;
                int y = BOARD_Y + tw[i].r * CELL + 4;
                DrawRectangle(x, y, 8, CELL - 8, col);
                DrawText(TextFormat("%d", tw[i].life), x - 4, y + 12, 14, WHITE);
            } else {
                int x = BOARD_X + tw[i].c * CELL + 4;
                int y = BOARD_Y + (tw[i].r + 1) * CELL - 4;
                DrawRectangle(x, y, CELL - 8, 8, col);
                DrawText(TextFormat("%d", tw[i].life), x + 12, y - 6, 14, WHITE);
            }
        }
    }
}

static void DrawGame(const Game *g)
{
    int i;
    DrawBoardBase(&g->map);
    DrawWalls(&g->map, g->tempWalls, g->tempWallCount);

    for (i = 0; i < NUM_PELLETS; i++) {
        if (g->pelletAlive[i]) {
            int x = BOARD_X + g->map.pelletStart[i].c * CELL + CELL/2;
            int y = BOARD_Y + g->map.pelletStart[i].r * CELL + CELL/2;
            DrawCircle(x, y, 8, GREEN);
        }
    }

    DrawCircle(BOARD_X + g->pac.c * CELL + CELL/2,
               BOARD_Y + g->pac.r * CELL + CELL/2,
               17, YELLOW);
    DrawText("P", BOARD_X + g->pac.c * CELL + 18, BOARD_Y + g->pac.r * CELL + 12, 22, BLACK);

    for (i = 0; i < NUM_GHOSTS; i++) {
        if (g->ghosts[i].enabled && g->ghosts[i].alive) {
            int x = BOARD_X + g->ghosts[i].pos.c * CELL + CELL/2;
            int y = BOARD_Y + g->ghosts[i].pos.r * CELL + CELL/2;
            DrawCircle(x, y, 16, g->ghosts[i].color);
            DrawText(TextFormat("%d", i+1), x-5, y-9, 18, BLACK);
        }
    }

    DrawHud(g);
}

static void DrawHud(const Game *g)
{
    int x = SIDE_X;
    int y = 45;
    DrawText("INFORMACION", x, y, 24, YELLOW); y += 38;
    DrawText(TextFormat("Vidas: %d", g->lives), x, y, 20, RAYWHITE); y += 25;
    DrawText(TextFormat("Pac-bolas: %d/4", g->pelletsEaten), x, y, 20, RAYWHITE); y += 25;
    DrawText(TextFormat("Muros P-M: %d", g->pacWallsHand), x, y, 20, RAYWHITE); y += 25;
    DrawText(TextFormat("Muros Fant.: %d", g->ghostWallsHand), x, y, 20, RAYWHITE); y += 25;
    DrawText(TextFormat("Turno global: %d", g->globalTurn), x, y, 20, RAYWHITE); y += 25;

    if (g->turn == TURN_PAC) {
        DrawText("Turno: Pac-Man", x, y, 20, GOLD); y += 25;
        DrawText(TextFormat("Acciones: %d", g->pacActions), x, y, 20, GOLD); y += 35;
        DrawText("Flechas: mover", x, y, 18, LIGHTGRAY); y += 22;
        DrawText("Click borde: muro", x, y, 18, LIGHTGRAY); y += 22;
        DrawText("Enter: fin turno", x, y, 18, LIGHTGRAY); y += 22;
    } else {
        DrawText(TextFormat("Turno: %s", g->ghosts[g->currentGhost].name), x, y, 20, ORANGE); y += 25;
        if (g->config.mode == MODE_AI) DrawText("IA jugando...", x, y, 18, LIGHTGRAY);
        else DrawText("WASD mueve fantasma", x, y, 18, LIGHTGRAY);
        y += 35;
    }

    DrawText("R: reiniciar", x, y, 18, LIGHTGRAY); y += 22;
    DrawText("ESC: menu", x, y, 18, LIGHTGRAY); y += 34;

    DrawText("TIPOS DE MUROS:", x, y, 18, YELLOW); y += 24;
    DrawText("Azul: Muro fijo", x, y, 16, LIGHTGRAY); y += 20;
    DrawText("Amarillo: muro de Pac-man", x, y, 16, LIGHTGRAY); y += 20;
    DrawText("Rojo: muro de fantasmas", x, y, 16, LIGHTGRAY); y += 20;
    DrawText("Numero= turnos restantes", x, y, 16, LIGHTGRAY); y += 30;

    if (g->messageTime > 0) {
        if (g->messageTime > 0) {
        DrawRectangle(x - 10, 500, 330, 75, (Color){45, 45, 60, 255});
        DrawRectangleLines(x - 10, 500, 330, 75, YELLOW);
        DrawText(g->message, x, 515, 14, RAYWHITE);
}
    }
}

static void DrawEndScreen(const Game *g)
{
    const char *level = "Pac-Man novato";
    if (g->pelletsEaten == 2) level = "Pac-Man prometedor";
    else if (g->pelletsEaten == 3) level = "Pac-Man de categoria";
    else if (g->pelletsEaten >= 4) level = "Pac-Man de elite";

    DrawText("FIN DE LA PARTIDA", 260, 120, 40, YELLOW);
    if (g->winner == OWNER_PAC) DrawText("Gano Pac-Man", 330, 190, 32, GOLD);
    else DrawText("Ganaron los fantasmas", 280, 190, 32, RED);
    DrawText(TextFormat("Pac-bolas comidas: %d/4", g->pelletsEaten), 310, 250, 24, RAYWHITE);
    DrawText(TextFormat("Nivel alcanzado: %s", level), 260, 290, 24, SKYBLUE);
    DrawText("R: reiniciar partida", 320, 370, 22, LIGHTGRAY);
    DrawText("ENTER o ESC: volver al menu", 275, 405, 22, LIGHTGRAY);
}

/*MOUSE Y EDITOR*/

static bool CellFromMouse(const Map *m, Vector2 mouse, int *r, int *c)
{
    int col = (int)((mouse.x - BOARD_X) / CELL);
    int row = (int)((mouse.y - BOARD_Y) / CELL);
    if (IsInside(m, row, col)) {
        *r = row;
        *c = col;
        return true;
    }
    return false;
}

static bool WallFromMouse(const Map *m, Vector2 mouse, int *r, int *c, int *dir)
{
    int col, row;
    float localX, localY;
    if (!CellFromMouse(m, mouse, &row, &col)) return false;

    localX = mouse.x - (BOARD_X + col * CELL);
    localY = mouse.y - (BOARD_Y + row * CELL);

    /* Si el click esta cerca del borde derecho, se intenta muro vertical. */
    if (localX > CELL - 12 && col < m->cols - 1) {
        *r = row;
        *c = col;
        *dir = WALL_RIGHT;
        return true;
    }
    /* Si esta cerca del borde inferior, se intenta muro horizontal. */
    if (localY > CELL - 12 && row < m->rows - 1) {
        *r = row;
        *c = col;
        *dir = WALL_DOWN;
        return true;
    }
    return false;
}

static void InitEditor(Editor *e)
{
    ClearMap(&e->map, 9, 9);
    e->tool = TOOL_PAC;
    e->pelletIndex = 0;
    strcpy(e->message, "Editor listo. P,1,2,3,4,B,M eligen herramienta. S guarda.");
    e->messageTime = 4.0f;
}

static void EditorMessage(Editor *e, const char *text)
{
    strncpy(e->message, text, sizeof(e->message)-1);
    e->message[sizeof(e->message)-1] = '\0';
    e->messageTime = 3.0f;
}

static void UpdateEditor(Editor *e)
{
    int r, c, dir;
    if (e->messageTime > 0) e->messageTime -= GetFrameTime();

    if (IsKeyPressed(KEY_ESCAPE)) screen = SCREEN_CONFIG;
    if (IsKeyPressed(KEY_P)) { e->tool = TOOL_PAC; EditorMessage(e, "Herramienta: Pac-Man"); }
    if (IsKeyPressed(KEY_ONE)) { e->tool = TOOL_GHOST1; EditorMessage(e, "Herramienta: Blinky"); }
    if (IsKeyPressed(KEY_TWO)) { e->tool = TOOL_GHOST2; EditorMessage(e, "Herramienta: Inky"); }
    if (IsKeyPressed(KEY_THREE)) { e->tool = TOOL_GHOST3; EditorMessage(e, "Herramienta: Pinky"); }
    if (IsKeyPressed(KEY_FOUR)) { e->tool = TOOL_GHOST4; EditorMessage(e, "Herramienta: Clyde"); }
    if (IsKeyPressed(KEY_B)) { e->tool = TOOL_PELLET; e->pelletIndex = (e->pelletIndex + 1) % NUM_PELLETS; EditorMessage(e, "Herramienta: pac-bola. B cambia indice."); }
    if (IsKeyPressed(KEY_M)) { e->tool = TOOL_WALL; EditorMessage(e, "Herramienta: muro permanente. Click en borde."); }

    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        int rows = e->map.rows;
        int cols = e->map.cols + 1;
        ClearMap(&e->map, rows, cols);
        EditorMessage(e, "Columnas aumentadas. Se reinicio el mapa.");
    }
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        int rows = e->map.rows;
        int cols = e->map.cols - 1;
        ClearMap(&e->map, rows, cols);
        EditorMessage(e, "Columnas reducidas. Se reinicio el mapa.");
    }
    if (IsKeyPressed(KEY_EQUAL)) {
        int rows = e->map.rows + 1;
        int cols = e->map.cols;
        ClearMap(&e->map, rows, cols);
        EditorMessage(e, "Filas aumentadas. Se reinicio el mapa.");
    }
    if (IsKeyPressed(KEY_MINUS)) {
        int rows = e->map.rows - 1;
        int cols = e->map.cols;
        ClearMap(&e->map, rows, cols);
        EditorMessage(e, "Filas reducidas. Se reinicio el mapa.");
    }

    if (IsKeyPressed(KEY_S)) {
        if (SaveMapToFile(&e->map, "custom_map.txt")) EditorMessage(e, "Mapa guardado como custom_map.txt");
        else EditorMessage(e, "No se pudo guardar el mapa.");
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (e->tool == TOOL_WALL) {
            if (WallFromMouse(&e->map, GetMousePosition(), &r, &c, &dir)) {
                if (dir == WALL_RIGHT) e->map.wallRight[r][c] = !e->map.wallRight[r][c];
                else e->map.wallDown[r][c] = !e->map.wallDown[r][c];
            }
        } else if (CellFromMouse(&e->map, GetMousePosition(), &r, &c)) {
            if (e->tool == TOOL_PAC) e->map.pacStart = (Pos){r, c};
            else if (e->tool >= TOOL_GHOST1 && e->tool <= TOOL_GHOST4) e->map.ghostStart[e->tool - TOOL_GHOST1] = (Pos){r, c};
            else if (e->tool == TOOL_PELLET) e->map.pelletStart[e->pelletIndex] = (Pos){r, c};
        }
    }
}

static void DrawEditor(const Editor *e)
{
    int i;
    const char *toolName = "Pac-Man";
    if (e->tool == TOOL_GHOST1) toolName = "Blinky";
    else if (e->tool == TOOL_GHOST2) toolName = "Inky";
    else if (e->tool == TOOL_GHOST3) toolName = "Pinky";
    else if (e->tool == TOOL_GHOST4) toolName = "Clyde";
    else if (e->tool == TOOL_PELLET) toolName = "Pac-bola";
    else if (e->tool == TOOL_WALL) toolName = "Muro permanente";

    DrawBoardBase(&e->map);
    DrawWalls(&e->map, NULL, 0);

    for (i = 0; i < NUM_PELLETS; i++) {
        DrawCircle(BOARD_X + e->map.pelletStart[i].c * CELL + CELL/2,
                   BOARD_Y + e->map.pelletStart[i].r * CELL + CELL/2,
                   8, GREEN);
    }
    DrawCircle(BOARD_X + e->map.pacStart.c * CELL + CELL/2,
               BOARD_Y + e->map.pacStart.r * CELL + CELL/2,
               17, YELLOW);
    DrawText("P", BOARD_X + e->map.pacStart.c * CELL + 18, BOARD_Y + e->map.pacStart.r * CELL + 12, 22, BLACK);

    for (i = 0; i < NUM_GHOSTS; i++) {
        Color col = (i == 0) ? RED : (i == 1) ? SKYBLUE : (i == 2) ? PINK : ORANGE;
        DrawCircle(BOARD_X + e->map.ghostStart[i].c * CELL + CELL/2,
                   BOARD_Y + e->map.ghostStart[i].r * CELL + CELL/2,
                   16, col);
        DrawText(TextFormat("%d", i+1), BOARD_X + e->map.ghostStart[i].c * CELL + 19,
                 BOARD_Y + e->map.ghostStart[i].r * CELL + 13, 18, BLACK);
    }

    DrawText("EDITOR DE MAPAS", SIDE_X, 35, 24, YELLOW);
    DrawText(TextFormat("Tamanio: %dx%d", e->map.rows, e->map.cols), SIDE_X, 75, 20, RAYWHITE);
    DrawText(TextFormat("Herramienta: %s", toolName), SIDE_X, 105, 20, SKYBLUE);
    if (e->tool == TOOL_PELLET) DrawText(TextFormat("Indice pac-bola: %d", e->pelletIndex), SIDE_X, 130, 20, SKYBLUE);

    DrawText("P: Pac-Man", SIDE_X, 175, 17, LIGHTGRAY);
    DrawText("1-4: fantasmas", SIDE_X, 198, 17, LIGHTGRAY);
    DrawText("B: pac-bola", SIDE_X, 221, 17, LIGHTGRAY);
    DrawText("M: muro permanente", SIDE_X, 244, 17, LIGHTGRAY);
    DrawText("Click: colocar", SIDE_X, 267, 17, LIGHTGRAY);
    DrawText("S: guardar custom_map.txt", SIDE_X, 290, 17, LIGHTGRAY);
    DrawText("+/-: filas", SIDE_X, 313, 17, LIGHTGRAY);
    DrawText("[/]: columnas", SIDE_X, 336, 17, LIGHTGRAY);
    DrawText("ESC: volver", SIDE_X, 359, 17, LIGHTGRAY);

    if (e->messageTime > 0) DrawText(e->message, 40, 560, 18, RAYWHITE);
}
