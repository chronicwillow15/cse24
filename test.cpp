#include "iGraphics.h"
#include "iSound.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// Enum for game states
typedef enum {
    STATE_ENTER_NAME,
    STATE_MAIN_MENU,
    STATE_LEVEL_SELECTOR,
    STATE_GAME,
    STATE_INSTRUCTIONS,
    STATE_SETTINGS,
    STATE_PAUSE,
    STATE_GAMEOVER,
    STATE_VICTORY,
    STATE_EASTER_EGG,
    STATE_EXIT
} GameState;

GameState currentState = STATE_ENTER_NAME;

// Window size
int screenWidth = 1000;
int screenHeight = 600;

// Ball
float ballX = 100, ballY = 300;
float ballRadius = 20;
float ballDY = 0;
float gravity = -0.2;
bool onGround = false;

// Camera
float cameraX = 0;

// Buttons for menus
int btnX = 100, btnY = 100, btnW = 200, btnH = 50, gap = 20;

// Score and lives
int score = 0;
int lives = 3;
char scoreText[100];

// Map
#define MAX_ROWS 20
#define MAX_COLS 100
char map[MAX_ROWS][MAX_COLS];
int mapRows = 0, mapCols = 0;
int blockWidth = 50, blockHeight = 50;
Image blockImage; 

// Sounds
int jumpSound, itemSound, gameOverSound;

// Player Name Entry
char playerName[50] = "";
int nameIndex = 0;

// Timer
int levelTime = 60 * 5 * 60; // 5 minutes * 60 FPS ticks
int currentTime = 0;

// High Score
int highScore = 0;

// Level
int currentLevel = 1;
int totalItems = 0;
int totalLevels = 4;

// Enemy
typedef struct {
    float x, y;
    int dir;  // 1 = right, -1 = left
    float speed;
} Enemy;
Enemy enemy;

// Function prototypes
void loadHighScore();
void saveHighScore();
void loadMap(const char *filename);
void drawMap();
void updateCamera();
bool isColliding(float x, float y);
void updatePhysics();
void collectItems();
void updateEnemy();
bool checkEnemyCollision();
void resetLevel();
void drawUI();

void loadHighScore() {
    FILE *f = fopen("highscore.txt", "r");
    if (f) {
        fscanf(f, "%d", &highScore);
        fclose(f);
    }
}

void saveHighScore() {
    if (score > highScore) {
        FILE *f = fopen("highscore.txt", "w");
        if(f) {
            fprintf(f, "%d", score);
            fclose(f);
        }
    }
}

void loadMap(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return;
    char line[MAX_COLS];
    mapRows = 0;
    totalItems = 0;
    while (fgets(line, sizeof(line), file) && mapRows < MAX_ROWS) {
        int len = strlen(line);
        if (line[len - 1] == '\n') line[len - 1] = '\0';
        strcpy(map[mapRows], line);
        for (int j = 0; j < strlen(line); j++) {
            if (line[j] == '@') {
                ballX = j * blockWidth + blockWidth / 2;
                ballY = screenHeight - (mapRows + 1) * blockHeight + blockHeight / 2;
            }
            if (line[j] == '*') totalItems++;
        }
        mapRows++;
    }
    mapCols = strlen(map[0]);
    fclose(file);
    cameraX = 0;
    ballDY = 0;

    // Initialize enemy on level start (example fixed position)
    enemy.x = 300;
    enemy.y = 300;
    enemy.dir = 1;
    enemy.speed = 2.0f;
}

 void drawMap() {
    for (int i = 0; i < mapRows; i++) {
        size_t len = strlen(map[i]);
        for (size_t j = 0; j < len; j++) {
            if (map[i][j] == '#') {
                iShowImage(j * blockWidth - cameraX, screenHeight - (i + 1) * blockHeight , "block.jpg");
            } else if (map[i][j] == '*') {
                iSetColor(255, 215, 0);
                iFilledCircle(j * blockWidth + blockWidth / 2 - cameraX,
                              screenHeight - (i + 1) * blockHeight + blockHeight / 2, 10);
            }
        }
    }
}



void updateCamera() {
    cameraX = ballX - screenWidth / 2;
    if (cameraX < 0) cameraX = 0;
    float maxCameraX = mapCols * blockWidth - screenWidth;
    if (cameraX > maxCameraX) cameraX = maxCameraX;
}

