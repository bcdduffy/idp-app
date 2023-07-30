#include <MemoryFree.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
//#include <cppQueue.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <avr/sleep.h>
//#include <LowPower.h>

#include <Wire.h>
#include <SPI.h>
//#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//#i nclude <TimeLib.h>
//#include <DS1307RTC.h>


#define EXTERNAL_INTER_SEN 3

#define  MAX_LENGTH    50
#define BAUDRATE (16000000/16/9600-1)

#define USART_BAUDRATE 9600


#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)


Adafruit_BME280 bme; // I2C

volatile bool tempStatusFlag = false;

const int rainPin = 5;
const int sensor_pin = 8;
SemaphoreHandle_t xSemaphore;

QueueHandle_t queueTx;
QueueHandle_t queueRx;
QueueHandle_t messageBuffer;
QueueHandle_t dataQ;

String temp = "noData";
String temp2 = "noData";

bool empty = true;
volatile bool session = false;
volatile bool tasksDestroy = false;
volatile bool transOff = true;

TaskHandle_t xParse = NULL;
TaskHandle_t xTempData = NULL;


//volatile String currentData;

void ParseMessage(void *pvParameters);
void handleTempData(void *pvParameters);

void setup()
{
    pinMode(2, INPUT);
    //rtcPulseInit();
    //---------------------------------------------------------------//INIT UART START
    cli();
    if (UCSR0B != (1 << TXEN0)) 
    {

        UBRR0H = (unsigned char)(BAUDRATE >> 8);
        UBRR0L = (unsigned char)BAUDRATE;
        UCSR0A = 0;
        UCSR0B = (1 << TXEN0);
        UCSR0C = (3 << UCSZ00);
    }
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0B |= (1 << RXCIE0) ;
    UCSR0B |= (0 << UDRIE0);  // enable TX interrupt

    sei();
    //---------------------------------------------------------------//INIT UART END

   // initSensors();

    // timerSet();

    // rtcPulseInit();

    //---------------------------------------------------------------//INIT RTOS START
    queueTx = xQueueCreate(60, sizeof(char));
    queueRx = xQueueCreate(20, sizeof(char));
    messageBuffer = xQueueCreate(20, sizeof(char));
    dataQ = xQueueCreate(50, sizeof(char));

    xSemaphore = xSemaphoreCreateBinary();

    if (queueTx == NULL) {}
    if (queueRx == NULL) {}


    // Create task for Arduino led 
    xTaskCreate(ParseMessage, // Task function
                "ParseMessage", // Task name
                128, // Stack size 
                NULL, 
                1 ,// Priority
                &xParse );

    xTaskCreate(handleTempData, // Task function
                "handleTempData", // Task name
                205, // Stack size 
                NULL, 
                1 ,// Priority
                &xTempData );

              //  vTaskSuspend(xTempData);
              //  vTaskSuspend(xParse);

     vTaskStartScheduler();
   //  sleeping();
    //---------------------------------------------------------------//INIT RTOS END 
}

void loop()
{


}

//------------------------------------------------------// RTOS task code START
void ParseMessage(void *pvParameters)
{
    (void) pvParameters;
    uint8_t receive;
    uint8_t sender;
    //vTaskSuspend(xTempData);
    static String message = "";
    for (;;) {
        if(xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdPASS){
            
                if(xQueueReceive(queueRx, &receive, 0))
                {
                    if(receive == 's')
                    {
                     // vTaskResume(xTempData);
                      session = true;          
                      sender = receive;
                      xQueueSend(queueTx, &sender, 0);
                      tempStatusFlag =true;
                      cli();
                    UCSR0B &= (0 << UDRIE0); // disable tx 
                    UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
                      sei();
                    transOff =true;
                      
                    }
                    else if(receive == 'e')
                    {
                      session = false; 
                      tempStatusFlag = false;  
                      tasksDestroy = true;   
                    cli();
                    UCSR0B &= (0 << UDRIE0); // disable tx 
                    UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
                    sei();     
                     //vTaskEndScheduler();
                    vTaskSuspend(xTempData);
                    vTaskSuspend(xParse);    
                    //sleeping();                                 
                    }
                    else if(receive == 'd' && session){
                     
                      //taskENTER_CRITICAL(); 
                      while(xQueueReceive(dataQ, &sender, 0))
                        {
                            xQueueSend(queueTx, &sender, 0);
                            vTaskDelay(1);
                        }
                    empty = true;
                    
                    cli();
                    UCSR0B &= (0 << UDRIE0); // disable tx 
                    UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
                      sei();
                    transOff =true;
                  
                    // taskEXIT_CRITICAL();
                   
                    }

                }
                xSemaphoreGive( xSemaphore ); 
                vTaskDelay(1);
            }
        }
    }



