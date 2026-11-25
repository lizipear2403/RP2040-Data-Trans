/* S3: 32768 B 单次 SPI 事务，4 B 小端长度。P4: 32 B FramePkt，尾包补零。 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <RF24.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pinout.h"

static uint8_t link_buf[LINK_BUF_SIZE];
static SPI radio_spi;
static RF24 radio(NRF_CE, NRF_CSN, NRF_SPI_HZ);

static bool radio_init(void)
{
    static const uint8_t transmit_address[] = NRF_TADDR;
    static const uint8_t receive_address[] = NRF_RADDR;
    static_assert(sizeof(transmit_address) == NRF_ADDR_SIZE + 1 &&
                  sizeof(receive_address) == NRF_ADDR_SIZE + 1);

    radio_spi.begin(NRF_SPI, NRF_SCK, NRF_MOSI, NRF_MISO);
    if (!radio.begin(&radio_spi) || !radio.setDataRate(RF24_2MBPS)) {
        return false;
    }
    radio.setAddressWidth(NRF_ADDR_SIZE);
    radio.setChannel(NRF_RF_CHANNEL);
    radio.setPALevel(RF24_PA_MAX);
    radio.setCRCLength(RF24_CRC_8);
    radio.setPayloadSize(NRF_PAYLOAD_SIZE);
    radio.disableDynamicPayloads();
    radio.setAutoAck(false);
    radio.setAutoAck(0, true);
    radio.setRetries(NRF_RETRY_DELAY, NRF_RETRY_COUNT);
    radio.openReadingPipe(1, receive_address);
    radio.closeReadingPipe(1);
    radio.stopListening(transmit_address);
    return true;
}

static void link_init(void)
{
    gpio_init(LINK_CS);
    gpio_put(LINK_CS, 1);
    gpio_set_dir(LINK_CS, GPIO_OUT);
    gpio_init(LINK_READY);
    gpio_set_dir(LINK_READY, GPIO_IN);
    gpio_pull_down(LINK_READY);
    spi_init(LINK_SPI, LINK_SPI_HZ);
    spi_set_format(LINK_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(LINK_SCK, GPIO_FUNC_SPI);
    gpio_set_function(LINK_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(LINK_MISO, GPIO_FUNC_SPI);
}

static bool wait_ready(bool high, uint32_t timeout_ms)
{
    const absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    while (gpio_get(LINK_READY) != high) {
        if (time_reached(deadline)) {
            return false;
        }
        tight_loop_contents();
    }
    return true;
}

static void send_frame(uint16_t frame_id, bool radio_ready)
{
    const uint32_t len = (uint32_t)link_buf[0] | ((uint32_t)link_buf[1] << 8)
        | ((uint32_t)link_buf[2] << 16) | ((uint32_t)link_buf[3] << 24);
    const uint8_t *jpeg = link_buf + LINK_HEADER_SIZE;
    if (len < 4 || len > LINK_BUF_SIZE - LINK_HEADER_SIZE) {
        printf("frame=%u invalid SPI length=%lu\n", (unsigned)frame_id, (unsigned long)len);
        return;
    }
    if (len > FRAME_MAX_SIZE) {
        printf("frame=%u JPEG length=%lu exceeds %u, dropped\n",
               (unsigned)frame_id, (unsigned long)len, FRAME_MAX_SIZE);
        return;
    }
    if (jpeg[0] != 0xff || jpeg[1] != 0xd8 || jpeg[len - 2] != 0xff || jpeg[len - 1] != 0xd9) {
        printf("frame=%u length=%lu invalid JPEG markers\n",
               (unsigned)frame_id, (unsigned long)len);
        return;
    }
    const unsigned count = (len + FRAME_FRAGMENT_SIZE - 1) / FRAME_FRAGMENT_SIZE;
    if (!radio_ready) {
        printf("frame=%u SPI JPEG length=%lu packets=%u; radio unavailable\n",
               (unsigned)frame_id, (unsigned long)len, count);
        return;
    }
    for (unsigned i = 0; i < count; ++i) {
        uint8_t pkt[NRF_PAYLOAD_SIZE] = {
            (uint8_t)frame_id, (uint8_t)(frame_id >> 8), (uint8_t)i, (uint8_t)count,
        };
        const size_t offset = i * FRAME_FRAGMENT_SIZE;
        const size_t remaining = len - offset;
        const size_t size = remaining < FRAME_FRAGMENT_SIZE ? remaining : FRAME_FRAGMENT_SIZE;
        memcpy(pkt + FRAME_HEADER_SIZE, jpeg + offset, size);
        // RF24 单独通过 failureDetected 报告硬件超时。
        radio.failureDetected = 0;
        const bool acked = radio.write(pkt, sizeof(pkt));
        if (!acked || radio.failureDetected) {
            printf("frame=%u length=%lu packet=%u/%u dropped: %s\n",
                   (unsigned)frame_id, (unsigned long)len, i + 1, count,
                   radio.failureDetected ? "radio timeout" : "no ACK");
            return;
        }
    }
    printf("frame=%u length=%lu packets=%u ACKed\n",
           (unsigned)frame_id, (unsigned long)len, count);
}

int main(void)
{
    stdio_init_all();
    link_init();
    const bool radio_ready = radio_init();
    printf("nRF24 %s, channel=%u\n", radio_ready ? "ready" : "register check failed", NRF_RF_CHANNEL);

    uint16_t frame_id = 0;
    while (true) {
        if (!wait_ready(true, LINK_READY_WAIT_MS)) {
            puts("S3 READY low: waiting for JPEG");
            continue;
        }
        gpio_put(LINK_CS, 0);
        const int received = spi_read_blocking(LINK_SPI, 0, link_buf, sizeof(link_buf));
        gpio_put(LINK_CS, 1);

        // 先观察事务结束的低电平，避免发无线包期间错过它。
        bool released = wait_ready(false, LINK_RELEASE_WAIT_MS);
        if (!released) {
            puts("S3 READY stuck high after SPI transaction");
            while (!wait_ready(false, LINK_READY_WAIT_MS)) {
                puts("S3 READY still high");
            }
        }
        if (released && received == (int)sizeof(link_buf)) {
            send_frame(frame_id, radio_ready);
        } else {
            printf("frame=%u SPI transaction dropped, bytes=%d\n", (unsigned)frame_id, received);
        }
        ++frame_id;
    }
}
