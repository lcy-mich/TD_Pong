#include "TFT_eSPI.h"
#include "SPI.h"

#define L_BUTTON 0
#define R_BUTTON 35

#define PADDLE_PADDING 20
#define PADDLE_LENGTH 30
#define PADDLE_COLOUR TFT_WHITE
#define PADDLE_SPEED 10

#define STARTING_SPEED 0.5
#define BALL_COLOUR TFT_WHITE
#define BALL_RADIUS 10
#define BALL_COLLISION_PADDING 4

#define POINT_PADDING_X 8
#define POINT_PADDING_Y 16
#define MIDLINE_DOTTED_AMT 6
#define BACKGROUND_FOREGROUND_COLOUR TFT_WHITE
#define BACKGROUND_FOREGROUND_TEXT_COLOUR TFT_WHITE
#define BACKGROUND_BACKGROUND_COLOUR TFT_BLACK

TFT_eSPI tft = TFT_eSPI();

#define DEBOUNCE_TIME 50
#define MAX_BOTH_TIME 1000
unsigned long debounceTime;

typedef enum {
  NONE, LEFT, RIGHT, BOTH
} Button;
Button lastPressed = NONE;
bool buttonActed = true;

typedef struct {
  int x;
  int y;
} IntVector;

typedef struct {
  float x;
  float y;
} FloatVector;

typedef struct {
  FloatVector past_position;
  FloatVector position;
  FloatVector velocity;
  int radius;
} Ball;

typedef struct {
  IntVector past_position;
  IntVector position;
  int length;
} Paddle;

int playerPoint = 0;
int oppPoint = 0;

Paddle playerPaddle;
Paddle oppPaddle;

Ball ball;

void setLastPressed(Button button) {
  lastPressed = button;
  buttonActed = false;
}

void lButtonPressed() {
  if ((lastPressed == RIGHT) && ((millis() - debounceTime) < MAX_BOTH_TIME)) {
    setLastPressed(BOTH);
    return;
  }
  if ((millis() - debounceTime) < DEBOUNCE_TIME) {
    return;
  }
  debounceTime = millis();

  setLastPressed(LEFT);
}
void rButtonPressed() {
  if ((lastPressed == LEFT) && ((millis() - debounceTime) < MAX_BOTH_TIME)) {
    setLastPressed(BOTH);
    return;
  }
  if ((millis() - debounceTime) < DEBOUNCE_TIME) {
    return;
  }

  setLastPressed(RIGHT);
}

void rightAction() {
  playerPaddle.past_position = playerPaddle.position;
  playerPaddle.position = (IntVector){min(playerPaddle.position.x + PADDLE_SPEED, tft.width() - 1 - playerPaddle.length/2), playerPaddle.position.y};
}

void leftAction() {
  playerPaddle.past_position = playerPaddle.position;
  playerPaddle.position = (IntVector){max(playerPaddle.position.x - PADDLE_SPEED, 1 + playerPaddle.length/2), playerPaddle.position.y};
}

void bothAction() {
  initialise();
}

void drawMidline() {
  const int length = (tft.width() - 1) / (2*MIDLINE_DOTTED_AMT);
  for (int i = 0; i < 2*MIDLINE_DOTTED_AMT; i++) {
      if (i % 2 == 0) tft.drawFastHLine((length + MIDLINE_DOTTED_AMT/2)/2 + length*i, (tft.height() - 1) / 2, length, BACKGROUND_FOREGROUND_COLOUR);
  }
}

void drawPoints() {
  tft.setTextSize(2);
  // tft.setTextColor(BACKGROUND_FOREGROUND_TEXT_COLOUR);
  // tft.setText

  tft.setTextDatum(BL_DATUM);
  tft.drawString(String(oppPoint)+"  ", POINT_PADDING_X, -POINT_PADDING_Y + (tft.height() - 1) / 2);

  tft.setTextDatum(TL_DATUM);
  tft.drawString(String(playerPoint)+"  ", POINT_PADDING_X, POINT_PADDING_Y + (tft.height() - 1) / 2);
}

void drawBackground() {
  tft.fillScreen(BACKGROUND_BACKGROUND_COLOUR);

  tft.drawFastHLine(0, 0, tft.width(), BACKGROUND_FOREGROUND_COLOUR);
  tft.drawFastHLine(0, tft.height() - 1, tft.width(), BACKGROUND_FOREGROUND_COLOUR);
  tft.drawFastVLine(0, 0, tft.height(), BACKGROUND_FOREGROUND_COLOUR);
  tft.drawFastVLine(tft.width() - 1, 0, tft.height(), BACKGROUND_FOREGROUND_COLOUR);
}