void handleTempData(void *pvParameters)
{
    (void) pvParameters;
    uint8_t sender;
    //unsigned status;
    // status = bme.begin(0x76);
     
    for (;;) 
    {
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdPASS){
            if(tempStatusFlag)
            {
               unsigned status;
               status = bme.begin(0x76);
               char array1[7];
               char array2[7];
               char array3[7];
              
               String letters[7] = { String("{\"temp\":"), String("na"),  
                                     String(", \"hm\":"), String("na"),
                                     String(", \"pre\":"), String("na"),
                                     String("}") };
                                    
                float tempF = bme.readTemperature();
                float pres = bme.readPressure() / 100.0F;
                float hum = bme.readHumidity();

                dtostrf(tempF, 4, 2, array1); // fix to handle non 2 digit numbers
                dtostrf(pres,  5, 2, array2); // fix to handle non 2 digit numbers
                dtostrf(hum,4, 2, array3); // fix to handle non 2 digit numbers 
                
                char *tests1 = array1;
                String b =  String(tests1); 
                letters[1] = b;
                
                char *tests2  = array2;  
                String c = String(tests2);
                letters[5] = c;
                
                   
                char *tests4 = array3;
                String d =  String(tests4); 
                letters[3] = d;
          taskENTER_CRITICAL();      
          if(empty){
               for(int i = 0; i < 7; i++){
                for(int j = 0; j < letters[i].length(); j++)
                {
                  char dataC = letters[i].charAt(j);
                  xQueueSend(dataQ, &dataC, 0);
                }
               }
               empty = false;
          }
          taskEXIT_CRITICAL();
              
                xSemaphoreGive( xSemaphore );
                vTaskDelay(1);
            }


        }


    }
}

//------------------------------------------------------// RTOS task code END




//------------------------------------------------------// ISR code START
ISR(USART_RX_vect)
{
    BaseType_t xHigherPrioritTaskWoken = pdFALSE;
    char a = UDR0;
    if(a == 's' || transOff)
    {
      //sleep_disable();
      UCSR0B |= (1 << UDRIE0);  // enable TX interrupt
      xTaskResumeFromISR(xParse); 
      xTaskResumeFromISR(xTempData); 
      transOff = false;
    }
 
    xQueueSendFromISR(queueRx, &a, &xHigherPrioritTaskWoken);
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPrioritTaskWoken);
   
}


ISR(USART_UDRE_vect) { //Data Register Empty
    
    char d;
    BaseType_t xHigherPrioritTaskWoken = pdFALSE;
    xQueueReceiveFromISR(queueTx, &d, &xHigherPrioritTaskWoken);
    UDR0 = d;
}


//TODO - ISR FOR DATA collection
/*
ISR(INT0_vect)
{
    BaseType_t xHigherPrioritTaskWoken = pdFALSE;
    //tempStatusFlag = true;
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPrioritTaskWoken);
    
}
*/




//TODO - ISR for Battery Mode




//------------------------------------------------------// ISR code END




//-------------------------------------------------------// INIT Functions for data and Time START
void initSensors(){

    pinMode(rainPin, INPUT);
    pinMode(sensor_pin, INPUT);
}



//-------------------------------------------------------// INIT Functions for data and Time END



//-------------------------------------------------------// data START


//-------------------------------------------------------// data END


void rtcPulseInit() {

    Wire.begin();
    Wire.beginTransmission(0x68);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.end();

    /*
  //-------------
  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x07);
  Wire.write(0x10);  // Set Square Wave to 1 Hz
  Wire.endTransmission();
  Wire.end();
     */

}


//-------------------------------------------------------// Time END

void sleeping()
{
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_cpu();
}
