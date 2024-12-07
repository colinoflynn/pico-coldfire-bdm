#include <stdio.h>
#include "pico/stdlib.h"

#define PIN_LED PICO_DEFAULT_LED_PIN

#define PIN_RESET 7
// Two inputs to the Coldfire:
#define PIN_DSCLK 2
#define PIN_DSI 2

// Trigger a breakpoint by pulling this low:
// Low = 1, High = 0
#define PIN_BKPT 4

// One output from the Coldfire, which is visible a couple CPU clock cycles
// after the fall of DSCLK. In practice, this means we only need to wait ~30
// nanoseconds between fall of DSCLK and seeing the correct value on DSO.
#define PIN_DSO 1


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
  gpio_put(PIN_DSCLK, 1);
  gpio_put(PIN_DSCLK, 0);
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

    puts("Motorola Coldfire Debug Interface by Peter Sobot\n");

    gpio_init(PIN_DSCLK);
    gpio_set_dir(PIN_DSCLK, GPIO_OUT);

    gpio_init(PIN_DSI);
    gpio_set_dir(PIN_DSI, GPIO_OUT);

    gpio_init(PIN_DSO);
    gpio_set_dir(PIN_DSO, GPIO_IN);

    puts("Ready.\n");

    while(1){
        int command = getchar();

        switch (command) {
            case 'P': // for Ping
                puts("PONG");
                break;
            case 'B': // for Breakpoint
                enterDebugMode(false);
                break;
            case 'R': // for Reset
                enterDebugMode(true);
                break;
            case 'S': { // for Send-and-Receive
                uint16_t data = getNextTwoBytesFromUSB();
                Packet packet = sendAndReceivePacket(data);
                putchar(packet.status ? 'Y' : 'N');
                write((char *)&packet.data, sizeof(packet.data));
                break;
            }
            case 's': { // for Send
                sendPacket(getNextTwoBytesFromUSB());
                break;
            }
            case 'r': { // for Receive
                Packet packet = receivePacket();
                putchar(packet.status ? 'Y' : 'N');
                write((char *)&packet.data, sizeof(packet.data));
                break;
            }
        }
    }


    return 0;
}
