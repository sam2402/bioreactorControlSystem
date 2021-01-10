/*
 sketch.ino

 Sricharan Sanakkayala, Dec 2020

 This is the completed sketch for the testboard simduino

 */

#include <stdio.h>
#include <string.h>
#include <Wire.h>

// These define which pins are connected to what device on the virtual bioreactor
//
const byte lightgatePin  = 2;
const byte stirrerPin    = 5;
const byte heaterPin     = 6;
const byte thermistorPin = A0;
const byte pHPin         = A1;

// The PCA9685 is connected to the default I2C connections. There is no need
// to set these explicitly.

int x;

int setPoint = 55;

float heating = 27;
float stirring = 1100;
float ph = 5;

float temp = 0;
float rpm = 0;
float phVal = 0;

boolean reachedpH=false;

//stirring vars

// Stirring
// These are global as the ISR cannot take parameters
volatile int count_rotations = 0; // Varies between 0-1 to count when a full rotation has occurred
volatile unsigned long time_at_last_rotation = millis(); // Time at last rotation
volatile unsigned long time_for_1_rotation = 0.0; // Last time recorded for 1 full rotation
const int max_pwm = 255;
// STIRRING variables
  int stir_maximum_vals = 1;
  int stir_user_input = 1500;
 // float rpm = 0.0;
  int stirrer_pwm_val = 0;
  int stir_num_results = 0;

void setup() {
  Serial.begin(9600);

  pinMode(lightgatePin,  INPUT);
  pinMode(stirrerPin,    OUTPUT);
  pinMode(heaterPin,     OUTPUT);
  pinMode(thermistorPin, INPUT);
  pinMode(pHPin,         INPUT);

  attachInterrupt(digitalPinToInterrupt(lightgatePin), ISR_get_rpm, FALLING);

}

void updateSetpoints(){
  if (Serial.available()){
  heating = Serial.parseFloat();
  stirring = Serial.parseFloat();
  ph = Serial.parseFloat();
  reachedpH=false;
  }
}

void sendMsg(String msg){
//  Serial.println(msg);

  if (Serial.available() <= 0) {
    Serial.println(msg);
    Serial.flush();
  }
}

void getpHVal(){

  //pH vars
unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temporary;
  for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  {
    buf[i]=analogRead(pHPin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temporary=buf[i];
        buf[i]=buf[j];
        buf[j]=temporary;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  float phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue=3.5*phValue - 1.48;                      //convert the millivolt into pH value
  phVal = phValue;
}

void setpHVal(){
  Wire.beginTransmission(0x40);
  Wire.write(0x00);
  Wire.write(0x21);
  Wire.endTransmission();

  if(reachedpH==false){
  if(ph> (phVal+.02)){
    Wire.beginTransmission(0x40);
    Wire.write(0x06);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0xFF);
    Wire.write(0x0F);
    Wire.endTransmission();

    Wire.beginTransmission(0x40);
    Wire.write(0x0A);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.write(0xFF);
    Wire.write(0x0F);
    Wire.endTransmission();

  }else if(ph< (phVal-.02)){
    Wire.beginTransmission(0x40);
    Wire.write(0x0A);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0xFF);
    Wire.write(0x0F);
    Wire.endTransmission();

    Wire.beginTransmission(0x40);
    Wire.write(0x06);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.write(0xFF);
    Wire.write(0x0F);
    Wire.endTransmission();
  }
  else{
    Wire.beginTransmission(0x40);
    Wire.write(0x06);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0xFF);
    Wire.write(0x0F);
    Wire.endTransmission();

    Wire.beginTransmission(0x40);
    Wire.write(0x0A);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0xFF);
    Wire.write(0x0F);
    Wire.endTransmission();
    reachedpH=true;
  }
  }
  Wire.beginTransmission(0x40);
  Wire.write(0x00);
  Wire.write(0x10);
  Wire.endTransmission();
}



void getTemp()
{
float b;
int buf[10],temporary;
for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  {
    buf[i]=analogRead(thermistorPin);
    delay(20);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temporary=buf[i];
        buf[i]=buf[j];
        buf[j]=temporary;
      }
    }
  }
  float avgTemp=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgTemp+=buf[i];

float RT, B =4220, RN=10000, TN = 298.15;
float V2 = (avgTemp/6)/1023 * 5;
float I = (5-V2)/10000;
RT = V2/I;
temp = (B*TN)/(log(RT/RN)* TN +B) - 273.15;
}

