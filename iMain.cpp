#include "iGraphics.h"
#include "iSound.h"

// defining game state
typedef enum
{
    STATE_MAIN_MENU,
    STATE_GAME,
    STATE_INSTRUCTIONS,
    STATE_SETTINGS,
    STATE_PAUSE,
    STATE_GAMEOVER,
    STATE_VICTORY,
    STATE_EXIT,
    STATE_GAME_OVER,
} GameState;
GameState currentState = STATE_MAIN_MENU;
// Sound
bool bgmplaying = false;

// collector
#define collector_count 5
int collectorX[collector_count] = {152, 352, 552, 752, 902};
int collectorY[collector_count] = {125, 205, 285, 365, 445};
int collectorsize = 20;
bool collectorvisible[collector_count] = {true, true, true};
// score
int score = 0;
char scoreText[20];
// button
int btnX = 100, btnY = 100, btnW = 200, btnH = 50, gap = 20;
;
// ball
float ballX = 100, ballY = 400;
float ballRadius = 20;
float ballDY = 0;
float gravity = -0.2;
bool onGround = false;
#define PLATFORM_COUNT 5
int platformX[PLATFORM_COUNT] = {100, 300, 500, 700, 850};
int platformY[PLATFORM_COUNT] = {100, 180, 260, 340, 420};
int platformW = 120, platformH = 20;

// Spike
int spikeX = 600, spikeY = 260;
int spikeW = 30, spikeH = 30;

// moving enemy
int enemyX = 300, enemyY = 260;
int enemyW = 30, enemyH = 30;
int enemyDir = 1; // Direction: 1 = right, -1 = left
int enemySpeed = 2;
int enemyRange = 100;  // Patrol distance
int enemyStartX = 300; // Starting X

// Victory area
int goalX = 970, goalY = 460, goalW = 40, goalH = 20;

void updateBall()
{
    ballDY += gravity;
    ballY += ballDY;
    onGround = false;
    // Collectibles collision
    for (int i = 0; i < collector_count; i++)
    {
        if (collectorvisible[i])
        {
            int dx = ballX - collectorX[i];
            int dy = ballY - collectorY[i];
            if (dx * dx + dy * dy <= (ballRadius + collectorsize / 2) * (ballRadius + collectorsize / 2))
            {
                collectorvisible[i] = false;
                score += 50;
                iPlaySound("assets/sounds/chime.wav", false);
            }
        }
    }

    // Platform collision
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        if (ballY - ballRadius <= platformY[i] + platformH &&
            ballY - ballRadius >= platformY[i] &&
            ballX >= platformX[i] &&
            ballX <= platformX[i] + platformW)
        {
            ballY = platformY[i] + platformH + ballRadius;
            ballDY = 0;
            onGround = true;
        }
    }

    // Ground check
    if (ballY <= 0)
    {
        currentState = STATE_GAME_OVER;
    }

    // Enemy collision
    if (ballX + ballRadius >= enemyX &&
        ballX - ballRadius <= enemyX + enemyW &&
        ballY + ballRadius >= enemyY &&
        ballY - ballRadius <= enemyY + enemyH)
    {
        currentState = STATE_GAME_OVER;
    }

    // Spike collision
    if (ballX + ballRadius >= spikeX &&
        ballX - ballRadius <= spikeX + spikeW &&
        ballY + ballRadius >= spikeY &&
        ballY - ballRadius <= spikeY + spikeH)
    {
        currentState = STATE_GAME_OVER;
    }

    // Enemy collision
    if (ballX + ballRadius >= enemyX && ballX - ballRadius <= enemyX + enemyW &&
        ballY + ballRadius >= enemyY && ballY - ballRadius <= enemyY + enemyH)
    {
        currentState = STATE_GAME_OVER;
    }

    // Win condition
    if (ballX >= goalX &&
        ballX <= goalX + goalW &&
        ballY >= goalY &&
        ballY <= goalY + goalH)
    {
        currentState = STATE_VICTORY;
    }
}

void updateEnemy(); // Function declaration

void updateEnemy()
{
    enemyX += enemyDir * enemySpeed;
    if (enemyX > enemyStartX + enemyRange || enemyX < enemyStartX)
    {
        enemyDir *= -1;
    }
}

