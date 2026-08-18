#pragma once
#include <thread>

void serverScope(std::stop_token);
void serverTakedown();