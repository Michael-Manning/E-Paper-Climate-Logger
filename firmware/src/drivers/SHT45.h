#pragma once

constexpr uint8_t SHT45_Addr = 0x44;

enum class TemperatureFormat : uint8_t{
    Celcius = 0,
    Fahrenheit = 1
};

struct ClimateSample{

    // fixed decimal .00
    int16_t temperature_raw = 0;
    int16_t humidity_raw = 0;

    float Temperature_c() const {
        return (float)temperature_raw / 100.0f;
    };
    float Temperature_f() const {
        return Temperature_c() * (9.0f / 5.0f) + 32.0f;
    };
    float Humidity_r() const {
        return (float)humidity_raw / 100.0f;
    };
};

bool getClimate(ClimateSample& sample);