#include <Arduino.h>

uint32_t seed;

uint8_t matrix[8] = {
    0b00000000,
    0b00001000,
    0b00000100,
    0b00011100,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
};
uint8_t diff[8];

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

uint16_t hash() {
    uint16_t h = 0;

    for( uint8_t i = 0; i < 8; i++ ) {
        if( i % 2 == 0 ) h ^= matrix[i] << 8;
        else h ^= matrix[i];
    }

    return h;
}

uint8_t wrap(int8_t i, uint8_t limit) {
    return (i + limit) % limit;
}

uint32_t generateSeed() {
    return analogRead(A0) * analogRead(A1);
}

void setup() {
    Serial.begin(115200);
    while( !Serial );

    seed = generateSeed();
    randomSeed(seed);

    for( int8_t r = 0; r < 8; r++ ) {
        matrix[r] = random(0xFF);
    }

    cache[index] = hash();
}

void loop() {
    Serial.println();
    Serial.println(cache[index]);
    Serial.println("------------------");
    
    for( int8_t r = 0; r < 8; r++ ) {
        Serial.print("|");
        for( int8_t c = 0; c < 8; c++ ) {
            bool state = (matrix[r] >> (7 - c)) & 0x01;

            Serial.print(state ? "▇▇" : "  ");

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
        Serial.print("|");
        Serial.println();
    }

    Serial.println("------------------");

    // apply diff
    for( uint8_t i = 0; i < 8; i++ ) {
        matrix[i] ^= diff[i];
        diff[i] = 0;
    }

    if( index >= 499 ){
        // no more memory available
        Serial.println("Out of memory");
        while(1);
    }
    index++;
    uint16_t chash = hash();
    cache[index] = chash;

    if( cache[index] == 0 ){
        Serial.println("You died.");
        while(1);
    }

    uint16_t r = 0;
    for( int16_t i = (index - 1); i >= 0; i-- ){
        if( cache[i] == chash ){
            if( r == 0 ){
                Serial.print("Game is stuck.");
            }else{
                Serial.print("Recursion detected (");
                Serial.print(chash);
                Serial.print(") at ");
                Serial.print(r);
                Serial.println(" iterations");
            }
            while(1);
        }
        r++;
    }

    delay(500);
}