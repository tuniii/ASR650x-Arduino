#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "SSD1306Wire.h"
/*
   set LoraWan_RGB to 1,the RGB active
   RGB red means sending;
   RGB green means received done;
*/

SSD1306Wire display(0x3c, SDA, SCL);

#ifndef LoraWan_RGB
#define LoraWan_RGB 0
#endif

#define RF_FREQUENCY                                470000000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
//  1: 250 kHz,
//  2: 500 kHz,
//  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
//  2: 4/6,
//  3: 4/7,
//  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void displayInof();
void sleep(void);
void RGB_test(void);

typedef enum
{
  LOWPOWER,
  RX,
  TX
} States_t;

int16_t txnumber;
States_t state;
bool sleepmode = false;
int16_t RSSI, rxSize;



void setup() {
  BoardInitMcu( );
  Serial.begin(115200);

  RGB_test();


  txnumber = 0;
  RSSI = 0;

  pinMode(P3_3, INPUT);
  attachInterrupt(P3_3, sleep, FALLING);
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;

  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                     LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                     LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                     true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                     LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                     LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                     0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

  state = TX;
}



void loop()
{
  switch (state)
  {
    case TX:
      delay(200);
      txnumber++;
      sprintf(txpacket, "%s", "hello");
      sprintf(txpacket + strlen(txpacket), "%d", txnumber);
      sprintf(txpacket + strlen(txpacket), "%s", " rssi : ");
      sprintf(txpacket + strlen(txpacket), "%d", RSSI);
      RGB_ON(0x100000, 0);

      Serial.printf("\r\nsending packet \"%s\" , length %d\r\n", txpacket, strlen(txpacket));

      Radio.Send( (uint8_t *)txpacket, strlen(txpacket) );
      state = LOWPOWER;
      break;
    case RX:
      Serial.println("into RX mode");
      Radio.Rx( 0 );
      state = LOWPOWER;
      break;
    case LOWPOWER:
      if (sleepmode)
      {
        Radio.Sleep( );
        Wire.end();
        detachInterrupt(RADIO_DIO_1);
        RGB_OFF();
        pinMode(GPIO0, ANALOG);
        pinMode(GPIO1, ANALOG);
        pinMode(GPIO2, ANALOG);
        pinMode(GPIO3, ANALOG);
        pinMode(GPIO5, ANALOG);
        pinMode(ADC, ANALOG);
      }
      LowPower_Handler();
      break;
    default:
      break;
  }
  Radio.IrqProcess( );
}

void OnTxDone( void )
{
  Serial.print("TX done......");
  displayInof();
  RGB_ON(0, 0);
  state = RX;
}

void OnTxTimeout( void )
{
  Radio.Sleep( );
  Serial.print("TX Timeout......");
  state = TX;
}
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
  gpioOn();
  RSSI = rssi;
  rxSize = size;
  memcpy(rxpacket, payload, size );
  rxpacket[size] = '\0';
  RGB_ON(0x001000, 100);
  RGB_ON(0, 0);
  Radio.Sleep( );

  Serial.printf("\r\nreceived packet \"%s\" with RSSI %d , length %d\r\n", rxpacket, RSSI, rxSize);
  Serial.println("wait to send next packet");
  displayInof();

  state = TX;
}

void displayInof()
{
  display.clear();
  display.drawString(0, 50, "Packet " + String(txnumber, DEC) + " sent done");
  display.drawString(0, 0,  "Received Size" + String(rxSize, DEC) + " packages:");
  display.drawString(0, 15, rxpacket);
  display.drawString(0, 30, "With RSSI " + String(RSSI, DEC));
  display.display();
}


void sleep(void)
{
  delay(10);
  if (digitalRead(P3_3) == 0)
  {
    sleepmode = true;
  }
}

void RGB_test(void)
{
  display.drawString(0, 20, "RGB Testing");
  display.display();
  for (uint32_t i = 0; i <= 30; i++)
  {
    RGB_ON(i << 16, 10);
  }
  for (uint32_t i = 0; i <= 30; i++)
  {
    RGB_ON(i << 8, 10);
  }
  for (uint32_t i = 0; i <= 30; i++)
  {
    RGB_ON(i, 10);
  }
  RGB_ON(0, 0);
}

void gpioOn(void)
{
  pinMode(GPIO0, OUTPUT);
  pinMode(GPIO1, OUTPUT);
  pinMode(GPIO2, OUTPUT);
  pinMode(GPIO3, OUTPUT);
  pinMode(GPIO5, OUTPUT);
  digitalWrite(GPIO0, HIGH);
  digitalWrite(GPIO1, HIGH);
  digitalWrite(GPIO2, HIGH);
  digitalWrite(GPIO3, HIGH);
  digitalWrite(GPIO5, HIGH);
}