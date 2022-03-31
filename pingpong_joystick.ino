#include "Arduino.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
// #include "XPT2046_Touchscreen.h"
// #include "Math.h"

#define DEBUG false // use/ignore the code of debug message
/* NANO's PIN-assigment */
// For the Adafruit shield, these are the default.
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10
#define TFT_MOSI 11
#define TFT_MISO 12
#define TFT_CLK 13
// Joystick & Buttons & Others
#define JOY_X_PIN A7
#define JOY_Y_PIN A6
#define JOY_SW_PIN A3
#define START_PIN A2
#define SELECT_PIN A1
#define LED_PIN 5
#define BUZZER_PIN 6
//Touch screen
#define TS_CS 7
/* Coordinate  */
#define ROTATION 3 
#define VelocityFunc(level) (1 + 0.7*level)

/* Variables & Objects */
// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC/RST
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// Ball
struct Ball
{
    int16_t cur_x, cur_y;
    int16_t old_x, old_y;
    double vel_x, vel_y;
    int8_t radius = 3;
};
// Bat
struct Bat
{
    int16_t cur_x, cur_y;
    int16_t old_x, old_y;
    int8_t vel_x;
    int8_t width = 30;
    int8_t height = 3;
};
struct Player
{
    int8_t lives = 50;
    int8_t level = 0;
    int32_t score = 0;
};
// JoyStick
struct JoyStick
{
    int32_t cen_x, cen_y;
    int32_t right_x, left_x;
    int32_t upper_y, lower_y;
};
// Block
class Block {
public:
    int16_t x, y;
    int16_t width;
    int16_t height;
    int16_t colour;
    int16_t score;
    boolean isActive;

    void setBlock(int bx, int by, int bwidth, int bheight, int bcol, int bscore){
        x = bx;
        y = by;
        width = bwidth;
        height = bheight;
        colour = bcol;
        score = bscore;
        isActive = true;
    }
    void drawBlock(){
        tft.fillRect(x, y, width, height, colour);
    }
    void removeBlock(){
        tft.fillRect(x, y, width, height, ILI9341_BLACK);
        isActive = false;
    }
};
/* Function Declarations */
// Init func
void initInfoBoard();
void initGameBoard();
// Game info
void drawScore();
void drawLifes();
void drawLevel();
// Ball func
void moveBall();
void drawBall();
void newBall();
// Bat func
void moveBat();
void drawBat();
// Collision func
int checkCollision();
boolean checkAllBlocksHit();
// JoyStick
void joyCali();

/* Data and Objects*/
Ball ball;
Bat bat;
Player player;
Block blocks[5][16];
JoyStick joystick;
// Others
boolean mute = false;   //state mute mode
boolean pre_sel = true; //control mute mode
int16_t tftWidth;   //TFT width = 320
int16_t tftHeight;  //TFT height = 240
const int16_t topBorder = 20;           //the height of infoboard

