#include "config.hpp"

#ifdef CONFIG_IOT_ENABLE_UDP

#include <cstring>
#include <esp_crc.h>
#include <lwip/dns.h>
#include <netdb.h>

#include "utils.hpp"
#include "udp.hpp"

esp_err_t UDP::init()
{
  esp_log_level_set(TAG, CONFIG_IOT_LOG_LEVEL);

  esp_err_t status = ESP_OK;

  hostent * h = gethostbyname(CONFIG_IOT_GATEWAY_ADDRESS);
  in_addr addr;

  int i = 0;
  addr.s_addr = 0;

  while (h->h_addr_list[i] != 0) {
    addr.s_addr = *(u_long *) h->h_addr_list[i++];
    ESP_LOGD(TAG, "\tIPv4 Address #%d: %s\n", i, inet_ntoa(addr));
  }

  if (addr.s_addr == 0) {
    ESP_LOGE(TAG, "Unable to retrieve IP address of %s.", CONFIG_IOT_GATEWAY_ADDRESS);
  }

  dest_addr.sin_addr.s_addr = addr.s_addr;
  dest_addr.sin_family      = AF_INET;
  dest_addr.sin_port        = htons(CONFIG_IOT_UDP_PORT);
  
  int addr_family           = AF_INET;
  int ip_protocol           = IPPROTO_IP;

  sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
  if (sock < 0) {
    ESP_LOGE(TAG, "Unable to create socket: %s", strerror(errno));
    status = ESP_FAIL;
  }

  return status;
}

esp_err_t UDP::send(const uint8_t * data, int len)
{
  esp_err_t status = ESP_OK;

  static struct {
    uint16_t crc;
    char data[CONFIG_IOT_UDP_MAX_PKT_SIZE];
  } __attribute__((packed)) pkt;


  if (len > CONFIG_IOT_UDP_MAX_PKT_SIZE) {
    ESP_LOGE(TAG, "Cannot send data of length %d, too long. Max is %d.", len, CONFIG_IOT_UDP_MAX_PKT_SIZE);
    status = ESP_FAIL;
  }
  else {
    memcpy(pkt.data, data, len);
    pkt.crc = esp_crc16_le(UINT16_MAX, (const uint8_t *)(pkt.data), len);

    int err = sendto(sock, (const uint8_t *) &pkt, len + 2, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        status = ESP_FAIL;
    }
    else {
      ESP_LOGD(TAG, "The following message was sent:");
      dump_data(TAG, data, len);
    }
  }

  return status;
}

void UDP::prepare_for_deep_sleep()
{
  if (sock != -1) {
    ESP_LOGE(TAG, "Shutting down socket and restarting...");
    shutdown(sock, 0);
    close(sock);
  }
}

#endif