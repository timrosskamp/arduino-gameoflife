#include <Arduino.h>

uint32_t seed;

uint8_t matrix[8];
uint8_t diff[8];

#define CACHE_SIZE 500
uint16_t cache[500];
uint8_t index = 0;

uint8_t bitcount(uint8_t n) {
   uint8_t count = 0;
   while( n ){
      count++;
      n &= (n - 1);
   }
   return count;
}

uint16_t crc16(uint8_t* data_p, uint8_t length){
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }

    return crc;
}

uint8_t wrap(int8_t i, uint8_t limit) {
    return (i + limit) % limit;
}

uint32_t generateSeed() {
    return analogRead(A0) * analogRead(A1);
}

void start() {
    // reset index
    index = 0;

    // generate & save seed
    seed = generateSeed();
    randomSeed(seed);

    // fill the matrix with random values
    for( int8_t r = 0; r < 8; r++ ) {
        matrix[r] = random(0xFF);
    }

    // save the initial hash
    cache[0] = crc16(matrix, 8);

    Serial.println(cache[0]);
}

void iterate() {
    for( int8_t r = 0; r < 8; r++ ) {
        for( int8_t c = 0; c < 8; c++ ) {
            bool state = (matrix[r] >> (7 - c)) & 0x01;
            uint8_t n = 0;

            // count neighboring living cells
            for( int8_t y = -1; y <= 1; y++ ){
                uint8_t row = matrix[wrap(r + y, 8)];
                uint8_t mask = y == 0 ? 0b01000001 : 0b11000001;
                mask = (mask >> c) | (mask << (8 - c));
                n += bitcount(row & mask);
            }

            // living cells without 2 or 3 living neighbors die
            // dead cells with 3 neighbors turn alive
            if( (state == 1 && n != 2 && n != 3) || (state == 0 && n == 3) ){
                diff[r] |= (1 << (7 - c));
            }
        }
    }

    // apply and clear diff
    for( uint8_t i = 0; i < 8; i++ ) {
        matrix[i] ^= diff[i];
        diff[i] = 0;
    }
}

void print() {
    Serial.println("------------------");

    for( int8_t r = 0; r < 8; r++ ) {
        Serial.print("|");
        for( int8_t c = 0; c < 8; c++ ) {
            bool state = (matrix[r] >> (7 - c)) & 0x01;

            Serial.print(state ? "▇▇" : "  ");
        }
        Serial.print("|");
        Serial.println();
    }

    Serial.println("------------------");
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    while( !Serial );

    start();
    print();
}

void loop() {
    iterate();
    print();

    if( index >= (CACHE_SIZE - 1) ){
        // no more memory available
        Serial.println("Out of memory. Restarting...");
        Serial.println();

        delay(2000);
        start();
        print();
    }else{
        index++;
        uint16_t chash = crc16(matrix, 8);
        cache[index] = chash;

        Serial.println(chash);

        if( chash == 0 ){
            Serial.println("You died. Restarting...");
            Serial.println();

            delay(2000);
            start();
            print();
        }else{
            uint16_t r = 0;
            for( int16_t i = (index - 1); i >= 0; i-- ){
                if( cache[i] == chash ){
                    if( r == 0 ){
                        Serial.print("Game is stuck. Restarting...");
                        Serial.println();

                        delay(2000);
                        start();
                        print();
                        break;
                    }else{
                        Serial.print("Recursion detected (");
                        Serial.print(chash);
                        Serial.print(") at ");
                        Serial.print(r);
                        Serial.println(" iterations. Restarting...");
                        Serial.println();

                        delay(2000);
                        start();
                        print();
                        break;
                    }
                }
                r++;
            }
        }
    }

    delay(250);
}