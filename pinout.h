#pragma once

#define LINK_SPI                 spi0
#define LINK_SCK                 2
#define LINK_MOSI                3
#define LINK_MISO                4
#define LINK_CS                  5
#define LINK_READY               6
#define LINK_SPI_HZ              20000000u
#define LINK_BUF_SIZE            (32u * 1024u)
#define LINK_HEADER_SIZE         4u
#define LINK_READY_WAIT_MS       1000u
#define LINK_RELEASE_WAIT_MS     50u

#define NRF_SPI                  spi1
#define NRF_SCK                  10
#define NRF_MOSI                 11
#define NRF_MISO                 12
#define NRF_CSN                  13
#define NRF_CE                   14
#define NRF_IRQ                  15
#define NRF_SPI_HZ               4000000u
#define NRF_RF_CHANNEL           76u
#define NRF_PAYLOAD_SIZE         32u
#define NRF_ADDR_SIZE            5u
#define NRF_TADDR                "CTROL"
#define NRF_RADDR                "CAMRA"
#define NRF_RETRY_SETUP          0x5Fu
#define NRF_TX_WAIT_US           40000u
#define NRF_CE_PULSE_US          15u
#define NRF_POWER_ON_WAIT_MS     100u
#define NRF_POWER_UP_WAIT_US     1500u

#define FRAME_HEADER_SIZE        4u
#define FRAME_FRAGMENT_SIZE      (NRF_PAYLOAD_SIZE - FRAME_HEADER_SIZE)
#define FRAME_MAX_PACKETS        255u
#define FRAME_MAX_SIZE           (FRAME_FRAGMENT_SIZE * FRAME_MAX_PACKETS)
#define FRAME_SUB_INDEX_BYTE     2u
#define FRAME_COUNT_BYTE         3u