void initInfoBoard()
{
    /* init & draw info board */
    player.lives = 50;
    player.score = 0;
    player.level = 0;
    tft.drawFastHLine(0, topBorder-1, tftWidth, ILI9341_BLUE);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(0, 5);
    tft.print(F("SCORE :"));
    tft.setCursor(tftWidth/2, 5);
    tft.print(F("LIVES :"));
    tft.setCursor(tftWidth-75, 5);
    tft.print(F("LEVEL :"));
    drawLives();
    drawScore();
    drawLevel();
}
void initGameBoard()
{
    /* init & draw game board */
    int colour, score;
    // init Ball
    ball.cur_x = ball.radius + 1;
    ball.cur_y = 90;
    tft.fillCircle(ball.old_x, ball.old_y, ball.radius, ILI9341_BLACK);// remove ball
    tft.fillCircle(ball.cur_x, ball.cur_y, ball.radius, ILI9341_GREEN);
    ball.old_x = ball.cur_x;
    ball.old_y = ball.cur_y;
    ball.vel_x = 1;
    ball.vel_y = VelocityFunc(player.level);
    // init Bat
    bat.cur_x = (tftWidth - bat.width) / 2;
    bat.cur_y = (tftHeight - bat.height) - 30;
    tft.fillRect(bat.old_x, bat.old_y, bat.width, bat.height, ILI9341_BLACK);
    tft.fillRect(bat.cur_x, bat.cur_y, bat.width, bat.height, ILI9341_RED);
    bat.old_x = bat.cur_x;
    bat.old_y = bat.cur_y;

    // init Block
    for(int8_t row=0; row<5; row++){
        for (int8_t col=0; col<16; col++){
            if(row==0 || row==1) colour=ILI9341_BLUE   , score=50;
            else if(row==2 || row==3) colour=ILI9341_MAGENTA, score=30;
            else if(row==4 || row==5) colour=ILI9341_YELLOW , score=10;
            blocks[row][col].setBlock(col*20, row*10+30, 19, 9, colour, score);
            blocks[row][col].drawBlock();
        }
    }
}
void drawScore(){
    // clear old score
    tft.fillRect(50, 5, 25, 10, ILI9341_BLACK);
    // print new score
    tft.setCursor(50, 5);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(player.score);
    #if (DEBUG)
    Serial.print("Score: ");
    Serial.println(player.score);
    #endif
}
void drawLives(){
    // clear old lives
    tft.fillRect((tftWidth/2)+50, 5, 25, 10, ILI9341_BLACK);
    // print new score
    tft.setCursor((tftWidth/2)+50, 5);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(player.lives);
    #if (DEBUG)
    Serial.print("Lives: ");
    Serial.println(player.lives);
    #endif
}
void drawLevel(){
    // clear old level
    tft.fillRect(tftWidth-25, 5, 25, 10, ILI9341_BLACK);
    // print new score
    tft.setCursor(tftWidth-25,5);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(player.level + 1);
    #if (DEBUG)
    Serial.print("Level: ");
    Serial.println(player.level);
    #endif
}


void moveBall()
{
    // limit ball speed
    if(ball.vel_x >  2) ball.vel_x = 2;
    if(ball.vel_x < -2) ball.vel_x = -2;
    if(ball.vel_y >  2) ball.vel_y = 2;
    if(ball.vel_y < -2) ball.vel_y = -2;
    // move ball
    ball.cur_x += (int16_t)ball.vel_x;
    ball.cur_y += (int16_t)ball.vel_y;
    #if (DEBUG)
    Serial.print("xPos: ");
    Serial.print(ball.cur_x);
    Serial.print(" yPos: ");
    Serial.print(ball.cur_y);

    Serial.print("ball.vel_x: ");
    Serial.print(ball.vel_x);
    Serial.print(" ball.vel_y: ");
    Serial.println(ball.vel_y);
    #endif
}
void drawBall()
{
    tft.fillCircle(ball.old_x, ball.old_y, ball.radius, ILI9341_BLACK);
    tft.fillCircle(ball.cur_x, ball.cur_y, ball.radius, ILI9341_GREEN);
    ball.old_x = ball.cur_x;
    ball.old_y = ball.cur_y;
}
void newBall()
{
    int idx;
    randomSeed(millis());
    ball.cur_x = ((idx=random()%2) ? 0 : tftWidth - 15);
    ball.cur_y = 90;
    ball.vel_x = 0;
    ball.vel_y = -VelocityFunc(player.level);
    #if(DEBUG)
    Serial.print("random");//need debugging
    Serial.println(idx);
    #endif
}