void setTemp(){
  if(temp>heating-1.5){
  digitalWrite(heaterPin, LOW);
}else{
  digitalWrite(heaterPin, HIGH);
}
}

void stirringFunc(){
    float last_rpms[stir_maximum_vals];
   rpm = calculate_rpm();
    take_running_avg(last_rpms, stir_num_results, rpm, stir_maximum_vals);
    if (stir_num_results < stir_maximum_vals) {
      stir_num_results++;
    }
    else {
      rpm = get_avg(last_rpms, stir_num_results);
      stirrer_pwm_val = change_rpm(rpm, stirring, stirrer_pwm_val);
    }
}

// -------- STIRRING SUBSYSTEM FUNCTIONS ---------

void ISR_get_rpm() {
  // NOTE: needs 3 GLOBAL VARIABLES, count_rotations, time_at_last_rotation and time_for_1_rotation
  // Called 2 times per revolution, hence first time just increments count_rotations
  if (count_rotations == 0) {
    count_rotations = 1;
  }
  else {
    count_rotations = 0;
    time_for_1_rotation = millis() - time_at_last_rotation;
    time_at_last_rotation = millis();
  }
}
float calculate_rpm() {
float b;
int buf[10],temporary;
for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  {
    // Requires global variable time_for_1_rotation -> milliseconds for 1 rotation to take place
  float seconds_for_1_rotation = time_for_1_rotation * 0.001;
  if (seconds_for_1_rotation == 0) {
    return 0;
  }
    buf[i]= 60 / seconds_for_1_rotation;
    delay(12);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temporary=buf[i];
        buf[i]=buf[j];
        buf[j]=temporary;
      }
    }
  }
  float avgRpm=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgRpm+=buf[i];
float rpm = (avgRpm/6);
  return rpm;
}

void output_rpm(float rpm) {
  Serial.print("RPM: ");
  Serial.println(rpm, 3);
  delay(1000);
}


int change_rpm(float current_rpm, int target_rpm, int stirrer_pwm_val) {
  if(abs (current_rpm - target_rpm) >= 50){
  if (current_rpm < target_rpm) {
    stirrer_pwm_val = stirrer_pwm_val + calculate_change_req_rpm(current_rpm, target_rpm);
  }
  else if (current_rpm > target_rpm){
    stirrer_pwm_val = stirrer_pwm_val - calculate_change_req_rpm(current_rpm, target_rpm);
  }
  }else{
  if (current_rpm < target_rpm-20) {
    stirrer_pwm_val = stirrer_pwm_val + 1;
//    calculate_change_req_rpm(current_rpm, target_rpm);
  }
  else if (current_rpm > target_rpm+20){
    stirrer_pwm_val = stirrer_pwm_val -1;
   // calculate_change_req_rpm(current_rpm, target_rpm);
  }
  }
  stirrer_pwm_val = bound_integer(stirrer_pwm_val, max_pwm, 0);
  analogWrite(stirrerPin, stirrer_pwm_val);
  return stirrer_pwm_val;
}

int calculate_change_req_rpm(float current_rpm, int target_rpm) {
  if (abs(current_rpm - target_rpm) > 150) {
    return 20;
  }
  else {
    return 2;
  }
}


float* take_running_avg(float last_vals[], int num_results, float new_value, int max_vals) {
   if (num_results < max_vals) {
     last_vals[num_results] = new_value;
   }
   else if (num_results == max_vals) {
     for (int i=0; i<max_vals; i++) {
       if (i==max_vals-1) {
         last_vals[i] = new_value;
       }
       else {
         last_vals[i] = last_vals[i+1];
       }
     }
   }
   return last_vals;
}


float get_avg(float values[], int num_values) {
  float total = 0.0;
  for (int i=0; i<num_values; i++) {
    total = total + values[i];
  }
  return total/num_values;
}

int bound_integer (int num, int upper, int lower) {
  if (num > upper) {
    return upper;
  }
  else if (num < lower) {
    return lower;
  }
  return num;
}

void loop() {
  updateSetpoints();
  sendMsg(String(temp)+" "+String(rpm)+" "+String(phVal));
  getTemp();
  setTemp();
  stirringFunc();
  getpHVal();
  setpHVal();
}
