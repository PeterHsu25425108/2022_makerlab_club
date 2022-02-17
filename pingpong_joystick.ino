#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "XPT2046_Touchscreen.h"
#include "Math.h"

// function declarations
boolean checkCollision(int x1, int y1, int width1, int height1, int x2, int y2, int width2, int height2);

// For the Adafruit shield, these are the default.
#define TFT_CS 10
#define TFT_DC 9
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 8
#define TFT_MISO 12
#define joy_x A7
#define joy_y A6
#define joy_sw A3
#define start A2
#define select A1

#define TS_CS 7

#define ROTATION 3/*(0,0) is at the upper right corner
                    The same orientation as the joystick*/
#define buzzerin 6

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC/RST
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TS_CS);

float xCalM = 0.0, yCalM = 0.0;
float xCalC = 0.0, yCalC = 0.0;
float xPos = 0.0, yPos = 0.0;
float xPosLast = 0.0, yPosLast = 0.0;
float xVel = 0.0, yVel = 0.0;
int8_t topBorder = 20;
int8_t batWidth = 30;
int8_t batHeight = 3;
int16_t batX = 0, batY = 0;
int8_t ballSize = 3;
int playerLives = 50;
int playerScore = 0;
int gameState = 1; // 1=start 2=playing 3=gameover
int level;
bool mute = false;

int16_t tftWidth = tft.width(), tftHeight = tft.height();

class ScreenPoint {
public:
int16_t x;
int16_t y;

ScreenPoint(){
// default contructor
}

ScreenPoint(int16_t xIn, int16_t yIn){
x = xIn;
y = yIn;
}
};

class Block {
public:
int x;
int y;
int width;
int height;
int colour;
int score;
boolean isActive;

// default constructor
Block(){}

Block(int xpos, int ypos, int bwidth, int bheight, int bcol, int bscore){
x = xpos;
y = ypos;
width = bwidth;
height = bheight;
colour = bcol;
score = bscore;
isActive = true;
drawBlock();
}

void drawBlock(){
tft.fillRect(x,y,width,height,colour);
}

void removeBlock(){
tft.fillRect(x,y,width,height,ILI9341_BLACK);
isActive = false;
}

boolean isHit(float x1,float y1, int w1,int h1){
if (checkCollision((int)round(x1),(int)round(y1),w1,h1,x,y,width,height)){
return true;
}
else {
return false;
}
}
};
Block blocks[5][16];

ScreenPoint getScreenCoords(int16_t x, int16_t y){
int16_t xCoord = round((x * xCalM) + xCalC);
int16_t yCoord = round((y * yCalM) + yCalC);
if(xCoord < 0) xCoord = 0;
if(xCoord >= tftWidth) xCoord = tftWidth - 1;
if(yCoord < 0) yCoord = 0;
if(yCoord >= tftHeight) yCoord = tftHeight - 1;
return(ScreenPoint(xCoord, yCoord));
}

