/******************************************************************
 This is an example for the Adafruit RA8875 Driver board for TFT displays
 ---------------> http://www.adafruit.com/products/1590
 The RA8875 is a TFT driver for up to 800x480 dotclock'd displays
 It is tested to work with displays in the Adafruit shop. Other displays
 may need timing adjustments and are not guanteed to work.

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source hardware
 by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries.
 BSD license, check license.txt for more information.
 All text above must be included in any redistribution.
 ******************************************************************/

#include <SPI.h>
#include <time.h>
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"

// Library only supports hardware SPI at this time
// Connect SCLK to UNO Digital #13 (Hardware SPI clock)
// Connect MISO to UNO Digital #12 (Hardware SPI MISO)
// Connect MOSI to UNO Digital #11 (Hardware SPI MOSI)
#define RA8875_INT 3
#define RA8875_CS 10
#define RA8875_RESET 4

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define MAX_WAIT 10

// Analog Joystick
#define SW_PIN 5 // digital pin connected to switch output
#define X_PIN 0  // analog pin connected to X output
#define Y_PIN 1  // analog pin connected to Y output

// Buzzer
#define BUZZER_PIN 8

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);

const int16_t PADDLE_WIDTH = 40;
const int16_t PADDLE_HEIGHT = 15;
const int16_t PADDLE_Y = SCREEN_HEIGHT - PADDLE_HEIGHT;
const int16_t BALL_WIDTH = 15;
const int16_t BALL_HEIGHT = 15;
const int16_t TEXT_LENGTH = 24;

int balls = 15;
int score = 0;
int balls0, score0;
boolean gameover = false;

int32_t paddle_x = (SCREEN_WIDTH - PADDLE_WIDTH) >> 1;
int32_t paddle_x0 = paddle_x;

double ball_x = (SCREEN_WIDTH - BALL_WIDTH) / 2;
double ball_y = 0;
double ball_dx = 0;
double ball_dy = 1;
double ball_x0, ball_y0;
double ball_speed = 1;

double rnd() {
  return (static_cast <double> (rand()) / static_cast <double> (RAND_MAX));
}

void resetBall() {
  // Send ball in a random direction from the top of the screen.
  ball_x = rnd() * SCREEN_WIDTH;
  ball_y = 0;
  ball_dy = rnd() / 2 + 0.5;
  ball_dx = (1 - square(ball_dy)) * (rnd() > 0.5 ? 1 : -1);
}

void playBuzzer(int cycles = 10) {
   for (int i = 0; i < cycles; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1);
    digitalWrite(BUZZER_PIN, LOW);
    delay(1);
   }
}

void moveBall() {
  ball_x0 = ball_x;
  ball_y0 = ball_y;
  ball_x += ball_dx;
  ball_y += ball_dy;

  if (ball_x < 0) {
    ball_x = 0;
    ball_dx = 1 - square(ball_dy);  // Needs to be done
    playBuzzer();
  }
  if ((ball_x + BALL_WIDTH) > SCREEN_WIDTH) {
    ball_x = SCREEN_WIDTH - BALL_WIDTH;
    ball_dx *= -1;
    playBuzzer();
  }
  if (ball_y < 0) {
    ball_y = 0;
    ball_dy = abs(ball_dy);  // Needs to be done this way.
    playBuzzer();
  }
  if (ball_y > SCREEN_HEIGHT) {
    resetBall();
  }
}

void movePaddle() {
  int32_t a = analogRead(X_PIN);
  paddle_x0 = paddle_x;
  paddle_x = (a * (SCREEN_WIDTH - PADDLE_WIDTH)) >> 10;
}

void displayBall() {
  int16_t x = (int16_t)round(ball_x);
  int16_t y = (int16_t)round(ball_y);
  int16_t x0 = (int16_t)round(ball_x0);
  int16_t y0 = (int16_t)round(ball_y0);
  
  if (x != x0 || y != y0) {
    tft.fillRect(x0, y0, BALL_WIDTH, BALL_HEIGHT, RA8875_BLACK);
    tft.fillRect(x, y, BALL_WIDTH, BALL_HEIGHT, RA8875_WHITE);
  }
}

void displayPaddle() {
  if (paddle_x != paddle_x0) {
    tft.fillRect((int16_t)paddle_x0, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, RA8875_BLACK);
  }
  tft.fillRect((int16_t)paddle_x, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, RA8875_WHITE);
}

void displayScore() {
  if (balls != balls0 || score != score0 ||
      (ball_x < (TEXT_LENGTH * 10) && int(ball_y) <= 11)) {
    tft.textMode();
    tft.textSetCursor(10, 10);
    char buff[TEXT_LENGTH];
    memset(buff, ' ', TEXT_LENGTH);
    buff[TEXT_LENGTH] = '\0';
    tft.textWrite(buff);
    tft.textSetCursor(10, 10);
    if (balls > 0) {
      tft.textColor(RA8875_YELLOW, RA8875_BLACK);
      sprintf(buff, "Balls: %2d  Score: %d", balls, score);
    } else {
      tft.textColor(RA8875_GREEN, RA8875_BLACK);
      sprintf(buff, "Game Over!  Score: %d", score);
    }
    tft.textWrite(buff);
    tft.graphicsMode();
  }
}

void hitBall() {
  playBuzzer();
  score++;
  if (score >= MAX_WAIT) {
    ball_speed *= 1.1;
    //ball_dx = ball_speed - square()
  } else {
    ball_dy *= -1;
  }
}

void gameOver() {
  while (digitalRead(SW_PIN) == HIGH) {
    // Wait for paddle to be pressed.
    delay(10);
  }
  balls = 15;
  score = 0;
  ball_speed = 1;
  resetBall();
}

void scoreGame() {
  balls0 = balls;
  score0 = score;
  if ((ball_y + BALL_HEIGHT) >= (SCREEN_HEIGHT - BALL_HEIGHT)) {
    if (int(ball_x + BALL_WIDTH) >= paddle_x && int(ball_x) <= (paddle_x + PADDLE_WIDTH)) {
      hitBall();
    } else {
      balls--;
      if (balls == 0) {
        displayScore();
        gameOver();
        return;
      } else {
        resetBall();
      }
    }
  }
}

void setup()
{
  pinMode(BUZZER_PIN,OUTPUT);

  /* Initialize the display using 'RA8875_480x80', 'RA8875_480x128', 'RA8875_480x272' or 'RA8875_800x272' */
  if (!tft.begin(RA8875_480x272)) {
    return;
  }

  pinMode(SW_PIN, INPUT);
  digitalWrite(SW_PIN, HIGH);

  srand(time(NULL));

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);

  tft.graphicsMode(); 
  tft.fillScreen(RA8875_BLACK);
  tft.fillRect((int16_t)paddle_x, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, RA8875_WHITE);

  displayScore();
  balls0 = balls;
  score0 = score;

  resetBall();
}

void loop()
{
  moveBall();
  movePaddle();
  scoreGame();
  displayBall();
  displayPaddle();
  displayScore();
  if (score < MAX_WAIT) {
    delay(MAX_WAIT - score);
  }
}