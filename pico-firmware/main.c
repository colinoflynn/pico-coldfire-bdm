// Coldfire BDM Serial Bridge
// by Peter Sobot (@psobot), November 8, 2022
// Ported to R-Pi Pico by Colin O'Flynn, 2024

#include <stdio.h>
#include "pico/stdlib.h"

#define PIN_LED PICO_DEFAULT_LED_PIN

// Pins are nothing special - GPIO is used for this platform.
// The selected pins could use the SPI peripheral block in the future although it's probably
// not needed for any reason

#define PIN_RESET 0
// Two inputs to the Coldfire:
#define PIN_DSCLK 2
#define PIN_DSI 3

// Trigger a breakpoint by pulling this low:
// Low = 1, High = 0
#define PIN_BKPT 5

// One output from the Coldfire, which is visible a couple CPU clock cycles
// after the fall of DSCLK. In practice, this means we only need to wait ~30
// nanoseconds between fall of DSCLK and seeing the correct value on DSO.
#define PIN_DSO 4


// Serial data is sent in 17-bit packets:
//  Receive (by the Coldfire): [status] [16 bits of data]
//  Send (to the Coldfire): [0] [16 bits of data]

typedef struct Packet {
  uint8_t status;
  uint16_t data;
} Packet;

// The protocol here is technically full-duplex; commands can be sent and
// received simultaneously.
bool sendAndReceiveBit(uint8_t bitToSend) {
  gpio_put(PIN_DSI, bitToSend);
  sleep_us(1);
  gpio_put(PIN_DSCLK, 1);
  sleep_us(1);
  gpio_put(PIN_DSCLK, 0);
  sleep_us(1);
  return gpio_get(PIN_DSO);
}

bool receiveBit() { return sendAndReceiveBit(0); }

void sendBit(uint8_t bit) { sendAndReceiveBit(bit); }

Packet sendAndReceivePacket(uint16_t dataToSend) {
  Packet packet;

  packet.status = 0;
  packet.data = 0;

  packet.status = sendAndReceiveBit(0);
  for (int i = 15; i >= 0; i--) {
    packet.data |= sendAndReceiveBit((dataToSend >> i) & 1) << i;
  }

  return packet;
}

Packet receivePacket() { return sendAndReceivePacket(0); }

void sendPacket(uint16_t data) {
  sendBit(0);

  for (int i = 15; i >= 0; i--) {
    char singleBit = (data >> i) & 1;
    sendBit(singleBit);
  }
}

void enterDebugMode(bool reset) {
  gpio_put(PIN_BKPT, 0);
  gpio_set_dir(PIN_BKPT, GPIO_OUT);
  sleep_ms(50);

  if (reset) {
    gpio_put(PIN_RESET, 0);
    gpio_set_dir(PIN_RESET, GPIO_OUT);
    sleep_ms(50);

    gpio_set_dir(PIN_RESET, GPIO_IN);
    sleep_ms(50);
  }

  gpio_set_dir(PIN_BKPT, GPIO_IN);
  sleep_ms(50);
}

uint16_t getNextTwoBytesFromUSB() {
  uint16_t data = ((uint16_t)getchar()) << 8;
  data |= getchar();
  return data;
}

void write(char * data, int len){
    while(len--){
        putchar(*(data++));
    }
    return;
}

int main() {
    stdio_init_all();
    stdio_set_translate_crlf(&stdio_usb, false);

    //WARNING: Default library needs flow control set to RTS/CTS - otherwise port will
    // not connect. The LED turns on once port is detected in firmware.
    while (!stdio_usb_connected());

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 1);

    //No capital N in here for NACK-as-ready-logic
    fputs("Motorola Coldfire Debug Interface by Peter Sobot\n", stdout);
    fputs("(Minor porting by Colin O'Flynn)\n", stdout);

    gpio_init(PIN_DSCLK);
    gpio_set_dir(PIN_DSCLK, GPIO_OUT);

    gpio_init(PIN_DSI);
    gpio_set_dir(PIN_DSI, GPIO_OUT);

    gpio_init(PIN_DSO);
    gpio_set_dir(PIN_DSO, GPIO_IN);

    gpio_init(PIN_RESET);
    gpio_set_dir(PIN_RESET, GPIO_IN);

    gpio_init(PIN_BKPT);
    gpio_set_dir(PIN_BKPT, GPIO_IN);

    //gpio_set_drive_strength(PIN_DSCLK, 0);
    //gpio_set_drive_strength(PIN_DSI, 0);

    //No capital N in here for NACK-as-ready-logic
    fputs("Ready.\n", stdout);
    fflush(stdout);

    while(1){
        int command = getchar();

        switch (command) {
            case 'P': // for Ping
                write("PONG", 4);
                putchar('A'); //ack
                break;
            case 'B': // for Breakpoint
                enterDebugMode(false);
                putchar('A'); //ack
                break;
            case 'R': // for Reset
                enterDebugMode(true);
                putchar('A'); //ack
                break;
            case 'S': { // for Send-and-Receive
                uint16_t data = getNextTwoBytesFromUSB();
                Packet packet = sendAndReceivePacket(data);
                putchar(packet.status ? 'Y' : 'N');
                write((char *)&packet.data, sizeof(packet.data));
                putchar('A'); //ack
                break;
            }
            case 's': { // for Send
                sendPacket(getNextTwoBytesFromUSB());
                putchar('A'); //ack
                break;
            }
            case 'r': { // for Receive
                Packet packet = receivePacket();
                putchar(packet.status ? 'Y' : 'N');
                write((char *)&packet.data, sizeof(packet.data));
                putchar('A'); //ack
                break;
            }
            default: {
                putchar('N'); //nack
                break;
            }
        }
    }


    return 0;
}
