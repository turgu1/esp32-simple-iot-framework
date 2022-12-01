#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <cctype>

#include "config.hpp"

void dump_data(const char *tag, const uint8_t * data, int len) 
{
  esp_log_level_t log_level = esp_log_level_get(tag);

  if ((log_level != ESP_LOG_DEBUG) && (log_level != ESP_LOG_VERBOSE)) return;

  int pos = 0;
  static char buff[80];

  while (pos < len) {
    sprintf(buff, "%03XH:", pos);
    int k = 5;
    for (int i = 0; i < 16; i++) {
      if ((pos + i) < len) {
        sprintf(&buff[k], " %02X", data[pos + i]);
      }
      else {
        strcpy(&buff[k], "   ");
      }
      if (i == 7) buff[k++] = ' ';
      k += 3;
    }
    strcpy(&buff[k], "  |");
    k += 3;
    for (int i = 0; i < 16; i++) {
      buff[k++] = ((pos + i) < len) 
                    ? (((data[pos + i] >= ' ') && (data[pos + i] <= '~')) 
                        ? data[pos + i] 
                        : '.') 
                    : ' ';
      if (i == 7) buff[k++] = ' ';
    }
    strcpy(&buff[k], "|");
    ESP_LOGD(tag, "%s", buff);

    pos += 16;
  }
}