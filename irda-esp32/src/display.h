#pragma once
#include <Arduino.h>

namespace Display {

bool init();
void update();
Print& print();

}  // namespace Display