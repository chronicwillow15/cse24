#include "iGraphics.h"
#include "iSound.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// Game constants
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 50
#define MAX_ROWS 20
#define MAX_COLS 100
#define MAX_NAME_LENGTH 49
#define TOTAL_LEVELS 4
#define FPS 60

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
    STATE_EASTER_EGG
} GameState;

// Game structure
typedef struct {
    // Game state
    GameState currentState;
    
    // Player info
    char playerName[MAX_NAME_LENGTH + 1];
    int nameIndex;
    int score;
    int highScore;
    int lives;
    
    // Level info
    int currentLevel;
    int totalItems;
    char map[MAX_ROWS][MAX_COLS];
    int mapRows;
    int mapCols;
    
    // Player physics
    float ballX, ballY;
    float ballRadius;
    float ballDY;
    float gravity;
    bool onGround;
    
    // Camera
    float cameraX;
    
    // Enemy
    struct {
        float x, y;
        int dir;
        float speed;
    } enemy;
    
    // Timer
    int levelTime;
    int currentTime;
    
    // Resources
    Image blockImage;
    int jumpSound, itemSound, gameOverSound;
} Game;

Game game;

// Button structure
typedef struct {
    int x, y, w, h;
    const char* text;
} Button;

// Initialize buttons
Button mainMenuButtons[] = {
    {100, 250, 200, 50, "Start Game"},
    {100, 190, 200, 50, "Instructions"},
    {100, 130, 200, 50, "Settings"},
    {100, 70, 200, 50, "Exit"}
};

// Function prototypes
void initializeGame();
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
void drawButtons(Button buttons[], int count);
bool isButtonClicked(Button btn, int mx, int my);

void initializeGame() {
    // Initialize game state
    game.currentState = STATE_ENTER_NAME;
    
    // Initialize player info
    memset(game.playerName, 0, sizeof(game.playerName));
    game.nameIndex = 0;
    game.score = 0;
    game.lives = 3;
    
    // Initialize physics
    game.ballX = 100;
    game.ballY = 300;
    game.ballRadius = 20;
    game.ballDY = 0;
    game.gravity = -0.2f;
    game.onGround = false;
    
    // Initialize camera
    game.cameraX = 0;
    
    // Initialize enemy
    game.enemy.x = 300;
    game.enemy.y = 300;
    game.enemy.dir = 1;
    game.enemy.speed = 2.0f;
    
    // Initialize timer
    game.levelTime = 5 * 60 * FPS; // 5 minutes
    game.currentTime = 0;
    
    // Initialize level
    game.currentLevel = 1;
    game.totalItems = 0;
    
    // Load resources
    if (!iLoadImage(&game.blockImage, "block.bmp")) {
        printf("Failed to load block image!\n");
        // Handle error or use fallback drawing
    }
    
    // Load sounds (commented out as they may not be available)
    // game.jumpSound = iLoadSound("jump.wav");
    // game.itemSound = iLoadSound("item.wav");
    // game.gameOverSound = iLoadSound("gameover.wav");
    
    loadHighScore();
}

void loadHighScore() {
    FILE *f = fopen("highscore.txt", "r");
    if (f) {
        fscanf(f, "%d", &game.highScore);
        fclose(f);
    }
}

void saveHighScore() {
    if (game.score > game.highScore) {
        FILE *f = fopen("highscore.txt", "w");
        if (f) {
            fprintf(f, "%d", game.score);
            fclose(f);
            game.highScore = game.score;
        }
    }
}

void loadMap(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to load map file: %s\n", filename);
        return;
    }
    
    char line[MAX_COLS];
    game.mapRows = 0;
    game.totalItems = 0;
    
    while (fgets(line, sizeof(line), file) && game.mapRows < MAX_ROWS) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = '\0';
        
        strncpy(game.map[game.mapRows], line, MAX_COLS);
        
        for (int j = 0; j < strlen(line); j++) {
            if (line[j] == '@') {
                game.ballX = j * BLOCK_SIZE + BLOCK_SIZE / 2;
                game.ballY = SCREEN_HEIGHT - (game.mapRows + 1) * BLOCK_SIZE + BLOCK_SIZE / 2;
            }
            if (line[j] == '*') game.totalItems++;
        }
        
        game.mapRows++;
    }
    
    game.mapCols = strlen(game.map[0]);
    fclose(file);
    game.cameraX = 0;
    game.ballDY = 0;
}