void joyCali()
{
    // 1. You can use the data you have get.
    joystick.cen_x = 501;
    joystick.cen_y = 519;
    joystick.right_x = 0;
    joystick.left_x = 1021;
    joystick.upper_y = 0;
    joystick.lower_y = 1021;

    // 2 Or you can use the following program to get the data.
    // Center X and Y
    joystick.cen_x = 0;
    joystick.cen_y = 0;
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(tftWidth / 2 - 100 + 25, tftHeight / 2 - 20 + 12);
    tft.print(F("Center"));
    while(digitalRead(START_PIN));
    for(int16_t i=0 ; i<500 ; i++){
        joystick.cen_x += analogRead(JOY_X_PIN);
        joystick.cen_y += analogRead(JOY_Y_PIN);
        if(i==125 || i==250 || i==375){
            tft.print(F("."));
            delay(500);
        }

    }
    joystick.cen_x /= 500;
    joystick.cen_y /= 500;
    tft.setTextColor(ILI9341_DARKGREEN);
    tft.print(F("Done"));
    delay(1000);
    tft.fillRect(tftWidth / 2 - 100, tftHeight / 2 - 20, 200, 40, ILI9341_BLACK);

    // Right_X ( X~= 0)
    joystick.right_x = 0;
    tft.setCursor(tftWidth / 2 - 100 + 25, tftHeight / 2 - 20 + 12);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(F("Right X"));
    while(digitalRead(START_PIN));
    for(int16_t i=0 ; i<500 ; i++){
        joystick.right_x += analogRead(JOY_X_PIN);
        if(i==125 || i==250 || i==375){
            tft.print(F("."));
            delay(500);
        }
    }
    joystick.right_x /= 500;
    tft.setTextColor(ILI9341_DARKGREEN);
    tft.print(F("Done"));
    delay(1000);
    tft.fillRect(tftWidth / 2 - 100, tftHeight / 2 - 20, 200, 40, ILI9341_BLACK);

    // Left_X
    joystick.left_x = 0;
    tft.setCursor(tftWidth / 2 - 100 + 25, tftHeight / 2 - 20 + 12);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(F("Left X"));
    while(digitalRead(START_PIN));
    for(int16_t i=0 ; i<500 ; i++){
        joystick.left_x += analogRead(JOY_X_PIN);
        if(i==125 || i==250 || i==375){
            tft.print(F("."));
            delay(500);
        }
    }
    joystick.left_x /= 500;
    tft.setTextColor(ILI9341_DARKGREEN);
    tft.print(F("Done"));
    delay(1000);
    tft.fillRect(tftWidth / 2 - 100, tftHeight / 2 - 20, 200, 40, ILI9341_BLACK);

    // Upper_Y
    joystick.upper_y = 0;
    tft.setCursor(tftWidth / 2 - 100 + 25, tftHeight / 2 - 20 + 12);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(F("Upper Y"));
    while(digitalRead(START_PIN));
    for(int16_t i=0 ; i<500 ; i++){
        joystick.upper_y += analogRead(JOY_Y_PIN);
        if(i==125 || i==250 || i==375){
            tft.print(F("."));
            delay(500);
        }
    }
    joystick.upper_y /= 500;
    tft.setTextColor(ILI9341_DARKGREEN);
    tft.print(F("Done"));
    delay(1000);
    tft.fillRect(tftWidth / 2 - 100, tftHeight / 2 - 20, 200, 40, ILI9341_BLACK);

    // Lower_Y
    joystick.lower_y = 0;
    tft.setCursor(tftWidth / 2 - 100 + 25, tftHeight / 2 - 20 + 12);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(F("Lower Y"));
    while(digitalRead(START_PIN));
    for(int16_t i=0 ; i<500 ; i++){
        joystick.lower_y += analogRead(JOY_Y_PIN);
        if(i==125 || i==250 || i==375){
            tft.print(F("."));
            delay(500);
        }
    }
    joystick.lower_y /= 500;
    tft.setTextColor(ILI9341_DARKGREEN);
    tft.print(F("Done"));
    delay(1000);
    tft.fillRect(tftWidth / 2 - 100, tftHeight / 2 - 20, 200, 40, ILI9341_BLACK);
    #if (DEBUG)
    Serial.print(F("CX: ")); Serial.println(joystick.cen_x);
    Serial.print(F("CY: ")); Serial.println(joystick.cen_y);
    Serial.print(F("RX: ")); Serial.println(joystick.right_x);
    Serial.print(F("LX: ")); Serial.println(joystick.left_x);
    Serial.print(F("UY: ")); Serial.println(joystick.upper_y);
    Serial.print(F("DY: ")); Serial.println(joystick.lower_y);
    #endif
}

double getJoy(double delta_x) {
    double x = analogRead(JOY_X_PIN);
    if(x < joystick.cen_x - 100){   //RIGHT
        x = (delta_x * (x - joystick.cen_x)) / (joystick.right_x - joystick.cen_x);
        return x;
    }else if(x > joystick.cen_x + 100){ //LEFT
        x = -(delta_x * (x - joystick.cen_x)) / (joystick.left_x - joystick.cen_x);
        return x;
    }
    return 0;
}
boolean selectPressed(boolean& pre_sel)
{
    boolean now_sel = digitalRead(SELECT_PIN);
    if(pre_sel && (!now_sel)){ // falling
        pre_sel = now_sel;
        return true;
    }
    else{
        pre_sel = now_sel;
        return false;
    }
}

