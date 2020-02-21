/*Darkball
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

  todo:
  set ball speed based on accuracy

  change path color
  juicy stuff
  color sparkle trail. based on speed. hue wave, and saturation 200m
  double click for super mode

*/


// Did we get an error onthis face recently?
Timer errorOnFaceTimer[ FACE_COUNT ];

const int showErrTime_ms = 500;    // Show the errror for 0.5 second so people can see it

static Timer showColorOnFaceTimer[ FACE_COUNT ];

#define SHOW_COLOR_TIME_MS  110   // Long enough to see

byte ball[] { 1, 0, 6, 3 }; //speed, n rounds ball has been played, last path length, position to blink,

#define MAGIC_VALUE 42

boolean hasNeigbhorAtFace[ FACE_COUNT ];
byte neighborCount = 0;
long lastMillis = 0;
int sendBall = -1;
byte hp = FACE_COUNT ;
byte lastNeighbor;
long lastReceivedBall = 0;
int ballResponseRange = 200;
boolean hasBall = true;
boolean missed = true;
boolean swung = false;
long lastSwing = 0;
int slowestBallSpeed = 110; //bigger is slower

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
      setColorOnFace( OFF ,  sendBall  ); //turn off lights
      sendDatagramOnFace( &ball , sizeof( ball ) , sendBall ); //send ball
      showColorOnFaceTimer[sendBall].set( SHOW_COLOR_TIME_MS ); //set face color
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

      // Check that the length and all of the data btyes of the recieved datagram match what we were expecting
      // Note that `memcmp()` returns 0 if it is a match

      //if ( getDatagramLengthOnFace(f) == sizeof( ball )  &&  !memcmp( datagramPayload , ball , sizeof( ball )) )
      {
        // This is the datagram we are looking for!

        //update ball
        ball[0] = datagramPayload[0]; //get ball speed


        setColorOnFace( OFF , f ); // draw dark ball
        lastMillis = millis();

        int avalableNeighboringFaces[FACE_COUNT];
        int count = 0;
        //find next available face
        FOREACH_FACE(nf) { //cycle through faces
          if (f != nf) { //don't check face that just received data
            if (hasNeigbhorAtFace[nf]) { //if face is connected
              //set face to send ball
              avalableNeighboringFaces[count] = nf;
              count++;
              //sendBall = nf;
              //isEnd = false;
            }
          }
        }

        if (count > 0) sendBall = avalableNeighboringFaces[int(random(count - 1))];

        if (neighborCount == 1) { //received the ball and is the end
          hasBall = true;
          lastReceivedBall = millis();
          //check if swing is early or late
          if (swung ) {
            if ( millis() - lastSwing < ballResponseRange ) { //if i have the ball and i hit it in time
             shoot((millis() - lastSwing) / float(ballResponseRange)); 
            }
          }
        }
        else hasBall = false;

      }
      //      else {
      //
      //        // Oops, we goty a datagram, but not the one we are loooking for
      //
      //        setColorOnFace( RED , f );
      //
      //      }


      showColorOnFaceTimer[f].set( SHOW_COLOR_TIME_MS );

      // We are done with the datagram, so free up the buffer so we can get another on this face
      markDatagramReadOnFace( f );


    } else if ( showColorOnFaceTimer[f].isExpired() ) {

      if ( !isValueReceivedOnFaceExpired( f ) ) {

        // Show green if we do have a neighbor
        //path color
        if (getLastValueReceivedOnFace(f) == MAGIC_VALUE ) { //connected to neighbor
          hasNeigbhorAtFace[f] = true;
          setColorOnFace( makeColorHSB( 33 , 255 , 255 ) , f );

        }
        //        else {
        //          setColorOnFace( CYAN, f );
        //        }

      } else {
        // Or off if no neighbor
        hasNeigbhorAtFace[f] = false;
        setColorOnFace( OFF , f );
      }

      //count hasNeigbhorAtFace
      neighborCount = 0;
      for (int i = 0; i < FACE_COUNT; i++) {
        if (hasNeigbhorAtFace[i]) {
          neighborCount++;
          lastNeighbor = i;
        }
      }
      if (neighborCount == 1) {//endpoint
        int count = lastNeighbor + 1; //light up from "top" (connection to path)
        FOREACH_FACE(f) {
          if (count < hp + lastNeighbor + 1) {
            if (hasBall && missed == true) setColorOnFace( CYAN, count % FACE_COUNT );
            else setColorOnFace( GREEN, count % FACE_COUNT );
          }
          else {
            if (hasBall)
              setColorOnFace( ORANGE, count % FACE_COUNT );
            else setColorOnFace( RED, count % FACE_COUNT );
          }
          count++;
          if (hp <= 0) {
            //explode

            //reset
            hp = FACE_COUNT ;
          }

        }
      } else if (neighborCount == 0) {//not connected
        setColor(RED);
      }
    }
  }


  if (buttonPressed()) {
    // When the button is click, trigger a datagram send on all faces
    if (neighborCount == 1 ) { //check if i'm an endpoint and have ball

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
          shoot(.5);
        }
      }


    }
  }

  if (neighborCount == 0 && buttonDoubleClicked) {
    hasBall = true;
    hp = FACE_COUNT ;
    missed = true;
  }

}



void shoot(float ballSpeed) {//0-1

  byte s = ballSpeed * slowestBallSpeed;

  FOREACH_FACE(f) {
    ball[0] = byte(s); //random speed
    //ball[1] = byte(int(random(5))); //when to show
    sendDatagramOnFace( &ball , sizeof( ball ) , f );
  }
  missed = false;
  hasBall = false;
  swung = false;
}