void drawMap() {
    for (int i = 0; i < game.mapRows; i++) {
        for (int j = 0; j < game.mapCols; j++) {
            float screenX = j * BLOCK_SIZE - game.cameraX;
            float screenY = SCREEN_HEIGHT - (i + 1) * BLOCK_SIZE;
            
            if (game.map[i][j] == '#') {
                if (game.blockImage.width > 0) {
                    iShowImage(screenX, screenY, game.blockImage);
                } else {
                    // Fallback if image not loaded
                    iSetColor(100, 100, 100);
                    iFilledRectangle(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE);
                }
            } else if (game.map[i][j] == '*') {
                iSetColor(255, 215, 0);
                iFilledCircle(j * BLOCK_SIZE + BLOCK_SIZE / 2 - game.cameraX,
                             SCREEN_HEIGHT - (i + 1) * BLOCK_SIZE + BLOCK_SIZE / 2, 
                             10);
            }
        }
    }
}

void updateCamera() {
    game.cameraX = game.ballX - SCREEN_WIDTH / 2;
    if (game.cameraX < 0) game.cameraX = 0;
    
    float maxCameraX = game.mapCols * BLOCK_SIZE - SCREEN_WIDTH;
    if (game.cameraX > maxCameraX) game.cameraX = maxCameraX;
}

bool isColliding(float x, float y) {
    for (int i = 0; i < game.mapRows; i++) {
        for (int j = 0; j < game.mapCols; j++) {
            if (game.map[i][j] == '#') {
                float blockX = j * BLOCK_SIZE;
                float blockY = SCREEN_HEIGHT - (i + 1) * BLOCK_SIZE;
                
                if (x + game.ballRadius > blockX && 
                    x - game.ballRadius < blockX + BLOCK_SIZE &&
                    y + game.ballRadius > blockY && 
                    y - game.ballRadius < blockY + BLOCK_SIZE) {
                    return true;
                }
            }
        }
    }
    return false;
}

void updatePhysics() {
    float nextY = game.ballY + game.ballDY;
    
    if (!isColliding(game.ballX, nextY)) {
        game.ballY = nextY;
        game.ballDY += game.gravity;
        game.onGround = false;
    } else {
        game.ballDY = 0;
        game.onGround = true;
    }
}

void collectItems() {
    for (int i = 0; i < game.mapRows; i++) {
        for (int j = 0; j < game.mapCols; j++) {
            if (game.map[i][j] == '*') {
                float itemX = j * BLOCK_SIZE + BLOCK_SIZE / 2;
                float itemY = SCREEN_HEIGHT - (i + 1) * BLOCK_SIZE + BLOCK_SIZE / 2;
                float dx = game.ballX - itemX;
                float dy = game.ballY - itemY;
                float distance = sqrt(dx * dx + dy * dy);
                
                if (distance < game.ballRadius + 10) {
                    game.map[i][j] = '.';
                    game.score += 10;
                    game.totalItems--;
                    
                    // Play sound if available
                    // if (game.itemSound) iPlaySound(game.itemSound, false);
                    
                    if (game.totalItems == 0) {
                        game.currentLevel++;
                        if (game.currentLevel > TOTAL_LEVELS) {
                            game.currentState = STATE_VICTORY;
                            saveHighScore();
                        } else {
                            char path[50];
                            sprintf(path, "maps/level%d.txt", game.currentLevel);
                            loadMap(path);
                        }
                    }
                }
            }
        }
    }
}

void updateEnemy() {
    game.enemy.x += game.enemy.speed * game.enemy.dir;
    
    // Simple patrol behavior between 200 and 600
    float leftBound = 200;
    float rightBound = 600;
    
    if (game.enemy.x < leftBound) {
        game.enemy.x = leftBound;
        game.enemy.dir = 1;
    } else if (game.enemy.x > rightBound) {
        game.enemy.x = rightBound;
        game.enemy.dir = -1;
    }
}

bool checkEnemyCollision() {
    float dx = fabs(game.ballX - game.enemy.x);
    float dy = fabs(game.ballY - game.enemy.y);
    float collisionDist = game.ballRadius + 20; // Enemy size approx
    
    return (dx < collisionDist && dy < collisionDist);
}

void resetLevel() {
    game.lives = 3;
    game.score = 0;
    game.currentTime = 0;
    game.currentLevel = 1;
    
    char path[50];
    sprintf(path, "maps/level%d.txt", game.currentLevel);
    loadMap(path);
}

