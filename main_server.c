#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include <sys/socket.h>

#define DHTPIN 7  
#define MAX 85

int dht_val[5] = {0, 0, 0, 0, 0};


int read_dht22_data() {
    uint8_t last_state = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    for (i = 0; i < 5; i++) {
        dht_val[i] = 0;
    }

    
    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(30);
    pinMode(DHTPIN, INPUT);

    
    for (i = 0; i < MAX; i++) {
        counter = 0;
        while (digitalRead(DHTPIN) == last_state) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        last_state = digitalRead(DHTPIN);

        if (counter == 255) {
            break;
        }

        
        if ((i >= 4) && (i % 2 == 0)) {
            dht_val[j / 8] <<= 1;
            if (counter > 30) {
                dht_val[j / 8] |= 1;
            }
            j++;
        }
    }

    
    if ((j >= 40) && (dht_val[4] == ((dht_val[0] + dht_val[1] + dht_val[2] + dht_val[3]) & 0xFF))) {
        return 1;  
    } else {
        return 0;  
    }
}


void print_dht22_data() {
    float humidity = ((dht_val[0] << 8) + dht_val[1]) / 10.0;
    float celsius = ((dht_val[2] << 8) + dht_val[3]) / 10.0;
    float fahrenheit = celsius * 9.0 / 5.0 + 32;

    printf("현재 연구실 습도 = %.1f %% 현재 연구실 온도 = %.1f *C (%.1f *F)\n", humidity, celsius, fahrenheit);
}

int main(void) {

    if (wiringPiSetup() == -1) {
        printf("WiringPi setup error!\n");
        return -1;
    }

    printf("Starting DHT22 sensor data read...\n");


    while (1) {
        if (read_dht22_data()) {
            print_dht22_data();
        } else {
            printf("Failed to read DHT22 data\n");
        }
        sleep(5);  
    }

    return 0;
}
