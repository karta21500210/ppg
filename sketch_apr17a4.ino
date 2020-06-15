/*
   buffer_length 當示波器
   output_length 當輸出的
   irbuffer, redbuffer原始波
   iractop, redactop, iracbottom, redacbottom紀錄要輸出的AC值
   irbuffer_d, redbuffer_d 一次微分
   flag 波型用的游標
   output_flag 輸出用游標
   ir_diff_treth, red_diff_treth 微分閥值
   heart_t_treth 心率時間間隔閥值
   irtopbuffer, irbottombuffer, irsibuffer, redtopbuffer, redbottombuffer, redsibuffer記錄各時間間隔
   ir_si_temp1, red_si_temp1有明顯反射波紀錄時間
   ir_si_temp2, red_si_temp2沒有明顯反射波紀錄時間
   ir_out_ok, red_out_ok 輸出用的判斷
   ir_si_flag1, red_si_flag1有明顯反射波紀錄開關
   ir_si_flag2, red_si_flag2沒有明顯反射波紀錄開關
   time_t1, top_temp計算心率間隔用
*/
#include <Wire.h>
#include "MAX30105.h"
#include <time.h>
#define buffer_length 100
#define output_length 5
#include <SoftwareSerial.h>
SoftwareSerial BTSerial(10, 11);

uint16_t irbuffer[buffer_length], redbuffer[buffer_length], iractop[output_length], redactop[output_length], iracbottom[output_length], redacbottom[output_length];
uint32_t redtopbuffer[output_length], redbottombuffer[output_length], redsibuffer[output_length], irtopbuffer[output_length], irbottombuffer[output_length], irsibuffer[output_length];
int8_t irbuffer_d[buffer_length], redbuffer_d[buffer_length];
uint8_t flag = 0, ir_diff_treth = 10, red_diff_treth = 10, heart_t_treth = 30\
                                  , ir_si_temp1 = 0, red_si_temp1 = 0, ir_si_temp2 = 0, red_si_temp2 = 0;
uint8_t output_flag = 0, ir_out_ok = 0, red_out_ok = 0;
uint32_t a = 0, time_t1 = 0, top_temp;
float height = 150.0;
bool ir_si_flag1 = 0, ir_si_flag2 = 0, red_si_flag1 = 0, red_si_flag2 = 0;
MAX30105 particleSensor; // initialize MAX30102 with I2C

