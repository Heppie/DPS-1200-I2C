#pragma once
#include "hpdps.h"

void webserver_init(DpsSensors *sensors, bool *powerOn, uint8_t *fanPct);
void webserver_start();
void webserver_handle();