void drawUI() {
    char textBuffer[100];
    
    // Draw score
    sprintf(textBuffer, "Score: %d", game.score);
    iSetColor(255, 255, 255);
    iText(10, SCREEN_HEIGHT - 30, textBuffer, GLUT_BITMAP_HELVETICA_18);
    
    // Draw lives
    sprintf(textBuffer, "Lives: %d", game.lives);
    iText(10, SCREEN_HEIGHT - 60, textBuffer, GLUT_BITMAP_HELVETICA_18);
    
    // Draw player name if available
    if (strlen(game.playerName) > 0) {
        sprintf(textBuffer, "Player: %s", game.playerName);
        iText(10, SCREEN_HEIGHT - 90, textBuffer, GLUT_BITMAP_HELVETICA_18);
    }
    
    // Draw timer
    int timeLeft = (game.levelTime - game.currentTime) / FPS;
    sprintf(textBuffer, "Time Left: %d", timeLeft);
    iText(SCREEN_WIDTH - 150, SCREEN_HEIGHT - 30, textBuffer, GLUT_BITMAP_HELVETICA_18);
}

void drawButtons(Button buttons[], int count) {
    for (int i = 0; i < count; i++) {
        Button btn = buttons[i];
        
        // Draw button background
        iSetColor(0, 100 + i * 50, 200 - i * 50);
        iFilledRectangle(btn.x, btn.y, btn.w, btn.h);
        
        // Draw button text
        iSetColor(0, 0, 0);
        int textWidth = iTextWidth(btn.text, GLUT_BITMAP_HELVETICA_18);
        iText(btn.x + (btn.w - textWidth) / 2, btn.y + btn.h / 2 - 7, btn.text, GLUT_BITMAP_HELVETICA_18);
    }
}

bool isButtonClicked(Button btn, int mx, int my) {
    // Convert mouse Y coordinate to iGraphics coordinate system
    int myInverted = SCREEN_HEIGHT - my;
    return (mx >= btn.x && mx <= btn.x + btn.w && 
            myInverted >= btn.y && myInverted <= btn.y + btn.h);
}