void calibrateTouchScreen(){
TS_Point p;
int16_t x1,y1,x2,y2;

tft.fillScreen(ILI9341_BLACK);
tft.setCursor((tftWidth/2)-100+25,(tftHeight/2)-20+12);
tft.setTextSize(2);
tft.println("CALIBRATING");
tft.setCursor((tftWidth/2)-100+25,(tftHeight/2)-20+36);
tft.println("Touch +");

// wait for no touch
while(ts.touched());
tft.drawFastHLine(10,20,20,ILI9341_WHITE);
tft.drawFastVLine(20,10,20,ILI9341_WHITE);
while(!ts.touched());
p = ts.getPoint();
x1 = p.x;
y1 = p.y;
tft.drawFastHLine(10,20,20,ILI9341_BLACK);
tft.drawFastVLine(20,10,20,ILI9341_BLACK);
delay(500);
while(ts.touched());
tft.drawFastHLine(tftWidth - 30,tftHeight - 20,20,ILI9341_WHITE);
tft.drawFastVLine(tftWidth - 20,tftHeight - 30,20,ILI9341_WHITE);
while(!ts.touched());
p = ts.getPoint();
x2 = p.x;
y2 = p.y;
tft.drawFastHLine(tftWidth - 30,tftHeight - 20,20,ILI9341_BLACK);
tft.drawFastVLine(tftWidth - 20,tftHeight - 30,20,ILI9341_BLACK);

int16_t xDist = tftWidth - 40;
int16_t yDist = tftHeight - 40;

// translate in form pos = m x val + c
// x
xCalM = (float)xDist / (float)(x2 - x1);
xCalC = 20.0 - ((float)x1 * xCalM);
// y
yCalM = (float)yDist / (float)(y2 - y1);
yCalC = 20.0 - ((float)y1 * yCalM);

Serial.print("x1 = ");Serial.print(x1);
Serial.print(", y1 = ");Serial.print(y1);
Serial.print("x2 = ");Serial.print(x2);
Serial.print(", y2 = ");Serial.println(y2);
Serial.print("xCalM = ");Serial.print(xCalM);
Serial.print(", xCalC = ");Serial.print(xCalC);
Serial.print("yCalM = ");Serial.print(yCalM);
Serial.print(", yCalC = ");Serial.println(yCalC);
tft.fillRect((tftWidth/2)-100,(tftHeight/2)-20,200,60,ILI9341_BLACK);

}


void setup() {
Serial.begin(9600);
pinMode(buzzerin,OUTPUT);
// avoid chip select contention
pinMode(TS_CS, OUTPUT);
digitalWrite(TS_CS, HIGH);
pinMode(TFT_CS, OUTPUT);
digitalWrite(TFT_CS, HIGH);

pinMode(joy_x,INPUT);
pinMode(joy_y,INPUT);
pinMode(joy_sw,INPUT_PULLUP);
pinMode(start,INPUT_PULLUP);
pinMode(select,INPUT_PULLUP);

tft.begin();
tft.setRotation(ROTATION);
tft.fillScreen(ILI9341_BLACK);
tftWidth = tft.width();
tftHeight = tft.height();
ts.begin();
ts.setRotation(ROTATION);
calibrateTouchScreen();

batY = tftHeight - batHeight -30;
}

void initGame(){

tft.fillScreen(ILI9341_BLACK);
tft.drawFastHLine(0, topBorder-1, tftWidth, ILI9341_BLUE);
tft.setCursor(0,5);
tft.setTextSize(1);
tft.setTextColor(ILI9341_WHITE);
tft.print("SCORE :");
tft.setCursor((tftWidth/2), 5);
tft.print("LIVES :");
tft.setCursor(tftWidth - 75, 5);
tft.print("LEVEL :");

batY = tftHeight - batHeight -30;
playerLives = 50;
playerScore = 0;
level = 0;

drawLives();
drawScore();
drawLevel();

initGameBoard();

}

void initGameBoard() {
int row, col;
int colour, score;
clearOldBallPos();
xPosLast = xPos = 0;
yPosLast = yPos = 90;
xVel = 1;
yVel = 1 + 0.7*(level);
gameState = 2;

for(row=0; row < 5; row++){
for (col=0; col < 16; col++){
switch(row){
case 0:
case 1:
colour = ILI9341_BLUE;
score = 50;
break;
case 2:
case 3:
colour = ILI9341_MAGENTA;
score = 30;
break;
case 4:
case 5:
colour = ILI9341_YELLOW;
score = 10;
break;
}
blocks[row][col] = Block(col*20, (row*10) + 30, 19, 9,colour,score);
}
}

}

void clearOldBallPos(){
tft.fillCircle(round(xPosLast), round(yPosLast), ballSize, ILI9341_BLACK);
}

