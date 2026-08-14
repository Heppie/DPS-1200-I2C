#pragma once
#include "dps1200.h"

void webserver_init(DpsSensors *sensors, bool *powerOn, uint8_t *fanPct);
void webserver_start();
void webserver_handle();