void setup()
{
  Serial.begin(9600);
  BTSerial.begin(9600);
  while (!Serial); //We must wait for Teensy to come online
  delay(100);
  // Initialize sensor
  if (particleSensor.begin(Wire, I2C_SPEED_FAST) == false) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }  
  /*
  Serial.println("Enter your height: XXX.X");
  char input_c[5];
  while(height < 100)
  {
    if(Serial.available())
    {
      Serial.readBytes(input_c,5);
      height =  atof(input_c);
    }
      
  }
  */
  Serial.println("OK!");
  byte ledBrightness = 35; //Options: 0=Off to 255=50mA
  byte sampleAverage = 10; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 800; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 16384; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  //  Serial.println("IR,RED,ir_d,red_d");
  Serial.println("HR, SI, SPO2");
  //先填滿BUFFER
  int16_t temp;
  bool ok = 1 ;
  while (ok)
  {
    particleSensor.check();
    while (particleSensor.available())
    {
      irbuffer[flag] = - uint16_t(particleSensor.getIR());
      redbuffer[flag] = - uint16_t(particleSensor.getRed());
      //一次微分
      temp = int16_t((irbuffer[flag] - irbuffer[flag - 1 >= 0 ? flag - 1 : buffer_length - 1]) * 5);
      //      Serial.print(temp);
      //      Serial.print(",");
      if (temp >= 127)
      {
        irbuffer_d[flag] = 127;
      }
      else if (temp <= -128)
      {
        irbuffer_d[flag] = -128;
      }
      else
      {
        irbuffer_d[flag] = int8_t(temp);
      }
      temp = int16_t((redbuffer[flag] - redbuffer[flag - 1 >= 0 ? flag - 1 : buffer_length - 1]) * 5);
      //      Serial.print(temp);
      if (temp >= 127)
      {
        redbuffer_d[flag] = 127;
      }
      else if (temp <= -128)
      {
        redbuffer_d[flag] = -128;
      }
      else
      {
        redbuffer_d[flag] = int8_t(temp);
      }
      flag++;
      if (flag >= buffer_length)
      {
        flag = 0;
        ok = 0;
        break;
      }
    }
  }
}
boolean receiveFlag = false;
void loop() {
  int16_t temp;
  particleSensor.check(); //Check the sensor
  //Serial.println("h:"+String(height));
  while (particleSensor.available()) {

    float h;
    char val;
    String recieveData;

    while(BTSerial.available()) //如果有收到資料  
    {   
      receiveFlag = true;   
      val=BTSerial.read(); //每次接收一個字元
      recieveData += val; //字元組成字串  
    }
    if(receiveFlag){
      receiveFlag = false;
      height = recieveData.toFloat();     
    }
    
    
    // read stored IR
    //    Serial.print(micros());
    //        Serial.print(",");
    //    a =  millis() - a;
    //    Serial.print("AAAAA:");
    //    Serial.println(a * 10);
    //    a =  millis();
    //取得波型
    irbuffer[flag] = - uint16_t(particleSensor.getIR());
    redbuffer[flag] = - uint16_t(particleSensor.getRed());
    //一次微分
    temp = int16_t((irbuffer[flag] - irbuffer[flag - 1 >= 0 ? flag - 1 : buffer_length - 1]) * 5);
    //      Serial.print(temp);
    //      Serial.print(",");
    if (temp >= 127)
    {
      irbuffer_d[flag] = 127;
    }
    else if (temp <= -128)
    {
      irbuffer_d[flag] = -128;
    }
    else
    {
      irbuffer_d[flag] = int8_t(temp);
    }
    temp = int16_t((redbuffer[flag] - redbuffer[flag - 1 >= 0 ? flag - 1 : buffer_length - 1]) * 5);
    //      Serial.print(temp);
    if (temp >= 127)
    {
      redbuffer_d[flag] = 127;
    }
    else if (temp <= -128)
    {
      redbuffer_d[flag] = -128;
    }
    else
    {
      redbuffer_d[flag] = int8_t(temp);
    }

    //      redbuffer_d[flag] = int8_t(float(redbuffer[flag] - redbuffer[flag-1>=0 ? flag-1 : buffer_length-1])*3);
    //      Serial.print(",");
    //    Serial.print(irbuffer_d[flag]);
    //    Serial.print(",");
    //    ////      // read stored red
    //    Serial.print(redbuffer_d[flag]);
    //    Serial.print(",");

    //波峰
    if (irbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] >= 0 and irbuffer_d[flag] < 0 and (irbuffer_d[flag - 4 >= 0 ? flag - 4 : buffer_length - 4] >= ir_diff_treth * 0.7))
    {
      //      Serial.print(140);
      ir_diff_treth += irbuffer_d[flag - 4 >= 0 ? flag - 4 : buffer_length - 4];
      ir_diff_treth /= 2;
      top_temp = time_t1;
      //將波峰紀錄
      ir_out_ok++;
      //      irtopbuffer[output_flag] = (flag - 1 >= 0) ? flag - 1  : buffer_length - 1;
      irtopbuffer[output_flag] = time_t1 - 1;
      iractop[output_flag] = irbuffer[flag - 1];
      ir_si_temp1 = 0;
      ir_si_temp2 = 0;
      ir_si_flag1 = 0;
      ir_si_flag2 = 0;

    }
    else
    {
      //      Serial.print(0);
    }
    //    Serial.print(",");
    if (redbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] >= 0 and redbuffer_d[flag] < 0 and (redbuffer_d[flag - 4 >= 0 ? flag - 4 : buffer_length - 4] >= red_diff_treth * 0.7))
    {
      //      Serial.print(-140);
      red_diff_treth += redbuffer_d[flag - 4 >= 0 ? flag - 4 : buffer_length - 4];
      red_diff_treth /= 2;
      red_out_ok++;
      //      redtopbuffer[output_flag] = (flag - 1 >= 0) ? flag - 1  : buffer_length - 1;
      redtopbuffer[output_flag] = time_t1 - 1;
      redactop[output_flag] = redbuffer[flag - 1];
      red_si_temp1 = 0;
      red_si_temp2 = 0;
      red_si_flag1 = 0;
      red_si_flag2 = 0;
    }
    else
    {
      //      Serial.print(0);
    }
    //    Serial.print(",");
    // 波谷
    if (time_t1 - top_temp > heart_t_treth * 0.8)
    {
      if (irbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] <= 0 and irbuffer_d[flag] > 0 and (irbuffer_d[flag - 4 >= 0 ? flag - 4 : buffer_length - 4] <= -1))
      {
        //        Serial.print(140);
        ir_out_ok++;
        //        irbottombuffer[output_flag] = (flag - 1 >= 0) ? flag - 1  : buffer_length - 1;
        irbottombuffer[output_flag] = time_t1 - 1;
        iracbottom[output_flag] = irbuffer[flag - 1];
        //找反射波
        if (ir_si_flag1 == 1)
        {
          irsibuffer[output_flag] = ir_si_temp1;
        }
        else if (ir_si_flag1 == 0 and ir_si_flag2 == 1)
        {
          irsibuffer[output_flag] = ir_si_temp2;
        }
      }
      else
      {
        //        Serial.print(0);
      }
      //      Serial.print(",");
      if (redbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] <= 0 and redbuffer_d[flag] > 0 and (redbuffer_d[flag - 4 >= 0 ? flag - 4 : buffer_length - 4] <= -1))
      {
        //        Serial.print(-140);
        heart_t_treth += time_t1 - top_temp;
        top_temp = time_t1;
        heart_t_treth /= 2;
        if (heart_t_treth > 50)
        {
          heart_t_treth = 30;
        }
        red_out_ok++;
        //        redbottombuffer[output_flag] = (flag - 1 >= 0) ? flag - 1  : buffer_length - 1;
        redbottombuffer[output_flag] = time_t1 - 1;
        redacbottom[output_flag] = redbuffer[flag - 1];
        //找反射波
        if (red_si_flag1 == 1)
        {
          redsibuffer[output_flag] = red_si_temp1;
        }
        else if (red_si_flag1 == 0 and red_si_flag2 == 1)
        {
          redsibuffer[output_flag] = red_si_temp2;
        }
      }
      else
      {
        //        Serial.print(0);
      }
    }
    else
    {

      //      Serial.print("0,0");
    }

    // read next set of samples
    //      particleSensor.nextSample();
    //    Serial.print(",");
    //反射波 明顯

    if (time_t1 - top_temp <= heart_t_treth * 0.7 and time_t1 - top_temp > heart_t_treth * 0.17)
    {
      if (irbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] >= 0 and irbuffer_d[flag] < 0 and ir_si_flag1 == 0)
      {
        //        Serial.print(140);
        ir_si_flag1 = 1;
        //        ir_si_temp1 = (flag - 1 >= 0) ? flag - 1  : buffer_length - 1;
        ir_si_temp1 = time_t1 - 1;
      }//因為記憶體空間不夠只能用這樣
      else if (ir_si_flag2 == 0 and irbuffer_d[flag] < 0 and irbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] < 0 \
               and irbuffer_d[flag - 2 >= 0 ? flag - 2 : buffer_length - 2] < 0  and irbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] > irbuffer_d[flag]\
               and irbuffer_d[flag - 2 >= 0 ? flag - 2 : buffer_length - 2] < irbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1])
      {
        //        Serial.print(140);
        ir_si_flag2 = 1;
        //        ir_si_temp2 = flag;
        ir_si_temp2 = time_t1;

      }
      else
      {
        //        Serial.print(0);
      }
      //      Serial.print(",");
      if (redbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] >= 0 and redbuffer_d[flag] < 0 and red_si_temp1 == 0)
      {
        //        Serial.println(-140);
        red_si_flag1 = 1;
        //        red_si_temp1 = (flag - 1 >= 0) ? flag - 1  : buffer_length - 1;
        red_si_temp1 = time_t1 - 1;

      }
      else if (red_si_flag2 == 0 and redbuffer_d[flag] < 0 and redbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] < 0 \
               and redbuffer_d[flag - 2 >= 0 ? flag - 2 : buffer_length - 2] < 0  and redbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1] > redbuffer_d[flag]\
               and redbuffer_d[flag - 2 >= 0 ? flag - 2 : buffer_length - 2] < redbuffer_d[flag - 1 >= 0 ? flag - 1 : buffer_length - 1])
      {
        //        Serial.println(-140);
        red_si_flag2 = 1;
        //        red_si_temp2 = flag;
        red_si_temp2 = time_t1;
      }
      else
      {
        //        Serial.println(0);
      }
    }
    else
    {
      //      Serial.println("0,0");
    }
    time_t1++;
    flag++;
    if (flag >= buffer_length)
    {
      flag = 0;
    }
    //找到正常的波
    if ( ir_out_ok >= 2 and red_out_ok >= 2)
    {
      output_flag++;
      ir_out_ok = 0;
      red_out_ok = 0;

      //找到完全的
      if (output_flag >= output_length)
      {
        output_flag = 0;
        int item = 0, si_count = 0, heart_count = 0;
        float output_time = 0, si_time = 0, irac = 0, irdc = 0, redac = 0, reddc = 0, heart_rate_output = 0, ROS = 0, SPO2 = 0;

        for (item = 0; item < output_length; item++)
        {
          //計算心率
          //          if (irbottombuffer[item] >= irtopbuffer[item])
          //          {
          //            output_time += float(-irtopbuffer[item] + irbottombuffer[item]);
          //          }
          //          else
          //          {
          //            output_time += float(-irtopbuffer[item] + irbottombuffer[item] + buffer_length);
          //          }
          //          if (redtopbuffer[item] <= redbottombuffer[item])
          //          {
          //            output_time += float(-redtopbuffer[item] + redbottombuffer[item]);
          //          }
          //          else
          //          {
          //            output_time += float(-redtopbuffer[item] + redbottombuffer[item] + buffer_length);
          //          }
          if (-irtopbuffer[item] + irbottombuffer[item] <= 50 and - irtopbuffer[item] + irbottombuffer[item] >= 20)
          {
            output_time += float(-irtopbuffer[item] + irbottombuffer[item]);
            irac += float(iractop[item] - iracbottom[item]);
            redac += float(redactop[item] - redacbottom[item]);
            irdc += float(iracbottom[item]);
            reddc += float(redacbottom[item]);
            heart_count ++;
          }

          if (-redtopbuffer[item] + redbottombuffer[item] <= 45 and - redtopbuffer[item] + redbottombuffer[item] >= 18)
          {
            output_time += float(-redtopbuffer[item] + redbottombuffer[item]);
            irac += float(iractop[item] - iracbottom[item]);
            redac += float(redactop[item] - redacbottom[item]);
            irdc += float(iracbottom[item]);
            reddc += float(redacbottom[item]);
            heart_count ++;
          }


          //計算反射SI
          if (irsibuffer[item] != 0)
          {
            if (irsibuffer[item] > irtopbuffer[item] and float(-irtopbuffer[item] + irsibuffer[item]) <= 23 and float(-irtopbuffer[item] + irsibuffer[item]) >= 1)
            {
              si_time += float(-irtopbuffer[item] + irsibuffer[item]);
              si_count++;
            }

          }
          if (redsibuffer[item] != 0)
          {
            if (redtopbuffer[item] < redsibuffer[item] and float(-redtopbuffer[item] + redsibuffer[item]) <= 23 and float(-irtopbuffer[item] + irsibuffer[item]) >= 1)
            {
              si_time += float(-redtopbuffer[item] + redsibuffer[item]);
              si_count++;
            }

          }


          //計算血氧


          //          Serial.print(irbottombuffer[item]);
          //          Serial.print(",");
          //          Serial.print(irtopbuffer[item]);
          //          Serial.print(",");
          //          Serial.print( redbottombuffer[item]);
          //          Serial.print(",");
          //          Serial.println( redtopbuffer[item]);
          irsibuffer[item] = 0;
          redsibuffer[item] = 0;
          //          Serial.print(si_time);
          //          Serial.print(",");
          //          Serial.print(time_t1 * 50);
          //          Serial.print(",");
          //          Serial.println(ROS);
        }
        //        output_time /= heart_count;
        si_time /= si_count > 0 ? si_count : 0;
        output_time *= 0.025;
        si_time *= 0.025;
        irac /= heart_count;
        redac /= heart_count;
        irdc /= heart_count;
        reddc /= heart_count;
        //        heart_rate_output = output_length / output_time * 60;
        heart_rate_output = heart_count / output_time * 60;
        ROS = log((redac + reddc) / reddc) / log((irac + irdc) / irdc);
        SPO2 = (0.86 - 0.2 * ROS) / (0.74 + 0.09 * ROS);
         //-45.060*n_ratio_average* n_ratio_average/10000 + 30.354 *n_ratio_average/100 + 94.845 ; 
        SPO2 = -45.06*ROS*ROS/10000 + 30.354*ROS/100 +94.845;
        if (heart_rate_output >= 60 and heart_rate_output <= 150)
        {
          Serial.print(heart_rate_output);
          Serial.print(",");
          Serial.print(height*0.01 / si_time);
          Serial.print(",");
          //          Serial.print(time_t1 * 50);
          //          Serial.print(",");
          //          Serial.print(si_count);
          //          Serial.print(",");
          //          Serial.println(SPO2);
          Serial.println(SPO2);
          BTSerial.print("A:"+String(heart_rate_output));
          delay(200);
          BTSerial.print("B:"+String((height*0.01 / si_time)));
          delay(200);
          BTSerial.print("C:"+String((SPO2)));
        }

        time_t1 = 1;

      }
    }

  }
}
