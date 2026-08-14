#pragma once
#include "hpdps.h"

void webserver_init(DpsSensors *sensors, bool *powerOn, uint8_t *fanPct,
                    const char *model, const char *part_num);
void webserver_start();
void webserver_handle();
