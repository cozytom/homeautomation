/**
 * Crossing signal, with two tracks. Uses CdS cells for train detection.
 * It takes 8, 4 for each track. The two CdS cells next to the crossing
 * clear the blink state, and the inside and outside hold unique states
 * such that when a train on one track is about to clear, the other train
 * will maintain the blink state. 
 * 
 * TGB 01/23/21
 */

#include <VariableTimedAction.h>


static unsigned long xingBlinkRate=1000; // 1 seconds

static int leftLED = 2;    // crossing LEDs
static int rightLED = 3;

static int outsideTrigger1=A0;
static int insideTrigger1=A1;
static int outsideClear2=A2;   
static int insideClear2=A3;
static int outsideClear1=A4;   
static int insideClear1=A5;
static int outsideTrigger2=A6;
static int insideTrigger2=A7;

class XingBlinkLED : public VariableTimedAction {
private:
  //stores the pin of this LED
  int pin;
 
  unsigned long run() {
    static bool leftOn=true;
    
    //if the left LED is on, then turn it off
    if (leftOn) {
      Serial.println("right");
      digitalWrite(rightLED, HIGH);
      digitalWrite(leftLED, LOW);
      leftOn = false;
    } else {
      Serial.println("left");
      digitalWrite(rightLED, LOW);
      digitalWrite(leftLED, HIGH);
      leftOn = true;
    }
    
    //returns the amount in seconds to wait before executing the next event
    //if 0 is returned, then the previous interval is maintained
    return 0;
  }
public:
  XingBlinkLED() {
    
  }
};


XingBlinkLED xingBlinker = XingBlinkLED();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello!, crossing");

  pinMode(leftLED, OUTPUT); 
  pinMode(rightLED, OUTPUT); 

  // start both Lights off
  clearLights();

  xingBlinker.start(xingBlinkRate, false);  //don't start the timer yet
}

void clearLights()
{
  digitalWrite(leftLED, HIGH);
  digitalWrite(rightLED, HIGH);
}

/**
 * Track sensors, clear next to crossing, triggers on ends
 * 
 * --o-------o-xing-o------o--
 * trig1    clr2   clr1  trig2
 * 
 * As the train comes from the left, trigger starts blinking
 * and as the train clear the crossing, the blinking stops. 
 * Likewise as the train comes from the right, the trigger
 * starts the blinking and as it clears, the blinking stops.
 * I make sure the train has passed more than 5 seconds, then
 * the other trigger is ignored. 
 */
void insideTrackCheck()
{
  static unsigned long clear1time=0;
  static unsigned long clear2time=0;
  static unsigned long trigger1time=0;
  static unsigned long trigger2time=0;
  static bool triggered1=false;
  static bool triggered2=false;
  static bool cleared1=false;
  static bool cleared2=false;
  
  int trigger1 = analogRead(insideTrigger1);
  int clear1   = analogRead(insideClear1);
  int trigger2 = analogRead(insideTrigger2);
  int clear2   = analogRead(insideClear2);

  // cleared is photocell covered
  cleared1 = (clear1 < 180);
  cleared2 = (clear2 < 180);

  // check if time to start blinking
  if ((trigger1 < 180) && (!triggered2) && 
      (clear2time+3000 < millis())) {
    triggered1 = true;
    trigger1time = millis(); 
    Serial.println("Start Blinking");
    if (!xingBlinker.isRunning())
    {
      // ensure it is blinking
      xingBlinker.toggleRunning();
    }
  }

  // train started passed the crossing, but last car not clear.
  if ((triggered1) && (!cleared1)) {
    // wait 3 seconds for the train to get to the crossing
    if (millis() > trigger1time + 3000) {
      clear1time = 0;
      triggered1 = false;
      clear1time = millis();
      trigger1time = 0;
      if (xingBlinker.isRunning())
      {
        // ensure it stops blinking
        xingBlinker.toggleRunning();
      }
      clearLights();
      Serial.println("Stop 1 Blinking");
    }
  }

  /* ************* right to left **************/
  // check if time to start blinking
  if ((trigger2 < 180) && (!triggered1) && 
      (clear1time+3000 < millis())) {
    triggered2 = true;
    trigger2time = millis(); 
    Serial.println("Start 2 Blinking");
    if (!xingBlinker.isRunning())
    {
      // ensure it is blinking
      xingBlinker.toggleRunning();
    }
  }

  // train started passed the crossing, but last car not clear.
  if ((triggered2) && (!cleared2)) {
    // wait 3 seconds for the train to get to the crossing
    if (millis() > trigger2time + 3000) {
      clear2time = 0;
      triggered2 = false;
      clear2time = millis();
      trigger2time = 0;
      if (xingBlinker.isRunning())
      {
        // ensure it stops blinking
        xingBlinker.toggleRunning();
      }
      clearLights();
      Serial.println("Stop 2 Blinking");
    }
  }
}

