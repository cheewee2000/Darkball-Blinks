/*
   Darkball
   by Che-Wei Wang,  CW&T
   for Blinks by Move38

  setup:
   place blinks in a single path (two player mode), or branches (multiplayer mode), or a single path with a loop ( P shape, single player mode)
   endpoints will be green
   paths will be yellow
   cyan endpoints have the ball


  each player plays at one endpoint. finger on the end tile


  cyan endpoints can send the ball along the path
  green endpoints try to hit the ball back
  if players swing too late, or swing too early, they lose a point
  game ends when one player to loses 6 points

*/

#define DURATION_FOR_GAME_PIECE_TO_RESET  5000  // 5 seconds to reset a paddle
#define SHOW_COLOR_TIME_MS                110   // 0.11 Duration of darkball (Long enough to see)
#define EXHAUST_TRAIL_DURATION            500   // 0.5 seconds of a trail
#define DEFAULT_HUE                       20    // Orange color for track
#define MAX_HUE_SHIFT                     80    // Shift the trail into the blues  


// Did we get an error onthis face recently?
Timer errorOnFaceTimer[ FACE_COUNT ];

const int showErrTime_ms = 500;    // Show the errror for 0.5 second so people can see it

static Timer showColorOnFaceTimer[ FACE_COUNT ];
static Timer gameOverTimer;
long timeBallLastOnFace[ FACE_COUNT ];

byte ball[] { 1, 0, 6, 3 }; //speed, n rounds ball has been played, last path length, position to blink,

#define MAGIC_VALUE 22

boolean hasNeigbhorAtFace[ FACE_COUNT ];
byte neighborCount = 0;
long lastMillis = 0;
int sendBall = -1;
byte hp = FACE_COUNT ;
byte lastNeighbor;
long lastReceivedBall = 0;
int ballResponseRange = 100;
boolean hasBall = true;
boolean missed = true;
boolean swung = false;
long lastSwing = 0;
int slowestBallSpeed = 110; //bigger is slower
int endAnimCount = 0;
long lastWasEndpoint = 0;
boolean superMode = false;

void setup() {
  setValueSentOnAllFaces(MAGIC_VALUE);
}

int memcmp(const void *s1, const void *s2, unsigned n)
{
  if (n != 0) {
    const unsigned char *p1 = s1, *p2 = s2;

    do {
      if (*p1++ != *p2++)
        return (*--p1 - *--p2);
    } while (--n != 0);
  }
  return (0);
}


