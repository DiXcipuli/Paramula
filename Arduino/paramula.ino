byte serialArray[16];
int A0value, A1value, A2value, A3value, A4value, A5value;

void setup() {
  // initialize serial:
  Serial.begin(115200);

  //Input pin
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);

  //Sync ASCII code
  serialArray[0] = 83; //S char in ASCII
  serialArray[1] = 89; //Y char in ASCII
  serialArray[2] = 78; //N char in ASCII
  serialArray[3] = 67; //C char in ASCII
  
}

void loop() {
  A0value = analogRead(A0);
  A1value = analogRead(A1);
  A2value = analogRead(A2);
  A3value = analogRead(A3);
  A4value = analogRead(A4);
  A5value = analogRead(A5);
  
   //Arduino Pedal Board input voltage
  serialArray[4] = highByte(A0value);
  serialArray[5] = lowByte(A0value);
   //Piezo1
  serialArray[6] = highByte(A1value);
  serialArray[7] = lowByte(A1value);
  //Piezo2
  serialArray[8] = highByte(A2value);
  serialArray[9] = lowByte(A2value);
  //Piezo3
  serialArray[10] = highByte(A3value);
  serialArray[11] = lowByte(A3value);
  //Piezo4
  serialArray[12] = highByte(A4value);
  serialArray[13] = lowByte(A4value);
  //Piezo5
  serialArray[14] = highByte(A5value);
  serialArray[15] = lowByte(A5value);
  
  Serial.write(serialArray, 16);
  //delay(1000);
  //Serial.println(A0value);
}
