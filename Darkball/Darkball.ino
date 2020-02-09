/*Darkball
   by Che-Wei Wang,  CW&T
   for Blinks by Move38
*/



// Did we get an error onthis face recently?
Timer errorOnFaceTimer[ FACE_COUNT ];

const int showErrTime_ms = 500;    // Show the errror for 0.5 second so people can see it

static Timer showColorOnFaceTimer[ FACE_COUNT ];

#define SHOW_COLOR_TIME_MS  110   // Long enough to see

byte ball[] { 1, 0, 6, 3 }; //speed, n rounds ball has been played, last path length, position to blink,

#define MAGIC_VALUE 42

boolean neighbors[ FACE_COUNT ];
byte neighborCount = 0;
long lastMillis = 0;
int sendBall = -1;
boolean hasBall = false;
byte hp = FACE_COUNT + 1;
byte lastNeighbor;
long lastReceivedBall = 0;
byte ballResponseRange = 200;

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

  // First check all faces for an incoming datagram
  if (sendBall >= 0) {
    if (millis() - lastMillis > ball[0]) {

      setColorOnFace( OFF ,  sendBall  );
      sendDatagramOnFace( &ball , sizeof( ball ) , sendBall );
      showColorOnFaceTimer[sendBall].set( SHOW_COLOR_TIME_MS );
      sendBall = -1;
    }

  }
  FOREACH_FACE(f) {

    if ( isDatagramReadyOnFace( f ) ) {

      const byte *datagramPayload = getDatagramOnFace(f);

      // Check that the length and all of the data btyes of the recieved datagram match what we were expecting
      // Note that `memcmp()` returns 0 if it is a match

      //if ( getDatagramLengthOnFace(f) == sizeof( ball )  &&  !memcmp( datagramPayload , ball , sizeof( ball )) )
      {
        // This is the datagram we are looking for!

        //update ball
        ball[0] = datagramPayload[0]; //speed


        setColorOnFace( OFF , f );
        lastMillis = millis();
        //find next available face
        //boolean isEnd = true;
        FOREACH_FACE(nf) { //cycle through faces
          if (f != nf) { //don't check face that just received data
            if (neighbors[nf]  ) { //if face is connected
              //set face to send ball
              sendBall = nf;
              //isEnd = false;
            }
          }
        }

        if (neighborCount == 1) { //received the ball and is the end
          //hasBall=true;
          lastReceivedBall = millis();
          hp--;
          //          setColorOnFace( RED ,  f  );
          //          showColorOnFaceTimer[f].set( SHOW_COLOR_TIME_MS );

        }

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

        if (getLastValueReceivedOnFace(f) == MAGIC_VALUE ) { //connected to neighbor
          neighbors[f] = true;
          setColorOnFace( YELLOW , f );

        } else {
          setColorOnFace( CYAN, f );
        }

      } else {
        // Or off if no neighbor
        neighbors[f] = false;
        setColorOnFace( OFF , f );
      }

      //count neighbors
      neighborCount = 0;
      for (int i = 0; i < FACE_COUNT; i++) {
        if (neighbors[i]) {
          neighborCount++;
          lastNeighbor = i;
        }
      }
      if (neighborCount == 1) {//endpoint
        int count = lastNeighbor + 1; //light up from "top" (connection to path)
        FOREACH_FACE(f) {
          if (count < hp + lastNeighbor + 1) setColorOnFace( GREEN, count % FACE_COUNT );
          else setColorOnFace (RED, count % FACE_COUNT );
          count++;
          if (hp <= 0) {
            //explode

            //reset
            hp = FACE_COUNT + 1;
          }

        }
      } else if (neighborCount == 0) {//not connected
        setColor(RED);
      }
    }
  }


  if (buttonPressed()) {
    // When the button is click, trigger a datagram send on all faces
    if (neighborCount == 1 )//check if i'm an endpoint and have ball

      if (millis() - lastReceivedBall > 0  || hp > FACE_COUNT) { //in possession of ball  or is first time after reset
        shoot();
        //lastReceivedBall = 0;
        if (millis() - lastReceivedBall < ballResponseRange ) { //if i have the ball and i hit it in time
          //hp++;
          if (hp > FACE_COUNT)hp = FACE_COUNT; //max hp

        } else { //swing and miss
          hp--;

        }

      }
  }

}



void shoot() {
  FOREACH_FACE(f) {

    ball[0] = byte(int(random(110))); //random speed
    //ball[1] = byte(int(random(5))); //when to show

    sendDatagramOnFace( &ball , sizeof( ball ) , f );


  }
}
