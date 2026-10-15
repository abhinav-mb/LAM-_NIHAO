//Senosr
#define FLOAT_SENSOR_PIN 35 
#define FLOW_SENSOR_PIN  14 
#define SOLENOID_PIN     27 

//Pump1
#define PUMP1_L_EN  13
#define PUMP1_R_EN  26
#define PUMP1_L_PWM 32
#define PUMP1_R_PWM 33

//Pump2
#define PUMP2_L_EN  16
#define PUMP2_R_EN  17
#define PUMP2_L_PWM 18 
#define PUMP2_R_PWM 19 

//Pump3
#define PUMP3_L_EN  21
#define PUMP3_R_EN  22
#define PUMP3_L_PWM 23
#define PUMP3_R_PWM 25


volatile unsigned int pulseCount = 0;
unsigned long oldTime = 0;
float totalLiters = 0.0;
float flushLiters = 0.0; 
float flowRate_mL_sec = 0.0;

enum SystemState { CASE_1, TRANSITION, CASE_2, FINISHED };
SystemState currentState = CASE_1;
bool floatWasTriggered = false;
unsigned long transitionStartTime = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setPump1Speed(int speed) { analogWrite(PUMP1_R_PWM, speed); }
void setPump2Speed(int speed) { analogWrite(PUMP2_L_PWM, speed); } 
void setPump3Speed(int speed) { analogWrite(PUMP3_R_PWM, speed); }

void setup() {
  Serial.begin(115200);

  pinMode(FLOAT_SENSOR_PIN, INPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW); 

  pinMode(PUMP1_L_EN, OUTPUT); pinMode(PUMP1_R_EN, OUTPUT);
  pinMode(PUMP1_L_PWM, OUTPUT); pinMode(PUMP1_R_PWM, OUTPUT);
  digitalWrite(PUMP1_L_EN, HIGH); digitalWrite(PUMP1_R_EN, HIGH);
  analogWrite(PUMP1_L_PWM, 0); 

  pinMode(PUMP2_L_EN, OUTPUT); pinMode(PUMP2_R_EN, OUTPUT);
  pinMode(PUMP2_L_PWM, OUTPUT); pinMode(PUMP2_R_PWM, OUTPUT);
  digitalWrite(PUMP2_L_EN, HIGH); digitalWrite(PUMP2_R_EN, HIGH);
  analogWrite(PUMP2_R_PWM, 0); 

  pinMode(PUMP3_L_EN, OUTPUT); pinMode(PUMP3_R_EN, OUTPUT);
  pinMode(PUMP3_L_PWM, OUTPUT); pinMode(PUMP3_R_PWM, OUTPUT);
  digitalWrite(PUMP3_L_EN, HIGH); digitalWrite(PUMP3_R_EN, HIGH);
  analogWrite(PUMP3_L_PWM, 0); 

  setPump1Speed(0);
  setPump2Speed(0);
  setPump3Speed(0);

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
  
  Serial.println("INITIATING CASE 1");
}

void loop() {
  bool sumpOverfilled = (digitalRead(FLOAT_SENSOR_PIN) == LOW); 

  switch (currentState) {
    
    case CASE_1:
      if (sumpOverfilled) {
        floatWasTriggered = true; 
        digitalWrite(SOLENOID_PIN, LOW); 
        setPump1Speed(0);               
        setPump2Speed(255);           
      } else {
        if (floatWasTriggered) {
          currentState = TRANSITION;
          transitionStartTime = millis();
          digitalWrite(SOLENOID_PIN, LOW); 
          setPump1Speed(0);
          setPump2Speed(0);
          Serial.println("FLUSH SEQUENCE INITIATING IN 10 SECONDS");
          Serial.println("OPEN BALL VALVE");
        } else {
          digitalWrite(SOLENOID_PIN, HIGH); 
          setPump1Speed(127);               
          setPump2Speed(255);               
        }
      }
      break;

    case TRANSITION:
      if (millis() - transitionStartTime >= 10000) {
        currentState = CASE_2;
        flushLiters = 0.0; 
        Serial.println("\n INITIATING CASE 2");
      }
      break;

    case CASE_2:
      digitalWrite(SOLENOID_PIN, LOW);  
      setPump1Speed(255);              
      setPump2Speed(255);             
      setPump3Speed(255);          

      if (flushLiters >= 2.0) {   
        currentState = FINISHED;
        Serial.println("\n FLUSH COMPLETE");
      }
      break;

    case FINISHED:
      digitalWrite(SOLENOID_PIN, LOW);
      setPump1Speed(0);
      setPump2Speed(0);
      setPump3Speed(0);
      break;
  }

  if ((millis() - oldTime) >= 1000) {
    detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));
    unsigned int currentPulses = pulseCount; 
    pulseCount = 0; 
    oldTime = millis();
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);

    float flowRate_L_min = (float)currentPulses / 98.0;
    flowRate_mL_sec = flowRate_L_min * (1000.0 / 60.0);
    float liters_this_second = flowRate_L_min / 60.0;
    
    totalLiters += liters_this_second;

    if (currentState == CASE_2) {
      flushLiters += liters_this_second;
    }

    if (currentState != FINISHED) {
      Serial.print("Flow (L/min): ");
      Serial.print(flowRate_L_min, 3);
      
      if (currentState == CASE_2) {
        Serial.print("\tFlush Vol (L): ");
        Serial.print(flushLiters, 4);
        Serial.print("\tState: CASE 2 (FLUSHING)");
      } else if (currentState == TRANSITION) {
        Serial.print("\tWaiting: ");
        Serial.print(10 - ((millis() - transitionStartTime) / 1000));
        Serial.print("s");
        Serial.print("\tState: WAITING TO INITIATE FLUSH SEQUENCE");
      } else {
        Serial.print("\tTotal Vol (L): ");
        Serial.print(totalLiters, 4);
        Serial.print("\tState: ");
        Serial.print(sumpOverfilled ? "SUMP FULL" : "SUMP NOT FULL");
      }
      Serial.println();
    }
  }
}