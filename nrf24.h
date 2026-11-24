#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "pinout.h"

typedef enum {
    NRF24_SENT,
    NRF24_MAX_RETRIES,
    NRF24_TIMED_OUT,
} nrf24_tx_result_t;

bool nrf24_init(void);
nrf24_tx_result_t nrf24_send(const uint8_t payload[NRF_PAYLOAD_SIZE]);