bool isColliding(float x, float y) {
    for (int i = 0; i < mapRows; i++) {
        for (int j = 0; j < mapCols; j++) {
            if (map[i][j] == '#') {
                float bx = j * blockWidth;
                float by = screenHeight - (i + 1) * blockHeight;
                if (x + ballRadius > bx && x - ballRadius < bx + blockWidth &&
                    y + ballRadius > by && y - ballRadius < by + blockHeight) {
                    return true;
                }
            }
        }
    }
    return false;
}

void updatePhysics() {
    float nextY = ballY + ballDY;
    if (!isColliding(ballX, nextY)) {
        ballY = nextY;
        ballDY += gravity;
        onGround = false;
    } else {
        ballDY = 0;
        onGround = true;
    }
}

void collectItems() {
    for (int i = 0; i < mapRows; i++) {
        for (int j = 0; j < mapCols; j++) {
            if (map[i][j] == '*') {
                float cx = j * blockWidth + blockWidth / 2;
                float cy = screenHeight - (i + 1) * blockHeight + blockHeight / 2;
                float dx = ballX - cx;
                float dy = ballY - cy;
                if (sqrt(dx * dx + dy * dy) < ballRadius + 10) {
                    map[i][j] = '.';
                    score += 10;
                    totalItems--;

                    // Play item collection sound
                   // iPlaySound(itemSound, false);

                    if (totalItems == 0) {
                        currentLevel++;
                        if (currentLevel > totalLevels) {
                            currentState = STATE_VICTORY;
                            saveHighScore();
                        } else {
                            char path[50];
                            sprintf(path, "maps/level%d.txt", currentLevel);
                            loadMap(path);
                        }
                    }
                }
            }
        }
    }
}

void updateEnemy() {
    enemy.x += enemy.speed * enemy.dir;
    // Move enemy back and forth between two X positions (example)
    float leftBound = 200;
    float rightBound = 600;
    if (enemy.x < leftBound) {
        enemy.x = leftBound;
        enemy.dir = 1;
    } else if (enemy.x > rightBound) {
        enemy.x = rightBound;
        enemy.dir = -1;
    }
}

bool checkEnemyCollision() {
    float dx = fabs(ballX - enemy.x);
    float dy = fabs(ballY - enemy.y);
    float collisionDist = ballRadius + 20;  // 20 is enemy size approx
    if (dx < collisionDist && dy < collisionDist) {
        return true;
    }
    return false;
}

void resetLevel() {
    lives = 3;
    score = 0;
    currentTime = 0;
    currentLevel = 1;
    char path[50];
    sprintf(path, "maps/level%d.txt", currentLevel);
    loadMap(path);
}

void drawUI() {
    // Draw score
    sprintf(scoreText, "Score: %d", score);
    iSetColor(255, 255, 255);
    iText(10, screenHeight - 30, scoreText, GLUT_BITMAP_HELVETICA_18);

    // Draw lives
    char lifeText[20];
    sprintf(lifeText, "Lives: %d", lives);
    iText(10, screenHeight - 60, lifeText, GLUT_BITMAP_HELVETICA_18);

    // Draw player name
    if (strlen(playerName) > 0) {
        char nameText[60];
        sprintf(nameText, "Player: %s", playerName);
        iText(10, screenHeight - 90, nameText, GLUT_BITMAP_HELVETICA_18);
    }

    // Draw timer
    int timeLeft = (levelTime - currentTime) / 60;  // assuming 60 ticks = 1 sec
    char timerText[30];
    sprintf(timerText, "Time Left: %d", timeLeft);
    iText(screenWidth - 150, screenHeight - 30, timerText, GLUT_BITMAP_HELVETICA_18);
}

