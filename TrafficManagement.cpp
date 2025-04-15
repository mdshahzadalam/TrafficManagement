// File: TrafficSimulation.cpp

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <iomanip>
#include <algorithm> // needed for std::find

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

enum class LightState { RED, YELLOW, GREEN };
enum class VehicleType { CAR, BUS, TRUCK, MOTORCYCLE };

class Lane;
class TrafficLight;

class Vehicle {
private:
    int id;
    VehicleType type;
    double speed;
    double position;
    Lane* currentLane;

public:
    Vehicle(int id, VehicleType type, double speed)
        : id(id), type(type), speed(speed), position(0.0), currentLane(nullptr) {}

    void move(double timeStep);

    int getId() const { return id; }
    double getSpeed() const { return speed; }
    double getPosition() const { return position; }
    VehicleType getType() const { return type; }
    Lane* getCurrentLane() const { return currentLane; }

    void setSpeed(double s) { speed = s; }
    void setPosition(double p) { position = p; }
    void setCurrentLane(Lane* lane) { currentLane = lane; }

    std::string getSymbol() const {
        switch (type) {
            case VehicleType::CAR: return "C";
            case VehicleType::BUS: return "B";
            case VehicleType::TRUCK: return "T";
            case VehicleType::MOTORCYCLE: return "M";
            default: return "?";
        }
    }
};

class TrafficLight {
private:
    LightState state;
    int greenTime, yellowTime, redTime;
    int timer;

public:
    TrafficLight(int g, int y, int r)
        : state(LightState::RED), greenTime(g), yellowTime(y), redTime(r), timer(0) {}

    void update(int step) {
        timer += step;
        switch (state) {
            case LightState::GREEN:
                if (timer >= greenTime) {
                    state = LightState::YELLOW;
                    timer = 0;
                }
                break;
            case LightState::YELLOW:
                if (timer >= yellowTime) {
                    state = LightState::RED;
                    timer = 0;
                }
                break;
            case LightState::RED:
                if (timer >= redTime) {
                    state = LightState::GREEN;
                    timer = 0;
                }
                break;
        }
    }

    LightState getState() const { return state; }
    std::string getSymbol() const {
        switch (state) {
            case LightState::GREEN: return "🟢";
            case LightState::YELLOW: return "🟡";
            case LightState::RED: return "🔴";
        }
        return "?";
    }
};

class Lane {
private:
    int id;
    double length;
    double speedLimit;
    std::vector<Vehicle*> vehicles;
    TrafficLight* light;

public:
    Lane(int id, double len, double limit)
        : id(id), length(len), speedLimit(limit), light(new TrafficLight(10, 3, 7)) {}

    ~Lane() { delete light; }

    void addVehicle(Vehicle* v) {
        vehicles.push_back(v);
        v->setCurrentLane(this);
    }

    void update(int timeStep) {
        light->update(timeStep);
        for (auto it = vehicles.begin(); it != vehicles.end();) {
            (*it)->move(timeStep);
            if ((*it)->getPosition() >= length) {
                it = vehicles.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::vector<Vehicle*>& getVehicles() { return vehicles; }
    TrafficLight* getLight() const { return light; }
    double getLength() const { return length; }
    int getId() const { return id; }
};

void Vehicle::move(double timeStep) {
    if (!currentLane) return;

    double mps = speed * 1000.0 / 3600.0;
    double nextPos = position + mps * timeStep;

    TrafficLight* light = currentLane->getLight();
    if (light && light->getState() != LightState::GREEN &&
        position < currentLane->getLength() - 10 &&
        nextPos >= currentLane->getLength() - 10) {
        speed = 0;
        return;
    }

    position = nextPos;
}

class Simulation {
private:
    std::vector<Lane*> lanes;
    std::vector<Vehicle*> vehicles;
    std::mt19937 rng;
    int nextId = 1;
    int simTime = 0;

public:
    Simulation() {
        std::random_device rd;
        rng.seed(rd());
    }

    ~Simulation() {
        for (auto v : vehicles) delete v;
        for (auto l : lanes) delete l;
    }

    void setup() {
        lanes.push_back(new Lane(1, 500.0, 50.0));
        lanes.push_back(new Lane(2, 600.0, 40.0));
    }

    void generateVehicle() {
        std::uniform_int_distribution<> typeDist(0, 3);
        std::uniform_real_distribution<> speedDist(20, 50);
        std::uniform_int_distribution<> laneDist(0, lanes.size() - 1);

        VehicleType type = static_cast<VehicleType>(typeDist(rng));
        double speed = speedDist(rng);
        int laneIndex = laneDist(rng);

        Vehicle* v = new Vehicle(nextId++, type, speed);
        lanes[laneIndex]->addVehicle(v);
        vehicles.push_back(v);
    }

    void update(int step) {
        simTime += step;
        if (std::uniform_real_distribution<>(0, 1)(rng) < 0.3) {
            generateVehicle();
        }
        for (auto lane : lanes) {
            lane->update(step);
        }
    }

    void display() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        std::cout << "Simulation Time: " << simTime << "s\n";

        for (auto lane : lanes) {
            std::cout << "Lane " << lane->getId() << " [Light: " << lane->getLight()->getSymbol() << "]\n  ";
            std::string road(50, '-');
            for (auto v : lane->getVehicles()) {
                int pos = static_cast<int>((v->getPosition() / lane->getLength()) * road.size());
                if (pos >= 0 && pos < 50)
                    road[pos] = v->getSymbol()[0];
            }
            std::cout << road << "\n\n";
        }
    }

    bool isRunning() const { return simTime < 60; } // 1-minute sim
};

int main() {
    Simulation sim;
    sim.setup();
    while (sim.isRunning()) {
        sim.update(1);
        sim.display();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "Simulation ended.\n";
    return 0;
}