void iDraw() {
    iClear();
    
    switch (game.currentState) {
        case STATE_ENTER_NAME:
            iSetColor(255, 255, 255);
            iText(400, 350, "Enter Player Name:", GLUT_BITMAP_TIMES_ROMAN_24);
            iText(400, 300, game.playerName, GLUT_BITMAP_HELVETICA_18);
            iText(400, 270, "Press Enter to confirm", GLUT_BITMAP_HELVETICA_18);
            break;
            
        case STATE_MAIN_MENU:
            iSetColor(0, 0, 100);
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            iSetColor(255, 255, 255);
            iText(SCREEN_WIDTH/2 - 100, 500, "BOUNCE CLASSIC", GLUT_BITMAP_TIMES_ROMAN_24);
            drawButtons(mainMenuButtons, 4);
            break;
            
        case STATE_LEVEL_SELECTOR: {
            iSetColor(0, 0, 150);
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            
            iSetColor(255, 255, 255);
            iText(SCREEN_WIDTH/2 - 120, 550, "SELECT LEVEL", GLUT_BITMAP_TIMES_ROMAN_24);
            
            // Draw level buttons
            Button levelButtons[TOTAL_LEVELS];
            for (int i = 0; i < TOTAL_LEVELS; i++) {
                levelButtons[i].x = 350 + (i % 2) * 250;
                levelButtons[i].y = 350 - (i / 2) * 100;
                levelButtons[i].w = 200;
                levelButtons[i].h = 50;
                sprintf(levelButtons[i].text, "Level %d", i+1);
            }
            drawButtons(levelButtons, TOTAL_LEVELS);
            
            // Back button
            Button backBtn = {10, 10, 200, 50, "Back to Menu"};
            drawButtons(&backBtn, 1);
            break;
        }
            
        case STATE_GAME:
            // Draw blue background
            iSetColor(0, 0, 200);
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            
            drawMap();
            
            // Draw enemy
            iSetColor(255, 0, 0);
            iFilledRectangle(game.enemy.x - 20 - game.cameraX, game.enemy.y - 20, 40, 40);
            
            // Draw player ball
            iSetColor(255, 255, 255);
            iFilledCircle(game.ballX - game.cameraX, game.ballY, game.ballRadius);
            
            drawUI();
            break;
            
        case STATE_INSTRUCTIONS:
            iSetColor(0, 0, 100);
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            
            iSetColor(255, 255, 255);
            iText(100, 550, "INSTRUCTIONS:", GLUT_BITMAP_TIMES_ROMAN_24);
            iText(100, 500, "- Use LEFT and RIGHT arrow keys to move", GLUT_BITMAP_HELVETICA_18);
            iText(100, 470, "- Press SPACE to jump", GLUT_BITMAP_HELVETICA_18);
            iText(100, 440, "- Collect all items (*) to win", GLUT_BITMAP_HELVETICA_18);
            iText(100, 410, "- Avoid the red enemy blocks", GLUT_BITMAP_HELVETICA_18);
            iText(100, 380, "- Press 'P' to pause the game", GLUT_BITMAP_HELVETICA_18);
            iText(100, 350, "- Press 'B' to return to menu", GLUT_BITMAP_HELVETICA_18);
            
            // Back button
            Button backBtn = {SCREEN_WIDTH/2 - 100, 50, 200, 50, "Back to Menu"};
            drawButtons(&backBtn, 1);
            break;
            
        case STATE_SETTINGS:
            iSetColor(0, 0, 100);
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            
            iSetColor(255, 255, 255);
            iText(SCREEN_WIDTH/2 - 100, 400, "SETTINGS MENU", GLUT_BITMAP_TIMES_ROMAN_24);
            iText(SCREEN_WIDTH/2 - 150, 350, "Coming in future update!", GLUT_BITMAP_HELVETICA_18);
            
            // Back button
            backBtn = {SCREEN_WIDTH/2 - 100, 50, 200, 50, "Back to Menu"};
            drawButtons(&backBtn, 1);
            break;
            
        case STATE_PAUSE:
            iSetColor(0, 0, 0, 150); // Semi-transparent overlay
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            
            iSetColor(255, 255, 0);
            iText(SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 + 20, "GAME PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
            iText(SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 20, "Press 'R' to resume or 'B' for menu", GLUT_BITMAP_HELVETICA_18);
            break;
            
        case STATE_GAMEOVER:
            iSetColor(0, 0, 0, 200);
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            
            iSetColor(255, 0, 0);
            iText(SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 30, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
            
            char scoreText[50];
            sprintf(scoreText, "Final Score: %d", game.score);
            iText(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 20, scoreText, GLUT_BITMAP_HELVETICA_18);
            
            iText(SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 60, "Press 'B' to return to menu", GLUT_BITMAP_HELVETICA_18);
            break;
            
        case STATE_VICTORY:
            iSetColor(0, 0, 0, 200);
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            
            iSetColor(0, 255, 0);
            iText(SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 + 30, "YOU WIN!", GLUT_BITMAP_TIMES_ROMAN_24);
            
            sprintf(scoreText, "Final Score: %d", game.score);
            iText(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 20, scoreText, GLUT_BITMAP_HELVETICA_18);
            
            if (game.score == game.highScore) {
                iText(SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 - 60, "NEW HIGH SCORE!", GLUT_BITMAP_HELVETICA_18);
            }
            
            iText(SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 100, "Press 'B' to return to menu", GLUT_BITMAP_HELVETICA_18);
            break;
            
        case STATE_EASTER_EGG:
            iSetColor(0, 0, 0, 200);
            iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            
            iSetColor(255, 255, 0);
            iText(SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 30, "EASTER EGG FOUND!", GLUT_BITMAP_TIMES_ROMAN_24);
            iText(SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 20, "Congratulations, you found the secret!", GLUT_BITMAP_HELVETICA_18);
            iText(SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 60, "Press 'B' to return", GLUT_BITMAP_HELVETICA_18);
            break;
    }
}

void iMouse(int button, int state, int mx, int my) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        switch (game.currentState) {
            case STATE_MAIN_MENU:
                for (int i = 0; i < 4; i++) {
                    if (isButtonClicked(mainMenuButtons[i], mx, my)) {
                        switch (i) {
                            case 0: game.currentState = STATE_LEVEL_SELECTOR; break;
                            case 1: game.currentState = STATE_INSTRUCTIONS; break;
                            case 2: game.currentState = STATE_SETTINGS; break;
                            case 3: exit(0); break;
                        }
                    }
                }
                break;
                
            case STATE_LEVEL_SELECTOR: {
                // Check level buttons
                Button levelButtons[TOTAL_LEVELS];
                for (int i = 0; i < TOTAL_LEVELS; i++) {
                    levelButtons[i].x = 350 + (i % 2) * 250;
                    levelButtons[i].y = 350 - (i / 2) * 100;
                    levelButtons[i].w = 200;
                    levelButtons[i].h = 50;
                    
                    if (isButtonClicked(levelButtons[i], mx, my)) {
                        game.currentLevel = i + 1;
                        resetLevel();
                        game.currentState = STATE_GAME;
                        return;
                    }
                }
                
                // Check back button
                Button backBtn = {10, 10, 200, 50, ""};
                if (isButtonClicked(backBtn, mx, my)) {
                    game.currentState = STATE_MAIN_MENU;
                }
                break;
            }
                
            case STATE_INSTRUCTIONS:
            case STATE_SETTINGS: {
                Button backBtn = {SCREEN_WIDTH/2 - 100, 50, 200, 50, ""};
                if (isButtonClicked(backBtn, mx, my)) {
                    game.currentState = STATE_MAIN_MENU;
                }
                break;
            }
                
            default:
                break;
        }
    }
}

