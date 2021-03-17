#pragma once

#include <nuttx/config.h>
#include <iostream>

class Log {
public:
	static void print(std::string message);
};