void moveBall(float &xPos, float &yPos, float &xVel, float &yVel,float &xPosLast,float &yPosLast){
float newX, newY;
newX = xPos + xVel;
newY = yPos + yVel;
if (newX < (float)ballSize){
newX = (float)ballSize;
xVel = -xVel;
}
if (newX > (float)(tftWidth - ballSize - 1)){
newX = (float)(tftWidth - ballSize - 1);
xVel = -xVel;
}
if (newY < topBorder + (float)ballSize){
newY = topBorder + (float)ballSize;
yVel = -yVel;
}
if ((round(newX) != round(xPosLast)) && (round(newY) != round(yPosLast))){
// draw ball
clearOldBallPos();
tft.fillCircle(round(newX), round(newY), ballSize, ILI9341_GREEN);
xPosLast = newX;
yPosLast = newY;
}
xPos = newX;
yPos = newY;
/*Serial.print("x: ");
Serial.print(xPos);
Serial.print(" ");
Serial.print("y: ");
Serial.print(yPos);

Serial.print("xVel: ");
Serial.print(xVel);
Serial.print(" ");
Serial.print("yVel: ");
Serial.println(yVel);*/
}

void drawScore(){
// clear old score
tft.fillRect(50,5,25,10,ILI9341_BLACK);
// print new score
tft.setCursor(50,5);
tft.setTextSize(1);
tft.setTextColor(ILI9341_WHITE);
tft.print(playerScore);
}

void drawLives(){
// clear old lives
tft.fillRect((tftWidth/2)+50,5,25,10,ILI9341_BLACK);
// print new score
tft.setCursor((tftWidth/2)+50,5);
tft.setTextSize(1);
tft.setTextColor(ILI9341_WHITE);
tft.print(playerLives);
}

void drawLevel(){
// clear old level
tft.fillRect(tftWidth-25,5,25,10,ILI9341_BLACK);
// print new score
tft.setCursor(tftWidth-25,5);
tft.setTextSize(1);
tft.setTextColor(ILI9341_WHITE);
tft.print(level + 1);
}

void newBall(){
int xl[2] = {0, tft.width()-15};
randomSeed(millis());
int idx =random(0,2);
Serial.print("random");//need debugging
Serial.println(idx);
xPos = xl[idx];
yPos = 90;
xVel = yVel = 2;
//delay(10);
tft.fillCircle(round(xPos), round(yPos), ballSize, ILI9341_GREEN);
long lastFrame = millis();
while(digitalRead(start))
{
  while((millis() - lastFrame) < 10);
  lastFrame = millis();
  moveBat();
}
  
tft.fillCircle(round(xPos), round(yPos), ballSize, ILI9341_BLACK);
moveBall(xPos,yPos,xVel,yVel,xPosLast,yPosLast);
//delay(1000);
}

boolean checkBallLost(){
if (yPos > tftHeight + ballSize){
return true;
}
else {
return false;
}
}

double getJoy()
{
  double x = analogRead(joy_x)-503.0;
  return -x;
}

void moveBat(){
int16_t newBatX;
//ScreenPoint sp = ScreenPoint(0,0);
double joy_input=getJoy();
//Serial.println(joy_input);
if (abs(joy_input)>0) {
//TS_Point p = ts.getPoint();
//sp = getScreenCoords(p.x, p.y);
  double max_deltaX = 3;
  newBatX = batX + getJoy()*max_deltaX/360.0;
  if (newBatX < 0)
    newBatX = 0;
  if (newBatX >= (tftWidth - batWidth))
    newBatX = tftWidth - 1 - batWidth;
}

/*if (ts.touched()) {
  TS_Point p = ts.getPoint();
  sp = getScreenCoords(p.x, p.y);
  newBatX = sp.x - (batWidth / 2);
  if (newBatX < 0) 
    newBatX = 0;
  if (newBatX >= (tftWidth - batWidth))
    newBatX = tftWidth - 1 - batWidth;
}*/

if (abs(newBatX - batX) > 1){
tft.fillRect(batX, batY, batWidth, batHeight,ILI9341_BLACK);
batX = newBatX;
tft.fillRect(batX, batY, batWidth, batHeight,ILI9341_RED);
}
}