void drawPaddle(Paddle paddle) {
  tft.drawFastHLine(paddle.past_position.x - paddle.length/2, paddle.past_position.y, paddle.length, BACKGROUND_BACKGROUND_COLOUR);
  tft.drawFastHLine(paddle.position.x - paddle.length/2, paddle.position.y, paddle.length, BACKGROUND_FOREGROUND_COLOUR);
}

void drawPaddles() {
  drawPaddle(oppPaddle);
  drawPaddle(playerPaddle);
}

void drawBall() {
  tft.fillCircle(ball.past_position.x, ball.past_position.y, BALL_RADIUS, BACKGROUND_BACKGROUND_COLOUR);
  tft.fillCircle(ball.position.x, ball.position.y, BALL_RADIUS, BALL_COLOUR);
}

void updateBackground() {
  drawPoints();
  drawMidline();
}

void updateForeground() {
  drawPaddles();
  drawBall();
}

void updatePhysics() {
  ball.past_position = ball.position;
  ball.position = (FloatVector){ball.position.x + ball.velocity.x, ball.position.y + ball.velocity.y};

  if ((ball.position.x + ball.radius >= tft.width() - 3 - BALL_COLLISION_PADDING) || (ball.position.x - ball.radius <= 2 + BALL_COLLISION_PADDING)) {
    ball.velocity = (FloatVector){-ball.velocity.x, ball.velocity.y};
  }

  if ( (((ball.position.y + ball.radius >= playerPaddle.position.y - 1) && ((ball.position.x - ball.radius >= playerPaddle.position.x - playerPaddle.length/2) && (ball.position.x + ball.radius <= playerPaddle.position.x + playerPaddle.length/2))) || ((ball.position.y - ball.radius <= oppPaddle.position.y + 1) && ((ball.position.x - ball.radius >= oppPaddle.position.x - oppPaddle.length/2) && (ball.position.x + ball.radius <= oppPaddle.position.x + oppPaddle.length/2))))  || (((ball.position.y + ball.radius >= tft.height() - 3 - BALL_COLLISION_PADDING) || (ball.position.y - ball.radius <= 2 + BALL_COLLISION_PADDING)))) {
    ball.velocity = (FloatVector){ball.velocity.x, -ball.velocity.y};
  }

  ball.position = (FloatVector){ball.past_position.x + ball.velocity.x, ball.past_position.y + ball.velocity.y};
}

void initialise() {
  drawBackground();

  playerPaddle.position = (IntVector){tft.width()/2, tft.height() - 2 - PADDLE_PADDING};
  playerPaddle.past_position = playerPaddle.position;
  playerPaddle.length = PADDLE_LENGTH;

  oppPaddle.position = (IntVector){tft.width()/2, 1 + PADDLE_PADDING};
  oppPaddle.past_position = oppPaddle.position;
  oppPaddle.length = PADDLE_LENGTH;

  ball.position = (FloatVector){tft.width()/2, tft.height()/2};
  ball.past_position = ball.position;
  ball.velocity = (FloatVector){((float)random(-75, 75))/100,((float)random(-100, 100))/100};
  ball.velocity = (FloatVector){STARTING_SPEED*ball.velocity.x/sqrt(ball.velocity.x*ball.velocity.x + ball.velocity.y*ball.velocity.y), STARTING_SPEED*ball.velocity.y/sqrt(ball.velocity.x*ball.velocity.x + ball.velocity.y*ball.velocity.y)};
  ball.radius = BALL_RADIUS;

  oppPoint = 0;
  playerPoint = 0;
}

void setup() {
  tft.init();
  tft.setRotation(0);
  

  initialise();

  debounceTime = millis();
  attachInterrupt(digitalPinToInterrupt(L_BUTTON), lButtonPressed, RISING);
  attachInterrupt(digitalPinToInterrupt(R_BUTTON), rButtonPressed, RISING);
}

void loop() {
  updateBackground();
  updatePhysics();
  updateForeground();

  // tft.drawString(String(ball.velocity.x) + ", " + String(ball.velocity.y), 0, 0);

  if (buttonActed) return;

  switch (lastPressed) {
    case RIGHT:
      rightAction();
      break;
    case LEFT:
      leftAction();
      break;
    case BOTH:
      bothAction();
      break;
  }
  buttonActed = true;
}
