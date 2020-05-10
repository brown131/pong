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
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"

// Library only supports hardware SPI at this time
// Connect SCLK to UNO Digital #13 (Hardware SPI clock)
// Connect MISO to UNO Digital #12 (Hardware SPI MISO)
// Connect MOSI to UNO Digital #11 (Hardware SPI MOSI)
#define RA8875_INT 3
#define RA8875_CS 10
#define RA8875_RESET 4

// Analog Joystick
#define SW_PIN 5 // digital pin connected to switch output
#define X_PIN 0  // analog pin connected to X output
#define Y_PIN 1  // analog pin connected to Y output

// Buzzer
#define BUZZER_PIN 8

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);

const int16_t PADDLE_Y = 250;
const int16_t PADDLE_WIDTH = 40;
const int16_t PADDLE_HEIGHT = 15;
const int16_t BALL_WIDTH = 15;
const int16_t BALL_HEIGHT = 15;

int32_t paddle_x = (480 - PADDLE_WIDTH) >> 1;
int32_t paddle_x0 = paddle_x;

double ball_x = (480.0F - BALL_WIDTH) / 2.0F;
double ball_y = 0.0F;
double ball_dx = 0.0F;
double ball_dy = 1.0F;
double ball_x0;
double ball_y0;

void debug(const char *message) {
  tft.textSetCursor(10, 10);
  char spaces[48];
  memset(spaces, ' ', 48);
  spaces[48] = '\0';
  tft.textWrite(spaces);

  tft.textSetCursor(10, 10);
  
}

double rnd() {
  return (static_cast <double> (rand()) / static_cast <double> (RAND_MAX));
}

void directBall() {
  ball_dy = rnd() / 2.0F + 0.5F;
  ball_dx = (1.0F - square(ball_dy)) * (rnd() > 0.5F ? 1.0F : -1.0F);
}

void playBuzzer() {
   for (int i = 0; i < 10; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1);
    digitalWrite(BUZZER_PIN, LOW);
    delay(1);
   }
}

void setup()
{
  pinMode(SW_PIN, INPUT);
  digitalWrite(SW_PIN, HIGH);

  pinMode(BUZZER_PIN,OUTPUT);

  /* Initialize the display using 'RA8875_480x80', 'RA8875_480x128', 'RA8875_480x272' or 'RA8875_800x480' */
  if (!tft.begin(RA8875_480x272)) {
    return;
  }

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);

  tft.graphicsMode(); 
  tft.fillScreen(RA8875_BLACK);
  //tft.fillRect(ball_x, ball_y, BALL_WIDTH, BALL_HEIGHT, RA8875_WHITE);
  tft.fillRect((int16_t)paddle_x, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, RA8875_WHITE);

  directBall();
}

void moveBall() {
  ball_x0 = ball_x;
  ball_y0 = ball_y;
  ball_x += ball_dx;
  ball_y += ball_dy;

  if (ball_x < 0.0F) {
    ball_x = 0.0F;
    ball_dx *= -1.0F;
    playBuzzer();
  }
  if (ball_x > 480.0F) {
    ball_x = 480.0F;
    ball_dx *= -1.0F;
    playBuzzer();
  }
  if (ball_y < 0.0F) {
    ball_x *= -1.0F;
    ball_y *= -1.0F;
  }
  if (ball_y > 272.0F) {
    ball_x = rnd() * 480.0F;
    ball_y = 0.0F;
    directBall();
  }
}

void movePaddle() {
  int32_t a = analogRead(X_PIN);
  paddle_x0 = paddle_x;
  paddle_x = (a * (480 - PADDLE_WIDTH)) >> 10;
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
  tft.textMode();
  tft.textColor(RA8875_WHITE, RA8875_BLACK);
  tft.textSetCursor(10, 10);
  char spaces[48];
  memset(spaces, ' ', 22);
  spaces[22] = '\0';
  tft.textWrite(spaces);
  tft.textSetCursor(10, 10);debug("Balls: 00  Score: 0000");
  tft.graphicsMode();
}

void loop()
{
  moveBall();
  movePaddle();

  displayBall();
  displayPaddle();
  displayScore();
  
  delay(10);
}