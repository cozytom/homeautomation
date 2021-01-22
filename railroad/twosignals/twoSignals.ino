
static unsigned long sig1RedStart=0;
static unsigned long sig2RedStart=0;

static unsigned long redOnTime=10000; // 10 seconds?

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello!, Signal Control");

  pinMode(2, OUTPUT); // signal 1 red
  pinMode(3, OUTPUT); // signal 1 green
  pinMode(4, OUTPUT); // signal 2 red
  pinMode(5, OUTPUT); // signal 2 green

  // start both signals green
  digitalWrite(3, 1);
  digitalWrite(5, 1);

}

void loop() {
  // put your main code here, to run repeatedly:
  int insideTrack = analogRead(A0);
  int outsideTrack = analogRead(A1);

  Serial.print(millis());
  Serial.print(" inside val = ");
  Serial.print(insideTrack);
  Serial.print(" time1 = ");
  Serial.print(sig1RedStart);
  Serial.print(" outside val = ");
  Serial.print(outsideTrack);
  Serial.print(" time2 = ");
  Serial.println(sig2RedStart);

  if ((sig1RedStart == 0) && (insideTrack < 580)) {
    digitalWrite(3, 0);
    digitalWrite(2, 1);
    sig1RedStart = millis(); 
  }

  if ((sig2RedStart == 0) && (outsideTrack < 180)) {
    digitalWrite(5, 0);
    digitalWrite(4, 1);
    sig2RedStart = millis();
  }

  // reset to green after elapsed time expires
  /*
  Serial.print(" time1 = ");
  Serial.print(sig1RedStart);
  Serial.print(" + ");
  Serial.print(redOnTime);
  Serial.print(" = ");
  Serial.println(sig1RedStart+redOnTime);
  */
  if ((sig1RedStart > 0) && (millis() > sig1RedStart+redOnTime)) {
    digitalWrite(3, 1);
    digitalWrite(2, 0);
    sig1RedStart = 0; 
    Serial.println("Green");    
  }
  if ((sig2RedStart > 0) && (millis() > sig2RedStart+redOnTime)) {
    digitalWrite(5, 1);
    digitalWrite(4, 0);
    sig2RedStart = 0;     
  }
}
