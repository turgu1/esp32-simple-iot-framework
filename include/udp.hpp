#pragma once

#include "config.hpp"

#ifdef CONFIG_IOT_ENABLE_UDP

#define __UDP__
#include "global.hpp"
#undef __UDP__

#include <lwip/sockets.h>

class UDP
{
  private:
    static constexpr char const * TAG = "UDP Class";

    int                sock;
    struct sockaddr_in dest_addr;

  public:
    esp_err_t                   init();
    esp_err_t                   send(const uint8_t * data, int len);
    void      prepare_for_deep_sleep();
};

#endif