// bounding box collision detection
boolean checkCollision(int x1, int y1, int width1, int height1, int x2, int y2, int width2, int height2){
boolean hit = false;
if (
(((x2 + width2) >= x1) && (x2 <= (x1 + width1)))
&& (((y2 + height2) >= y1) && (y2 <= (y1 + height1)))
) {
hit = true;
}

return hit;
}

void checkHitBat(){
// check bat and bottom half of ball
float xInc;
boolean hit = checkCollision(batX, batY, batWidth, batHeight, (int)round(xPos)-ballSize, (int)round(yPos), ballSize*2, ballSize);
if (hit) {
// reverse ball y direction but increase speed
if(!mute)
  tone(buzzerin,200,70);
yVel += 0.001;
if (yVel > 2){
yVel = 2;
}
yVel = -yVel;
// rounded bounce
xInc = (xPos - (float)(batX + (batWidth / 2))) / (float)batWidth;
xVel += 2 * xInc;
if (abs(xVel) > 2){
if (xVel < 0) {
xVel = -2;
}
else {
xVel = 2;
}
}
// make sure ball not hitting bat
yPos = (float)(batY - ballSize -1);
}
}

void checkHitBlock(){
int row, col;
for (row=0; row<5/*&lt;5*/; row++){
for (col=0; col<16/*&lt;16*/; col++){
if (blocks[row][col].isActive && blocks[row][col].isHit(xPos,yPos, ballSize*2,ballSize*2)){
if(!mute)
  tone(buzzerin,1500,70);
//delay();
blocks[row][col].removeBlock();
playerScore += blocks[row][col].score;
drawScore();
yVel = -yVel;
return;
}
}
}
}

boolean checkAllBlocksHit(){
int row, col, actives;
actives = 0;
for (row=0; row<5; row++){
for (col=0; col<16; col++){
if (blocks[row][col].isActive){
return false;
}
}
}

return true; 
}

unsigned long lastFrame = millis();

void loop(void) {

// limit frame rate
while((millis() - lastFrame) < 10);
lastFrame = millis();

switch(gameState){
case 1: // start
gameState = 11;
break;

case 11: // click to play
tft.fillRect((tftWidth/2)-100,(tftHeight/2)-20,200,40,ILI9341_GREEN);
tft.setCursor((tftWidth/2)-100+25,(tftHeight/2)-20+12);
tft.setTextSize(2);
tft.setTextColor(ILI9341_WHITE);
tft.print("CLICK TO PLAY");
gameState = 12;
break;

case 12: // wait for click to play
if (ts.touched()) {
TS_Point p = ts.getPoint();
ScreenPoint sp = getScreenCoords(p.x, p.y);
if (checkCollision(sp.x, sp.y,1,1,(tftWidth/2)-50,(tftHeight/2)-20,100,40)){
initGame();
gameState = 2;
}
}
break;

case 2: // play
/*Serial.print(xPos);
Serial.print(" ");
Serial.println(yPos);*/
if(!digitalRead(select))
  mute = !mute;
moveBall(xPos,yPos,xVel,yVel,xPosLast,yPosLast);
moveBat();
checkHitBat();
checkHitBlock();
if (checkBallLost()){
//tft.fillCircle(round(xPos), round(yPos), ballSize, ILI9341_BLACK);
tft.fillCircle(round(xPosLast), round(yPosLast), ballSize, ILI9341_BLACK);
playerLives --;
drawLives();
if (playerLives > 0){
newBall();
xVel=1;
yVel=1;
}
else {
gameState = 3; // end game
}
}

if (checkAllBlocksHit()){
gameState = 4;
}

break;

case 4: // new blocks
delay(1000);
level ++;
drawLevel();
initGameBoard();
break;

case 3: // end
tft.fillScreen(ILI9341_BLACK);
tft.setCursor((tftWidth/2)-150,50);
tft.setTextSize(3);
tft.setTextColor(ILI9341_WHITE);
tft.print("You Scored ");
tft.print(playerScore);
gameState = 11; // click to play
break;
}




}
