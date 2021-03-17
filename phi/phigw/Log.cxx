#include "Log.h"
#include "json.hpp"
#include <stdio.h>

using json = nlohmann::json;

void Log::print(std::string message) {


	FILE *fp = fopen("/dev/ttyS0", "w");
	if (fp == NULL) {
		printf("Error opening serial port!\n");
		return;
	}

	json jMsg;
	jMsg["message"] = message;

	std::string msg = jMsg.dump() + "\n";

	/* Try to force input data on stdin */
	fwrite(msg.c_str(), sizeof(char), msg.length(), fp);
	// fwrite(message.c_str(), sizeof(char), message.length(), fp);

	fclose(fp);
}