void moveBat() {
  bat.vel_x = (int8_t) getJoy(5.0);
  if (fabs(bat.vel_x) > 0) {
    bat.cur_x += bat.vel_x;
    if (bat.cur_x < 0)
        bat.cur_x = 0;
    if (bat.cur_x >= (tftWidth - bat.width))
        bat.cur_x = tftWidth - 1 - bat.width;
  }
}
void drawBat()
{
    tft.fillRect(bat.old_x, bat.old_y, bat.width, bat.height, ILI9341_BLACK);
    tft.fillRect(bat.cur_x, bat.cur_y, bat.width, bat.height, ILI9341_RED);
    bat.old_x = bat.cur_x;
    bat.old_y = bat.cur_y;
}

int checkCollision()
{
    // bounding box collision detection
    /* hit bound */
    if (ball.cur_x < ball.radius + 1){ // hit right bound
        ball.cur_x = ball.radius + 1;
        ball.vel_x = -ball.vel_x;
        return 0;
    }
    if (ball.cur_x > tftWidth - ball.radius - 1){ // hit left bound
        ball.cur_x = tftWidth - ball.radius - 1;
        ball.vel_x = -ball.vel_x;
        return 0;
    }
    if (ball.cur_y < topBorder + ball.radius + 1){ // hit the upper bound
        ball.cur_y = topBorder + ball.radius + 1;
        ball.vel_y = -ball.vel_y;
        return 0;
    }
    
    if (ball.cur_y > tftHeight - ball.radius){ // hit the lower bound (Lost one life)
        ball.cur_y = tftHeight - ball.radius;
        ball.vel_x = 0; 
        ball.vel_y = 0;
        player.lives--;
        drawLives();
        return 1;
    }
    /* hit Bat */
    // 1. Can modify ball direction/speed  
    // 2. Here just hit bat or not, can check it more detail
    //    (ex: hit bat upper/lower/right/left bound)
    if( (ball.cur_x > bat.cur_x - ball.radius - 1) && (ball.cur_x < bat.cur_x + bat.width  + ball.radius + 1) &&
        (ball.cur_y > bat.cur_y - ball.radius - 1) && (ball.cur_y < bat.cur_y + bat.height + ball.radius + 1) )
    {
        if(!mute)
            tone(BUZZER_PIN, 200, 70);
        ball.cur_y = bat.cur_y - ball.radius -1;
        ball.vel_x += 2.0 * (ball.cur_x - bat.cur_x + bat.width / 2.0) / bat.width; // use hit position increase ball.vel_x
        ball.vel_y = -(ball.vel_y + 0.001); // increase ball.vel_y
        return 2;
    }

    /* hit Block */
    // 1. Here check all blocks each time, can use other efficient method
    // 2. Here just hit block or not, can check it more detail
    //    (ex: hit block upper/lower/right/left bound) 
    for(int8_t row=0 ; row<5 ; row++){
        for(int8_t col=0 ; col<16 ; col++){
            if (blocks[row][col].isActive){
                int16_t x1 = blocks[row][col].x - ball.radius;
                int16_t x2 = blocks[row][col].x;
                int16_t x3 = blocks[row][col].x + blocks[row][col].width;
                int16_t x4 = blocks[row][col].x + blocks[row][col].width + ball.radius;
                int16_t y1 = blocks[row][col].y - ball.radius;
                int16_t y2 = blocks[row][col].y;
                int16_t y3 = blocks[row][col].y + blocks[row][col].height;
                int16_t y4 = blocks[row][col].y + blocks[row][col].height + ball.radius;
                if(ball.cur_y > y2 && ball.cur_y < y3)
                {
                    if((ball.cur_x > x1 - 1 && ball.cur_x < x2 + 1)
                    || (ball.cur_x > x3 - 1 && ball.cur_x < x4 + 1))
                    {
                        if(!mute)
                            tone(BUZZER_PIN, 1500, 70);                        
                        ball.cur_x -= ball.vel_x;
                        ball.cur_y -= ball.vel_y;
                        blocks[row][col].removeBlock();
                        player.score += blocks[row][col].score;
                        drawScore();
                        ball.vel_x = -ball.vel_x;
                        return 3;
                    }
                }
                if(ball.cur_x > x2 && ball.cur_x < x3)
                {
                    if((ball.cur_y > y1 - 1 && ball.cur_y < y2 + 1)
                    || (ball.cur_y > y3 - 1 && ball.cur_y < y4 + 1))
                    {
                        if(!mute)
                            tone(BUZZER_PIN, 1500, 70);
                        ball.cur_x -= ball.vel_x;
                        ball.cur_y -= ball.vel_y;
                        blocks[row][col].removeBlock();
                        player.score += blocks[row][col].score;
                        drawScore();
                        ball.vel_y = -ball.vel_y;
                        return 3;
                    }
                }
            }
        }
    }
    return 0;
}

