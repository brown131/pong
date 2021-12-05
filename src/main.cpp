/******************************************************************
 Pong Game
 Scott Brown   May 10, 2020
 ******************************************************************/

/*
        +----------------+           +-----------------+
        |            D13 |-----------| SCK             |
        |            D12 |-----------| MISO            |
        |            D11 |-----------| MOSI   TFT LCD  |
        |            D10 |-----------| CS     display  |
        |             D3 |-----------| INT     RA8875  |
        |            TXD |-----------| RESET   module  |
        |                |     GND <-|                 |
        |   adafruit     |      5v <-|                 |
        |  pro trinket   |           +-----------------+
        |      5V        |           +----------------+
        |             A1 |-----------| HORZ           |
        |             A0 |-----------| VERT  thumb    |
        |             D5 |-----------| SEL  joystick  |
        |                |     GND <-|       module   |
        |                |      5v <-|                |
        |                |           +----------------+
        |                |             +------+
        |                |            /        \ 
        |            GND |---------->|  active  |
        |             5v |---------->|  buzzer  |
        |                |            \        /
        +----------------+             +------+
        
*/

/*
 COMPILE BUG WORKAROUND
 Need to copy ~/.platformio/packages/framework-arduino-avr/libraries/EEPROM/src/EEPROM.h to
 .pio/libdeps/protrinket5/Adafruit RA8875
*/

#include <SPI.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_RA8875.h>

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
const int16_t BALL_RADIUS = 7;
const int16_t TEXT_LENGTH = 24;

int balls = 15;
int score = 0;
int balls0, score0;
boolean hit_ball = false;

int32_t paddle_x = (SCREEN_WIDTH - PADDLE_WIDTH) >> 1;
int32_t paddle_x0 = paddle_x;

double ball_x = SCREEN_WIDTH / 2;
double ball_y = BALL_RADIUS;
double ball_dx = 0;
double ball_dy = 1;
double ball_x0, ball_y0;
double ball_speed = 1;

double rnd() {
  return (static_cast <double> (rand()) / static_cast <double> (RAND_MAX));
}

void resetBall() {
  // Send ball in a random direction from the top of the screen.
  ball_x = rnd() * (SCREEN_WIDTH - BALL_RADIUS * 2) + BALL_RADIUS;
  ball_y = BALL_RADIUS;
  double a = rnd() * PI / 3;  // Angle <= 60 degrees.
  ball_dy = cos(a) * ball_speed;
  ball_dx = (sin(a) * ball_speed) * (rnd() >= 0.5 ? 1 : -1);
  delay(200);
}

void playBuzzer(int cycles = 20, int duration = 2) {
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

  if ((ball_x - BALL_RADIUS) < 0) {
    ball_x = BALL_RADIUS;
    ball_dx = ball_speed - square(ball_dy);  // Needs to be done
    playBuzzer();
  }
  if ((ball_x + BALL_RADIUS) > SCREEN_WIDTH) {
    ball_x = SCREEN_WIDTH - BALL_RADIUS;
    ball_dx *= -1;
    playBuzzer();
  }
  if ((ball_y - BALL_RADIUS) < 0) {
    ball_y = BALL_RADIUS;
    ball_dy = abs(ball_dy);  // Needs to be done this way.
    playBuzzer();
  }
  if ((ball_y + BALL_RADIUS) > SCREEN_HEIGHT) {
    resetBall();
  }
}

void movePaddle() {
  int32_t x = analogRead(X_PIN);
  paddle_x0 = paddle_x;
  paddle_x = (x * (SCREEN_WIDTH - PADDLE_WIDTH)) >> 10;
}

void displayBall() {
  int16_t x = (int16_t)round(ball_x);
  int16_t y = (int16_t)round(ball_y);
  int16_t x0 = (int16_t)round(ball_x0);
  int16_t y0 = (int16_t)round(ball_y0);
  
  if (x != x0 || y != y0) {
    tft.fillCircle(x0, y0, BALL_RADIUS, RA8875_BLACK);
    tft.fillCircle(x, y, BALL_RADIUS, RA8875_WHITE);
  }
}

void displayPaddle() {
  if (paddle_x != paddle_x0) {
    tft.fillRect((int16_t)paddle_x0, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, RA8875_BLACK);
    tft.fillRect((int16_t)paddle_x, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, RA8875_WHITE);
  }
}

void displayScore() {
  if (balls != balls0 || score != score0 ||
      ((ball_x + BALL_RADIUS) < (TEXT_LENGTH * 10) && int(ball_y + BALL_RADIUS) <= 11)) {
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
  if (score >= MAX_WAIT && ball_speed < 2) {
    double a = acos(ball_dy / ball_speed);
    ball_speed *= 1.05;
    ball_dy = cos(a) * -ball_speed;
    ball_dx = (sin(a) * ball_speed) * (ball_dx < 0 ? 1 : -1);
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
  if (ball_y >= (SCREEN_HEIGHT - 1 - BALL_RADIUS * 3)) {
    if (int(ball_x + BALL_RADIUS) >= paddle_x && int(ball_x - BALL_RADIUS) <= (paddle_x + PADDLE_WIDTH)) {
      if (!hit_ball) {
        hitBall();
        hit_ball = true;
      }
      return;
    }
  }
  if (ball_y >= (SCREEN_HEIGHT - 1 - BALL_RADIUS)) {
    balls--;
    if (balls == 0) {
      displayScore();
      gameOver();
      return;
    } else {
      resetBall();
    }
  }
  hit_ball = false;
}

void setup() {
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

void loop() {
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