// Main draw function
void iDraw() {
    iClear();

    if (currentState == STATE_ENTER_NAME) {
        iSetColor(255, 255, 255);
        iText(400, 350, "Enter Player Name:", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(400, 300, playerName, GLUT_BITMAP_HELVETICA_18);
        iText(400, 270, "Press Enter to confirm", GLUT_BITMAP_HELVETICA_18);
    }
    else if (currentState == STATE_MAIN_MENU) {
        // Draw buttons for menu
        iSetColor(0, 255, 0);
        iFilledRectangle(btnX, btnY + 3*(btnH + gap), btnW, btnH);
        iSetColor(0, 0, 0);
        iText(btnX + 70, btnY + 3*(btnH + gap) + 15, "Start Game", GLUT_BITMAP_HELVETICA_18);

        iSetColor(0, 200, 255);
        iFilledRectangle(btnX, btnY + 2*(btnH + gap), btnW, btnH);
        iSetColor(0, 0, 0);
        iText(btnX + 70, btnY + 2*(btnH + gap) + 15, "Instructions", GLUT_BITMAP_HELVETICA_18);

        iSetColor(255, 165, 0);
        iFilledRectangle(btnX, btnY + 1*(btnH + gap), btnW, btnH);
        iSetColor(0, 0, 0);
        iText(btnX + 70, btnY + 1*(btnH + gap) + 15, "Settings", GLUT_BITMAP_HELVETICA_18);

        iSetColor(255, 0, 0);
        iFilledRectangle(btnX, btnY, btnW, btnH);
        iSetColor(0, 0, 0);
        iText(btnX + 70, btnY + 15, "Exit", GLUT_BITMAP_HELVETICA_18);
    }
    else if (currentState == STATE_LEVEL_SELECTOR) {
        iSetColor(0, 255, 255);
        iText(400, 550, "Select Level (1 - 4)", GLUT_BITMAP_TIMES_ROMAN_24);
        for (int i = 1; i <= totalLevels; i++) {
            int x = 350 + (i - 1) * (btnW + gap);
            int y = 400;
            iSetColor(100, 200, 255);
            iFilledRectangle(x, y, btnW, btnH);
            iSetColor(0, 0, 0);
            char levelText[20];
            sprintf(levelText, "Level %d", i);
            iText(x + 70, y + 15, levelText, GLUT_BITMAP_HELVETICA_18);
        }
        iSetColor(255, 0, 0);
        iFilledRectangle(10, 10, btnW, btnH);
        iSetColor(0, 0, 0);
        iText(50, 25, "Back to Menu", GLUT_BITMAP_HELVETICA_18);
    }
    else if (currentState == STATE_GAME) {
        // Draw blue background for gameplay
        iSetColor(0, 0, 200);
        iFilledRectangle(0, 0, screenWidth, screenHeight);

        drawMap();

        // Draw enemy as a red square
        iSetColor(255, 0, 0);
        iFilledRectangle(enemy.x - 20 - cameraX, enemy.y - 20, 40, 40);

        // Draw ball
        iSetColor(255, 255, 255);
        iFilledCircle(ballX - cameraX, ballY, ballRadius);

        drawUI();
    }
    else if (currentState == STATE_INSTRUCTIONS) {
        iSetColor(255, 255, 255);
        iText(100, 550, "Instructions:", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(100, 510, "- Use LEFT and RIGHT arrow keys to move.", GLUT_BITMAP_HELVETICA_18);
        iText(100, 480, "- Press SPACE to jump.", GLUT_BITMAP_HELVETICA_18);
        iText(100, 450, "- Collect all items (*) to win.", GLUT_BITMAP_HELVETICA_18);
        iText(100, 420, "- Avoid the red enemy blocks.", GLUT_BITMAP_HELVETICA_18);
        iText(100, 390, "- Press 'b' to return to menu.", GLUT_BITMAP_HELVETICA_18);
    }
    else if (currentState == STATE_SETTINGS) {
        iSetColor(255, 255, 255);
        iText(100, 550, "Settings (not implemented)", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(100, 510, "Press 'b' to return to menu.", GLUT_BITMAP_HELVETICA_18);
    }
    else if (currentState == STATE_PAUSE) {
        iSetColor(255, 255, 0);
        iText(screenWidth/2 - 70, screenHeight/2 + 20, "Game Paused", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(screenWidth/2 - 120, screenHeight/2 - 20, "Press 'r' to Resume or 'b' to Menu", GLUT_BITMAP_HELVETICA_18);
    }
    else if (currentState == STATE_GAMEOVER) {
        iSetColor(255, 0, 0);
        iText(screenWidth/2 - 100, screenHeight/2, "Game Over! Press 'b' to return.", GLUT_BITMAP_TIMES_ROMAN_24);
    }
    else if (currentState == STATE_VICTORY) {
        iSetColor(0, 255, 0);
        iText(screenWidth/2 - 70, screenHeight/2, "You Win! Press 'b' to return.", GLUT_BITMAP_TIMES_ROMAN_24);
    }
    else if (currentState == STATE_EASTER_EGG) {
        iSetColor(255, 255, 0);
        iText(screenWidth/2 - 120, screenHeight/2, "You found the Easter Egg!", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(screenWidth/2 - 120, screenHeight/2 - 40, "Press 'b' to return.", GLUT_BITMAP_HELVETICA_18);
    }
}

void iMouse(int button, int state, int mx, int my) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Note: In iGraphics, y-axis is flipped in mouse coords (0 at top-left), so adjust accordingly
        int my_inverted = screenHeight - my;

        if (currentState == STATE_MAIN_MENU) {
            if (mx >= btnX && mx <= btnX + btnW && my_inverted >= btnY + 3*(btnH + gap) && my_inverted <= btnY + 3*(btnH + gap) + btnH) {
                currentState = STATE_LEVEL_SELECTOR;
            }
            else if (mx >= btnX && mx <= btnX + btnW && my_inverted >= btnY + 2*(btnH + gap) && my_inverted <= btnY + 2*(btnH + gap) + btnH) {
                currentState = STATE_INSTRUCTIONS;
            }
            else if (mx >= btnX && mx <= btnX + btnW && my_inverted >= btnY + 1*(btnH + gap) && my_inverted <= btnY + 1*(btnH + gap) + btnH) {
                currentState = STATE_SETTINGS;
            }
            else if (mx >= btnX && mx <= btnX + btnW && my_inverted >= btnY && my_inverted <= btnY + btnH) {
                exit(0);
            }
        }
        else if (currentState == STATE_LEVEL_SELECTOR) {
            for (int i = 1; i <= totalLevels; i++) {
                int x = 350 + (i - 1) * (btnW + gap);
                int y = 400;
                if (mx >= x && mx <= x + btnW && my_inverted >= y && my_inverted <= y + btnH) {
                    currentLevel = i;
                    resetLevel();
                    currentState = STATE_GAME;
                }
            }
            if (mx >= 10 && mx <= 10 + btnW && my_inverted >= 10 && my_inverted <= 10 + btnH) {
                currentState = STATE_MAIN_MENU;
            }
        }
    }
}

void iKeyboard(unsigned char key) {
    if (currentState == STATE_ENTER_NAME) {
        if (key == '\r') { // Enter key
            if (nameIndex > 0) {
                currentState = STATE_MAIN_MENU;
            }
        } else if (key == '\b' || key == 127) { // Backspace key
            if (nameIndex > 0) {
                nameIndex--;
                playerName[nameIndex] = '\0';
            }
        } else if (nameIndex < (int)sizeof(playerName) - 1) {
            playerName[nameIndex++] = key;
            playerName[nameIndex] = '\0';
        }
        return;
    }

    if (key == 'b' || key == 'B') {
        if (currentState == STATE_GAME || currentState == STATE_PAUSE || currentState == STATE_GAMEOVER || currentState == STATE_VICTORY || currentState == STATE_INSTRUCTIONS || currentState == STATE_SETTINGS || currentState == STATE_EASTER_EGG) {
            currentState = STATE_MAIN_MENU;
            resetLevel();
        } else if (currentState == STATE_LEVEL_SELECTOR) {
            currentState = STATE_MAIN_MENU;
        }
    }
    else if (key == 'p' || key == 'P') {
        if (currentState == STATE_GAME) {
            iPauseTimer(0);
            currentState = STATE_PAUSE;
        }
    }
    else if (key == 'r' || key == 'R') {
        if (currentState == STATE_PAUSE) {
            iResumeTimer(0);
            currentState = STATE_GAME;
        }
    }
    else if (key == ' ' && onGround && currentState == STATE_GAME) {
        ballDY = 8;
        onGround = false;
       // iPlaySound(jumpSound, false);
    }
}

void iSpecialKeyboard(int key) {
    if (currentState == STATE_GAME) {
        if (key == GLUT_KEY_LEFT) {
            float nextX = ballX - 10;
            if (!isColliding(nextX, ballY)) ballX = nextX;
        }
        else if (key == GLUT_KEY_RIGHT) {
            float nextX = ballX + 10;
            if (!isColliding(nextX, ballY)) ballX = nextX;
        }
    }
}

void timer() {
    if (currentState == STATE_GAME) {
        updatePhysics();
        updateCamera();
        collectItems();
        updateEnemy();

        if (checkEnemyCollision()) {
            lives--;
           // iPlaySound(gameOverSound, false);

            if (lives <= 0) {
                currentState = STATE_GAMEOVER;
                saveHighScore();
            } else {
                ballX = 100;
                ballY = 300;
                ballDY = 0;
                onGround = false;
            }
        }

        currentTime++;
        if (currentTime >= levelTime) {
            currentState = STATE_GAMEOVER;
            saveHighScore();
        }
    }
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);

    iInitializeSound();

    
iLoadImage(&blockImage, "block.jpg"); 
    loadHighScore();

    char path[50];
    sprintf(path, "maps/level_%d.txt", currentLevel);
    loadMap(path);

    iSetTimer(17, timer);
    iInitialize(screenWidth, screenHeight, "Bounce Classic");

    return 0;
}