boolean checkAllBlocksHit(){
    int8_t row, col;
    for (row=0 ; row<5 ; row++){
        for (col=0 ; col<16 ; col++){
            if (blocks[row][col].isActive){
                return false;
            }
        }
    }
    return true; 
}


void setup() {
    Serial.begin(9600);
    /* I/O SETTING */
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    // avoid chip select contention
    pinMode(TS_CS , OUTPUT); digitalWrite(TS_CS , HIGH);
    pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);

    pinMode(JOY_X_PIN, INPUT);
    pinMode(JOY_Y_PIN, INPUT);
    pinMode(START_PIN , INPUT_PULLUP);
    pinMode(SELECT_PIN, INPUT_PULLUP);

    tft.begin();
    tft.setRotation(ROTATION);
    tft.fillScreen(ILI9341_BLACK);
    tftWidth = tft.width();
    tftHeight = tft.height();
    
    joyCali();
}

/* GAME STATE */
#define GAME_START 1
#define GAME_INIT 2
#define GAME_STOP 4
#define GAME_PLAY 5
#define GAME_RELOAD 6
#define GAME_OVER 7
int gameState = GAME_START; // 1=start 2=playing 3=gameover
unsigned long lastFrame = millis();
int collision;

void loop(void) {
    /* Limit frame rate */
    while((millis() - lastFrame) < 16);// limit FPS<100
    lastFrame = millis();

    /* Buzzer & LED control */
    if(selectPressed(pre_sel)) //mute mode
        mute = !mute;
    if(mute)
        digitalWrite(LED_PIN, HIGH);
    else
        digitalWrite(LED_PIN, LOW);
    
    /* Game STATE */
    switch(gameState){
    case GAME_START: // show "start" on tft & wait player press "start"
        tft.fillRect ((tftWidth/2)-100, (tftHeight/2)-20, 200, 40, ILI9341_GREEN);
        tft.setCursor((tftWidth/2)-100+25, (tftHeight/2)-20+12);
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_WHITE);
        tft.print(F("Press \"start\""));
        gameState = GAME_INIT;
        break;
    case GAME_INIT: //
       if (!digitalRead(START_PIN)){
            tft.fillScreen(ILI9341_BLACK);
            initInfoBoard();
            initGameBoard();
            gameState = GAME_STOP;
        }
        break;
    case GAME_STOP:
        if (!digitalRead(START_PIN)){
            gameState = GAME_PLAY;
        }
        moveBat();
        drawBat(); 
        break;
    case GAME_PLAY:
        moveBall();
        moveBat();
        collision = checkCollision();
        if(collision == 1){
            if(player.lives > 0){
                newBall();
                gameState = GAME_STOP;
            }else{
                gameState = GAME_OVER;
            }
        }
        if(checkAllBlocksHit()){
            gameState = GAME_RELOAD;
        }
        drawBall();
        drawBat();
        break;
    case GAME_RELOAD: // new blocks
        delay(1000);
        player.level++;
        drawLevel();
        initGameBoard();
        gameState = GAME_STOP;
        break;

    case GAME_OVER: // end
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor((tftWidth/2)-150,50);
        tft.setTextSize(3);
        tft.setTextColor(ILI9341_WHITE);
        tft.print(F("You Scored "));
        // tft.print(playerScore);
        // if (!digitalRead(START_PIN)){
        //     gameState = GAME_START; // click start to play
        // }
        break;
    }
}