void iKeyboard(unsigned char key) {
    if (game.currentState == STATE_ENTER_NAME) {
        if (key == '\r') { // Enter key
            if (game.nameIndex > 0) {
                game.currentState = STATE_MAIN_MENU;
            }
        } 
        else if (key == '\b' || key == 127) { // Backspace
            if (game.nameIndex > 0) {
                game.playerName[--game.nameIndex] = '\0';
            }
        }
        else if (game.nameIndex < MAX_NAME_LENGTH && 
                ((key >= 'A' && key <= 'Z') || 
                 (key >= 'a' && key <= 'z') || 
                 (key >= '0' && key <= '9') || 
                 key == ' ')) {
            game.playerName[game.nameIndex++] = key;
            game.playerName[game.nameIndex] = '\0';
        }
        return;
    }

    // Common keys for multiple states
    switch (key) {
        case 'b':
        case 'B':
            if (game.currentState == STATE_GAME || 
                game.currentState == STATE_PAUSE || 
                game.currentState == STATE_GAMEOVER || 
                game.currentState == STATE_VICTORY || 
                game.currentState == STATE_INSTRUCTIONS || 
                game.currentState == STATE_SETTINGS || 
                game.currentState == STATE_EASTER_EGG) {
                
                resetLevel();
                game.currentState = STATE_MAIN_MENU;
            } 
            else if (game.currentState == STATE_LEVEL_SELECTOR) {
                game.currentState = STATE_MAIN_MENU;
            }
            break;
            
        case 'p':
        case 'P':
            if (game.currentState == STATE_GAME) {
                iPauseTimer(0);
                game.currentState = STATE_PAUSE;
            }
            break;
            
        case 'r':
        case 'R':
            if (game.currentState == STATE_PAUSE) {
                iResumeTimer(0);
                game.currentState = STATE_GAME;
            }
            break;
            
        case ' ':
            if (game.currentState == STATE_GAME && game.onGround) {
                game.ballDY = 8;
                game.onGround = false;
                // if (game.jumpSound) iPlaySound(game.jumpSound, false);
            }
            break;
            
        case 'e':
        case 'E':
            // Easter egg secret key
            if (game.currentState == STATE_MAIN_MENU) {
                game.currentState = STATE_EASTER_EGG;
            }
            break;
    }
}

void iSpecialKeyboard(int key) {
    if (game.currentState == STATE_GAME) {
        switch (key) {
            case GLUT_KEY_LEFT: {
                float nextX = game.ballX - 10;
                if (!isColliding(nextX, game.ballY)) game.ballX = nextX;
                break;
            }
                
            case GLUT_KEY_RIGHT: {
                float nextX = game.ballX + 10;
                if (!isColliding(nextX, game.ballY)) game.ballX = nextX;
                break;
            }
        }
    }
}

void timer() {
    if (game.currentState == STATE_GAME) {
        updatePhysics();
        updateCamera();
        collectItems();
        updateEnemy();

        if (checkEnemyCollision()) {
            game.lives--;
            // if (game.gameOverSound) iPlaySound(game.gameOverSound, false);

            if (game.lives <= 0) {
                game.currentState = STATE_GAMEOVER;
                saveHighScore();
            } else {
                // Reset player position after hit
                game.ballX = 100;
                game.ballY = 300;
                game.ballDY = 0;
                game.onGround = false;
            }
        }

        game.currentTime++;
        if (game.currentTime >= game.levelTime) {
            game.currentState = STATE_GAMEOVER;
            saveHighScore();
        }
    }
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    
    // Initialize sound system
    iInitializeSound();
    
    // Initialize game
    initializeGame();
    
    // Load first level
    char path[50];
    sprintf(path, "maps/level%d.txt", game.currentLevel);
    loadMap(path);
    
    // Set up timer (60 FPS)
    iSetTimer(1000/FPS, timer);
    
    // Start the game
    iInitialize(SCREEN_WIDTH, SCREEN_HEIGHT, "Bounce Classic");
    
    return 0;
}