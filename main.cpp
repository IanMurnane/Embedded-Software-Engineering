#include <Adafruit_NeoPixel.h>

#define PIN 2  // input pin Neopixel is attached to
#define NUMPIXELS 44  // total LEDs in all pixels
#define NUMVEHICLES 2  // number of vehicles, also update the vehicle array

#define OFF pixels.Color(0, 0, 0)
#define RED pixels.Color(255, 0, 0)
#define GREEN pixels.Color(0, 255, 0)
#define BLUE pixels.Color(0, 0, 255)
#define YELLOW pixels.Color(255, 255, 0)

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// unsigned long timerA = 0, timerB = 0;

class Cloud {
private:
    int streets[NUMVEHICLES];
    int addresses[NUMVEHICLES];
public:
    void update(int id, int street, int address) {
        streets[id] = street;
        addresses[id] = address;
    }
    int getLocationFromId(int id) {
        return getLocation(streets[id], addresses[id]);
    }
    bool isFree(int id, int location) {
        for (int i = 0; i < NUMVEHICLES; i++) {
            if (i == id) continue;
            if (getLocationFromId(i) == location) return false;
        }
        return true;
    }
    int getStreet(int id) {
        return streets[id];
    }
    int getAddress(int id) {
        return addresses[id];
    }
};

class Vehicle {
private:
    Cloud& cloud;
    int direction;  // always 1 (forward) - can be removed

    bool randomBool() {
        return random(100) < 25;  // 25% chance of true
    }

public:
    int id;
    int street;  // 0-7
    int address;  // 0-9

    Vehicle(int id, Cloud& cloud, int street, int address, int direction) :
            id(id),
            cloud(cloud),
            street(street),
            address(address),
            direction(direction){};

    void move() {
        int origStreet = street;
        int origAddress = address;
        int location = getLocation(street, address);

        // turn down side-streets
        if (street == 0 && randomBool()) {
            switch(address) {
                case 4:
                    street = 1;
                    address = -1;
                    break;
            }
        }
        if (street == 4 && randomBool()) {
            switch(address) {
                case 1:
                    street = 2;
                    address = -1;
                    break;
                case 7:
                    street = 3;
                    address = -1;
                    break;
            }
        }

        // u-turn at the end of each road
        switch(location) {
            case 9:  // street 0
                street = 4;
                address = -1;
                break;
            case 13:  // street 1
                street = 5;
                address = -1;
                break;
            case 17:  // street 2
                street = 6;
                address = -1;
                break;
            case 21:  // street 3
                street = 7;
                address = -1;
                break;
            case 31:  // street 4
                street = 0;
                address = -1;
                break;
            case 35:  // street 5
                street = 0;
                address = 4;
                break;
            case 39:  // street 6
                street = 4;
                address = 1;
                break;
            case 43:  // street 7
                street = 4;
                address = 7;
                break;
        }

        address += direction;
        // ensure the new location is free
        int newLocation = getLocation(street, address);
        if (!cloud.isFree(id, newLocation)) {
            street = origStreet;
            address = origAddress;
        }
        else cloud.update(id, street, address);
    }

    int location() {
        return getLocation(street, address);
    }
};

Cloud cloud;
Vehicle vehicles[] = {
        Vehicle(0, cloud, 0, 0, 1),
        Vehicle(1, cloud, 4, 0, 1)
};

void setup() {
    randomSeed(analogRead(0));
    pixels.begin();
    Serial.begin(9600);
}

/*
void startTimer() {
  timerA = micros();
}
void stopTimer() {
  int currentTimer = micros() - timerA;
  if (currentTimer > timerB) {
    Serial.println(currentTimer);
    timerB = currentTimer;
  }
}
*/

uint32_t model[NUMPIXELS] = {};
void loop() {
    // clear model
    for (auto& i : model) i = OFF;

    // add lighting
    int lights[] = { -1, 1, 2 };  // 1 behind, 2 in front
    for (int i = 0; i < NUMVEHICLES; i++) {
        int address = cloud.getAddress(i);
        int street = cloud.getStreet(i);

        // street lighting
        for (int n : lights) {
            int addressN = address + n;
            // check light address is valid
            if (addressN < 0) continue;
            if (street == 0 || street == 4) {
                if (addressN > 9) continue;
            }
            else if (addressN > 3) {
                continue;
            }

            model[getLocation(street, addressN)] = YELLOW;
        }

        // intersection lighting
        if (street == 0) {
            if (address == 3 || address == 4) {
                model[getLocation(1, 0)] = YELLOW;
                model[getLocation(1, 1)] = YELLOW;
            }
        }
        else if (street == 4) {
            if (address == 0 || address == 1) {
                model[getLocation(2, 0)] = YELLOW;
                model[getLocation(2, 1)] = YELLOW;
            }
            if (address == 6 || address == 7) {
                model[getLocation(3, 0)] = YELLOW;
                model[getLocation(3, 1)] = YELLOW;
            }
        }
        else if (street == 5 && address > 1) {
            model[getLocation(0, 4)] = YELLOW;
            model[getLocation(0, 5)] = YELLOW;
            model[getLocation(0, 6)] = YELLOW;
        }
        else if (street == 6 && address > 1) {
            model[getLocation(4, 1)] = YELLOW;
            model[getLocation(4, 2)] = YELLOW;
            model[getLocation(4, 3)] = YELLOW;
        }
        else if (street == 7 && address > 1) {
            model[getLocation(4, 7)] = YELLOW;
            model[getLocation(4, 8)] = YELLOW;
            model[getLocation(4, 9)] = YELLOW;
        }
    }

    // add vehicles
    for (auto& vehicle : vehicles) {
        model[vehicle.location()] = vehicle.id == 0 ? BLUE : RED;
    }

    // output to LEDs
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, model[i]);
    }
    pixels.show();

    // move vehicles
    for (auto& vehicle : vehicles) {
        vehicle.move();
    }

    delay(350);
}

// convert street and address to LED location
int getLocation(int street, int address) {
    int location = address;
    if (street > 0) {
        location += 10;
        location += (street - 1) * 4;
    }
    if (street > 4) {
        location += 10;
        location -= 4;
    }
    return location;
}