void loop() {

  //pass the ball along
  if (sendBall >= 0) { //if i have the ball
    if (millis() - lastMillis > ball[0]) { //wait ball speed
      if (superMode) {
        if (ball[2] % 4 == 0)setColorOnFace( OFF ,  sendBall  ); //turn off lights every few tiles'
        ball[2]++;
      }
      else {
        setColorOnFace( OFF ,  sendBall  ); //turn off lights
      }
      sendDatagramOnFace( &ball , sizeof( ball ) , sendBall ); //send ball
      // TODO: is this how the dark ball is actually drawn?
      // This handles the sending face
      showColorOnFaceTimer[sendBall].set( SHOW_COLOR_TIME_MS ); //set face color
      timeBallLastOnFace[sendBall] = millis();
      sendBall = -1; //set sendball to -1
    }
  }


  if (hasBall && neighborCount == 1 && missed == false)  { //if I have the ball and and I'm an endpoint
    if ( millis() - lastReceivedBall > ballResponseRange) { //never swung or swung too late
      hp--;
      missed = true;
    }
  }


  // First check all faces for an incoming datagram
  FOREACH_FACE(f) {

    if ( isDatagramReadyOnFace( f ) ) { //received ball

      const byte *datagramPayload = getDatagramOnFace(f);

      {
        // This is the datagram we are looking for!
        //update ball
        ball[0] = datagramPayload[0]; //get ball speed
        ball[1] = datagramPayload[1]; //get ball superMode
        ball[2] = datagramPayload[2]; //get ball superMode counter
        superMode = ball[1];

        setColorOnFace( OFF , f ); // draw dark ball
        // TODO: is this how the dark ball is actually drawn?
        // This seems to handle the receiving face
        showColorOnFaceTimer[f].set( SHOW_COLOR_TIME_MS ); //set face color
        timeBallLastOnFace[f] = millis();
        lastMillis = millis();

        int avalableNeighboringFaces[FACE_COUNT];
        int count = 0;

        //find next available face
        FOREACH_FACE(nf) { //cycle through faces
          if (f != nf) { //don't check face that just received data
            if (hasNeigbhorAtFace[nf]) { //if face is connected
              avalableNeighboringFaces[count] = nf;//set face to send ball
              count++;
            }
          }
        }

        if (count > 0) sendBall = avalableNeighboringFaces[int(random(count - 1))]; //if there's a neighbor, send the ball there

        if (neighborCount == 1) { //received the ball and is the end
          hasBall = true;
          lastReceivedBall = millis();
          //check if swing is early or late
          if (swung ) {
            if ( millis() - lastSwing < ballResponseRange ) { //if i have the ball and i hit it in time
              shoot((millis() - lastSwing) / float(ballResponseRange));
            }
          }
          swung = false;
        }
        else hasBall = false;

      }


      // We are done with the datagram, so free up the buffer so we can get another on this face
      markDatagramReadOnFace( f );
    }


    //path
    if ( !isValueReceivedOnFaceExpired( f ) ) {
      if (getLastValueReceivedOnFace(f) == MAGIC_VALUE ) { //connected to neighbor
        hasNeigbhorAtFace[f] = true;
      }
    }
    else {
      hasNeigbhorAtFace[f] = false;
    }
  }


  //count hasNeigbhorAtFace
  neighborCount = 0;
  for (int i = 0; i < FACE_COUNT; i++) {
    if (hasNeigbhorAtFace[i]) {
      neighborCount++;
      lastNeighbor = i;
    }
  }

  //allow swinging again after 1 second
  if (swung && millis() - lastSwing >= 500) {
    swung = false;
  }


  //set colors
  //endpoint
  if (neighborCount == 1) {
    int count = lastNeighbor + 1; //light up from "top" (connection to path)
    lastWasEndpoint = millis();

    FOREACH_FACE(f) {
      //endpoint normal state shows points

      if (hp > 0) {
        if (count < hp + lastNeighbor + 1) {
          if (hasBall && missed == true) {
            if (superMode)setColorOnFace( WHITE, count % FACE_COUNT );
            else setColorOnFace( CYAN, count % FACE_COUNT );
          }
          else {
            if (swung && hasBall == false) setColorOnFace( dim( GREEN, 60), count % FACE_COUNT );
            else setColorOnFace( GREEN, count % FACE_COUNT );
          }
        }
        else {
          if (swung && hasBall == false)  setColorOnFace( dim( RED, 60), count % FACE_COUNT );
          else setColorOnFace( RED, count % FACE_COUNT );
        }

        count++;
      }



    }

  } else if (neighborCount == 0 && endAnimCount == 0) { //blinks not connected to anything and not showing animation
    spinAnimation(RED, 110);
  }
  else { //path
    FOREACH_FACE(f) {
      if (hasNeigbhorAtFace[f]) {
        // If the ball is hit perfectly, have the trail sparkle
        // TODO: Only do this when ball speed is XXX
        // after the ball passes, leave a trail of color/sparkle
        long timeSinceBall = millis() - timeBallLastOnFace[f];
        if (timeSinceBall > EXHAUST_TRAIL_DURATION) {
          timeSinceBall = EXHAUST_TRAIL_DURATION;
        }
        byte hueShift = MAX_HUE_SHIFT - map(timeSinceBall, 0, EXHAUST_TRAIL_DURATION, 0, MAX_HUE_SHIFT);
        byte hue = DEFAULT_HUE - hueShift;  // the byte wraps this with no problems
        byte bri = 255 - random(hueShift);
        setColorOnFace( makeColorHSB( hue, 255 , bri ) , f );//path color
      }
      else {
        setColorOnFace(OFF, f);
      }
      if (!showColorOnFaceTimer[f].isExpired()) {
        setColorOnFace(OFF, f);
      }

    }
  }

  if (hp == 0) {
    //game over animation
    if (gameOverTimer.isExpired()) {
      randomAnimation(RED, 30);
      gameOverTimer.set( 30 );
      endAnimCount++;
    }

    if (endAnimCount > 36) {
      reset();
    }

  }


  //reset tile
  if (buttonDoubleClicked()) {
    if (neighborCount == 0 ) hp = 0; //set hp to 0 to force game over anim
  }
  if ( millis() - lastWasEndpoint > DURATION_FOR_GAME_PIECE_TO_RESET ) {
    if (neighborCount == 0 || neighborCount > 1) reset(); //reset quietly if tile is no longer an endpoint
  }


  //swing paddle
  if (buttonPressed()) {
    // When the button is click, trigger a datagram send on all faces
    if (neighborCount == 1 ) { //check if i'm an endpoint

      if (swung == false) { //only swing once
        swung = true;
        lastSwing = millis();

        if (hasBall && millis() - lastReceivedBall < ballResponseRange  ) { //if i have the ball and i hit it in time
          shoot((millis() - lastReceivedBall) / float(ballResponseRange)); //sets swung to false etc.
        }

      }

      //serve ball
      if (missed) {
        if ( hasBall && millis() - lastReceivedBall > 500) { //prevent hitting the ball if we just missed the ball
          shoot(.5); //shoot at half speed
        }
      }
    }
  }

  //supermode
  if (buttonMultiClicked()) {
    if (neighborCount == 0 ) {
      superMode = !superMode;
      hp = 0;
    }

  }
}

void reset() {
  //reset
  hasBall = true;
  hp = FACE_COUNT ;
  missed = true;
  endAnimCount = 0;

}

Timer animStepTimer;
int animCount = 0;

void spinAnimation(Color c, int delayTime) {
  if (animStepTimer.isExpired()) {
    setColor(c);
    animCount++;
    setColorOnFace( OFF, animCount % FACE_COUNT );
    animStepTimer.set( delayTime );
  }
}

void randomAnimation(Color c, int delayTime) {
  if (animStepTimer.isExpired()) {
    setColor(c);
    animCount++;
    setColorOnFace( OFF, random( FACE_COUNT ));
    animStepTimer.set( delayTime );
  }
}

void swingAnimation(Color c, int delayTime) {
  if (animStepTimer.isExpired()) {
    setColor(c);
    animCount++;
    setColorOnFace( OFF, animCount % FACE_COUNT );
    animStepTimer.set( delayTime );
  }
}



void shoot(float ballSpeed) {//0-1
  byte s = ballSpeed * slowestBallSpeed;
  FOREACH_FACE(f) {
    ball[0] = byte(s); //random speed
    ball[1] = superMode;
    ball[2] = 0; //superMode when to show counter
    sendDatagramOnFace( &ball , sizeof( ball ) , f );
  }
  missed = false;
  hasBall = false;
  swung = false;
}
