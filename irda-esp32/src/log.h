#pragma once

#define ANSI_RED "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_RESET "\033[0m"

#define LOGE(tag, format, ...) Serial.printf(ANSI_RED "E[%s] " format ANSI_RESET "\n", tag, ##__VA_ARGS__)
#define LOGW(tag, format, ...) Serial.printf(ANSI_YELLOW "W[%s] " format ANSI_RESET "\n", tag, ##__VA_ARGS__)

#define LOGI(tag, format, ...) Serial.printf("I[%s] " format "\n", tag, ##__VA_ARGS__)
#define LOGD(tag, format, ...) Serial.printf("D[%s] " format "\n", tag, ##__VA_ARGS__)
#define LOGV(tag, format, ...) Serial.printf("V[%s] " format "\n", tag, ##__VA_ARGS__)
