#include <Wire.h>
#include "MAX30105.h"
#include <time.h>
#define buffer_length 200
#define output_length 5
uint16_t irbuffer[buffer_length], redbuffer[buffer_length], iractop[output_length], redactop[output_length], iracbottom[output_length], redacbottom[output_length];
int8_t irbuffer_d[buffer_length], redbuffer_d[buffer_length];
uint8_t flag = 0, ir_diff_treth=10, red_diff_treth, heart_t_treth=30,topbuffer[output_length], bottombuffer[output_length], sibuffer[output_length];
uint8_t output_flag = 0;
uint32_t a = 0, time_t1=0, top_temp; 
MAX30105 particleSensor; // initialize MAX30102 with I2C

void setup()
{
  Serial.begin(115200);
  while(!Serial); //We must wait for Teensy to come online
  delay(100);
  Serial.println("");
  Serial.println("MAX30102");
  delay(100);
  // Initialize sensor
  if (particleSensor.begin(Wire, I2C_SPEED_FAST) == false) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  byte ledBrightness = 35; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 400; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 16384; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  Serial.println("IR,RED,ir_d,red_d");
}


void loop() {
  int16_t temp;
  particleSensor.check(); //Check the sensor
  while (particleSensor.available()) {
      // read stored IR
      //Serial.print(micros());
      //Serial.print(",");
//      a =  millis() - a;
//      Serial.print("AAAAA:");
//      Serial.print(a);
//      a =  millis(); 
//取得波型
      irbuffer[flag] = - uint16_t(particleSensor.getIR());
      redbuffer[flag] = - uint16_t(particleSensor.getRed());
//一次微分
      temp = int16_t((irbuffer[flag] - irbuffer[flag-1>=0 ? flag-1 : buffer_length-1])*5);
//      Serial.print(temp);
//      Serial.print(",");
      if(temp >= 127)
      {
        irbuffer_d[flag] = 127;
      }
      else if(temp <= -128)
      {
        irbuffer_d[flag] = -128;
      }
      else
      {
        irbuffer_d[flag] = int8_t(temp);
      }
      temp = int16_t((redbuffer[flag] - redbuffer[flag-1>=0 ? flag-1 : buffer_length-1])*5);
//      Serial.print(temp);
      if(temp >= 127)
      {
        redbuffer_d[flag] = 127;
      }
      else if(temp <= -128)
      {
        redbuffer_d[flag] = -128;
      }
      else
      {
        redbuffer_d[flag] = int8_t(temp);
      }
      
//      redbuffer_d[flag] = int8_t(float(redbuffer[flag] - redbuffer[flag-1>=0 ? flag-1 : buffer_length-1])*3);
//      Serial.print(",");
      Serial.print(irbuffer_d[flag]);
      Serial.print(",");
////      // read stored red
      Serial.print(redbuffer_d[flag]);
      Serial.print(",");

//波峰
      if(irbuffer_d[flag-1>=0 ? flag-1 : buffer_length-1] >= 0 and irbuffer_d[flag] < 0 and (irbuffer_d[flag-4>=0 ? flag-4 : buffer_length-4] >= ir_diff_treth*0.7))
      {
        Serial.print(140);
        ir_diff_treth += irbuffer_d[flag-4>=0 ? flag-4 : buffer_length-4];
        ir_diff_treth /= 2;
        top_temp = time_t1;
        
      }
      else
      {
        Serial.print(0);
      }
      Serial.print(",");
      if(redbuffer_d[flag-1>=0 ? flag-1 : buffer_length-1] >= 0 and redbuffer_d[flag] < 0 and (redbuffer_d[flag-4>=0 ? flag-4 : buffer_length-4] >= red_diff_treth*0.7))
      {
        Serial.print(-140);
        red_diff_treth += redbuffer_d[flag-4>=0 ? flag-4 : buffer_length-4];
        red_diff_treth /= 2;
      }
      else
      {
        Serial.print(0);
      }
      Serial.print(",");
// 波谷
      if(irbuffer_d[flag-1 >= 0 ? flag-1 : buffer_length-1] <= 0 and irbuffer_d[flag] > 0 and (irbuffer_d[flag-4>=0 ? flag-4 : buffer_length-4] <= -1) and time_t1 - top_temp > heart_t_treth*0.8)
      {
        Serial.print(140);
        
      }
      else
      {
        Serial.print(0);
      }
      Serial.print(",");
      if(redbuffer_d[flag-1 >= 0 ? flag-1 : buffer_length-1] <= 0 and redbuffer_d[flag] > 0 and (redbuffer_d[flag-4>=0 ? flag-4 : buffer_length-4] <= -1) and time_t1 - top_temp > heart_t_treth*0.8)
      {
        Serial.print(-140);
        heart_t_treth += time_t1 - top_temp;
        top_temp = time_t1;
        heart_t_treth /= 2;
        if(heart_t_treth>50)
        {
          heart_t_treth = 30;
        }
      }
      else
      {
        Serial.print(0);
      }
      // read next set of samples
//      particleSensor.nextSample();   
      Serial.print(",");
//反射波
      if(irbuffer_d[flag-1>=0 ? flag-1 : buffer_length-1] >= 0 and irbuffer_d[flag] < 0 and time_t1 - top_temp <= heart_t_treth*0.7 and time_t1 - top_temp > heart_t_treth*0.2)
      {
        Serial.print(140);
      }
      else
      {
        Serial.print(0);
      }
      Serial.print(",");
      if(redbuffer_d[flag-1>=0 ? flag-1 : buffer_length-1] >= 0 and redbuffer_d[flag] < 0 and time_t1 - top_temp <= heart_t_treth*0.7 and time_t1 - top_temp > heart_t_treth*0.2)
      {
        Serial.println(-140);
      }
      else
      {
        Serial.println(0);
      }
      time_t1++;
      flag++;
      if(flag >= buffer_length)
      {
        flag = 0;
      }
  }
}