void outsideTrackCheck()
{
  static unsigned long clear1time=0;
  static unsigned long clear2time=0;
  static unsigned long trigger1time=0;
  static unsigned long trigger2time=0;
  static bool triggered1=false;
  static bool triggered2=false;
  static bool cleared1=false;
  static bool cleared2=false;
  
  int trigger1 = analogRead(outsideTrigger1);
  int clear1   = analogRead(outsideClear1);
  int trigger2 = analogRead(outsideTrigger2);
  int clear2   = analogRead(outsideClear2);

  // cleared is photocell covered
  cleared1 = (clear1 < 180);
  cleared2 = (clear2 < 180);

  // check if time to start blinking
  // the train is over the first trigger, the second trigger wasn't going already.
  // and we didn't just clear the second crossing side. 
  if ((trigger1 < 180) && (!triggered2) && 
      (clear2time+2000 < millis())) {
    triggered1 = true;
    trigger1time = millis(); 
    Serial.println("outside 1 Start Blinking");
    if (!xingBlinker.isRunning())
    {
      // ensure it is blinking
      xingBlinker.toggleRunning();
    }
  }

  // train started toward the crossing, and not train over clear.
  if ((triggered1) && (!cleared1)) {
    // wait 2 seconds for the train to get to the crossing
    if (millis() > trigger1time + 2000) {
      clear1time = 0;
      triggered1 = false;
      clear1time = millis();
      trigger1time = 0;
      if (xingBlinker.isRunning())
      {
        // ensure it stops blinking
        xingBlinker.toggleRunning();
      }
      clearLights();
      Serial.println("outside Stop 1 Blinking");
    }
  }

  /* ************* right to left **************/
  // check if time to start blinking
  // over trigger2 and grigger1 not going and not just cleared trigger1
  if ((trigger2 < 180) && (!triggered1) && 
      (clear1time+2000 < millis())) {
    triggered2 = true;
    trigger2time = millis(); 
    Serial.println("outside Start 2 Blinking");
    if (!xingBlinker.isRunning())
    {
      // ensure it is blinking
      xingBlinker.toggleRunning();
    }
  }

  // train started passed the crossing, not over clear.
  if ((triggered2) && (!cleared2)) {
    // wait 2 seconds for the train to get to the crossing
    if (millis() > trigger2time + 2000) {
      clear2time = 0;
      triggered2 = false;
      clear2time = millis();
      trigger2time = 0;
      if (xingBlinker.isRunning())
      {
        // ensure it stops blinking
        xingBlinker.toggleRunning();
      }
      clearLights();
      Serial.println("outside Stop 2 Blinking");
    }
  }
}

void loop() {
  //Serial.print(millis());
  insideTrackCheck();
  outsideTrackCheck();
  
  //this static method should really be the only thing called in your loop method to make sure events are updated on time
  VariableTimedAction::updateActions();
}