/*
function iDraw() is called again and again by the system.
*/
void iDraw()
{
    // place your drawing codes here
    iClear();
    iShowImage(0, 0, "wallpaper/wallpaper.bmp");
    if (!bgmplaying)
    {
        iPlaySound("assets/sounds/game_audio.wav", true);
        bgmplaying = true;
    }
    if (currentState == STATE_MAIN_MENU)
    {
        iSetColor(0, 255, 0);
        iFilledRectangle(btnX, btnY, btnW, btnH);
        iSetColor(0, 0, 0);
        iText(btnX + 70, btnY + 15, "Start", GLUT_BITMAP_HELVETICA_18);

        iSetColor(0, 200, 255);
        iFilledRectangle(btnX + 1 * (btnW + gap), btnY, btnW, btnH);
        iSetColor(0, 0, 0);
        iText(btnX + 1 * (btnW + gap) + 40, btnY + 15, "Instructions", GLUT_BITMAP_HELVETICA_18);

        iSetColor(255, 165, 0);
        iFilledRectangle(btnX + 2 * (btnW + gap), btnY, btnW, btnH);
        iSetColor(0, 0, 0);
        iText(btnX + 2 * (btnW + gap) + 60, btnY + 15, "Settings", GLUT_BITMAP_HELVETICA_18);

        iSetColor(255, 0, 0);
        iFilledRectangle(btnX + 3 * (btnW + gap), btnY, btnW, btnH);
        iSetColor(0, 0, 0);
        iText(btnX + 3 * (btnW + gap) + 70, btnY + 15, "Exit", GLUT_BITMAP_HELVETICA_18);
    }

    else if (currentState == STATE_GAME)
    {
        iClear();

        iShowImage(0, 0, "wallpaper/level1.bmp");
        updateBall();
        updateEnemy();
        iRectangle(0, 0, 1000, 600);
        // collector
        iSetColor(234, 213, 45);
        for (int i = 0; i < collector_count; i++)
        {
            if (collectorvisible[i])
            {
                iFilledCircle(collectorX[i], collectorY[i], collectorsize / 2);
            }
        }

        // score board
        sprintf(scoreText, "Score: %d", score);
        iSetColor(234, 123, 33);
        iText(850, 560, scoreText, GLUT_BITMAP_HELVETICA_18);
        // final score
        char finalScoreText[50];
        sprintf(finalScoreText, "Final Score: %d", score);
        iSetColor(255, 255, 255);
        iText(40, 40, finalScoreText, GLUT_BITMAP_HELVETICA_18);

        // Draw Ball
        iSetColor(255, 0, 0);
        iFilledCircle(ballX, ballY, ballRadius);

        // Draw Platforms
        iSetColor(150, 75, 0);
        for (int i = 0; i < PLATFORM_COUNT; i++)
        {
            iFilledRectangle(platformX[i], platformY[i], platformW, platformH);
        }

        // Draw Enemy
        iSetColor(0, 0, 255);
        iFilledRectangle(enemyX, enemyY, enemyW, enemyH);

        // Spike
        double x[3] = {(double)spikeX, (double)(spikeX + spikeW / 2), (double)(spikeX + spikeW)};
        double y[3] = {(double)spikeY, (double)(spikeY + spikeH), (double)spikeY};

        iSetColor(255, 0, 255);
        iFilledPolygon(x, y, 3);

        // Draw Goal
        iSetColor(0, 255, 0);
        iCircle(goalX, goalY, goalH);

        // Enemy
        iSetColor(244, 54, 123);
        iFilledRectangle(enemyX, enemyY, enemyW, enemyH);

        // line on screen
        iSetColor(0, 0, 0);
        iText(10, 460, "Use arrow keys to move, space to jump. Press 'b' to go back.", GLUT_BITMAP_HELVETICA_12);
    }
    else if (currentState == STATE_INSTRUCTIONS)
    {
        iSetColor(0, 0, 0);
        iText(100, 400, "Instructions:", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(100, 370, "- Use LEFT and RIGHT arrow keys to move the paddle.", GLUT_BITMAP_HELVETICA_18);
        iText(100, 340, "- Prevent the ball from falling below.", GLUT_BITMAP_HELVETICA_18);
        iText(100, 310, "- Press 'b' to return to the Main Menu.", GLUT_BITMAP_HELVETICA_18);
    }

    else if (currentState == STATE_SETTINGS)
    {
        iSetColor(0, 0, 0);
        iText(100, 400, "Settings (to be implemented)", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(100, 360, "Press 'b' to return to the Main Menu.", GLUT_BITMAP_HELVETICA_18);
    }

    else if (currentState == STATE_GAME_OVER)
    {
        iSetColor(255, 0, 0);
        iText(200, 250, "Game Over! Press 'b' to return.", GLUT_BITMAP_TIMES_ROMAN_24);
    }
    else if (currentState == STATE_PAUSE)
    {
        iSetColor(223, 168, 32);
        iText(420, 300, "Game Paused", GLUT_BITMAP_TIMES_ROMAN_24);
        iText(370, 260, "Press 'r' to Resume or 'b' to go to Menu.", GLUT_BITMAP_HELVETICA_18);
    }

    else if (currentState == STATE_VICTORY)
    {
        iSetColor(0, 255, 0);
        iText(200, 250, "You Win! Press 'b' to return.", GLUT_BITMAP_TIMES_ROMAN_24);
    }
}

/*
function iMouseMove() is called when the user moves the mouse.
(mx, my) is the position where the mouse pointer is.
*/
void iMouseMove(int mx, int my)
{
    // place your codes here
}

/*
function iMouseDrag() is called when the user presses and drags the mouse.
(mx, my) is the position where the mouse pointer is.
*/
void iMouseDrag(int mx, int my)
{
    // place your codes here
}

/*
function iMouse() is called when the user presses/releases the mouse.
(mx, my) is the position where the mouse pointer is.
*/
void iMouse(int button, int state, int mx, int my)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        if (currentState == STATE_MAIN_MENU)
        {
            // Start
            if (mx >= btnX && mx <= btnX + btnW && my >= btnY && my <= btnY + btnH)
            {
                currentState = STATE_GAME;
                score = 0;
                for (int i = 0; i < collector_count; i++)
                {
                    collectorvisible[i] = true;
                }
            }
            // Instructions
            else if (mx >= btnX + 1 * (btnW + gap) && mx <= btnX + 1 * (btnW + gap) + btnW &&
                     my >= btnY && my <= btnY + btnH)
            {
                currentState = STATE_INSTRUCTIONS;
            }
            // Settings
            else if (mx >= btnX + 2 * (btnW + gap) && mx <= btnX + 2 * (btnW + gap) + btnW &&
                     my >= btnY && my <= btnY + btnH)
            {
                currentState = STATE_SETTINGS;
            }
            // Exit
            else if (mx >= btnX + 3 * (btnW + gap) && mx <= btnX + 3 * (btnW + gap) + btnW &&
                     my >= btnY && my <= btnY + btnH)
            {
                exit(0);
            }
        }
    }
}

/*
function iMouseWheel() is called when the user scrolls the mouse wheel.
dir = 1 for up, -1 for down.
*/
void iMouseWheel(int dir, int mx, int my)
{
    // place your code here
}

/*
function iKeyboard() is called whenever the user hits a key in keyboard.
key- holds the ASCII value of the key pressed.
*/
void iKeyboard(unsigned char key)
{
    if (key == 'b')
    {
        currentState = STATE_MAIN_MENU;
        ballX = 100;
        ballY = 300;
        ballDY = 0;
    }
    if (key == 'p' || key == 'P')
    {
        iPauseTimer(0);
        // if (currentState == STATE_GAME) currentState = STATE_PAUSE;
    }
    if (key == ' ' && onGround)
    {
        ballDY = 8;
        onGround = false;
    }
    if (key == 'r' || key == 'R')
    {
        iResumeTimer(0);
        // if (currentState == STATE_PAUSE) currentState = STATE_GAME;
    }
}

/*
function iSpecialKeyboard() is called whenver user hits special keys likefunction
keys, home, end, pg up, pg down, arraows etc. you have to use
appropriate constants to detect them. A list is:
GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6,
GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11,
GLUT_KEY_F12, GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN,
GLUT_KEY_PAGE_UP, GLUT_KEY_PAGE_DOWN, GLUT_KEY_HOME, GLUT_KEY_END,
GLUT_KEY_INSERT */
void iSpecialKeyboard(unsigned char key)
{
    if (key == GLUT_KEY_LEFT)
    {
        ballX -= 10;
    }
    if (key == GLUT_KEY_RIGHT)
    {
        ballX += 10;
    }
}
void iTimer()
{
    if (currentState == STATE_GAME)
    {
        updateBall();
        updateEnemy();
        /*static int frameCount = 0;
         frameCount++;
         if (frameCount >= 100) {
             score += 10;
             frameCount = 0;
         }*/
    }
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    // place your own initization codes here.
    iInitializeSound();
    iSetTimer(5, iTimer);
    iInitialize(1000, 600, "Bounce Classic");
    // iPlaySound("assets/sounds/game_audio.wav",true);

    return 0;
}





