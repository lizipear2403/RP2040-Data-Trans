#include "nrf24.h"

#include "hardware/spi.h"
#include "pico/stdlib.h"

enum {
    REG_CONFIG = 0x00, REG_EN_AA = 0x01, REG_EN_RXADDR = 0x02,
    REG_SETUP_AW = 0x03, REG_SETUP_RETR = 0x04, REG_RF_CH = 0x05,
    REG_RF_SETUP = 0x06, REG_STATUS = 0x07, REG_RX_ADDR_P0 = 0x0A,
    REG_RX_ADDR_P1 = 0x0B, REG_TX_ADDR = 0x10, REG_RX_PW_P0 = 0x11,
    REG_DYNPD = 0x1C, REG_FEATURE = 0x1D,
    CMD_WRITE_REG = 0x20, CMD_TX_PAYLOAD = 0xA0,
    CMD_FLUSH_TX = 0xE1, CMD_FLUSH_RX = 0xE2, CMD_NOP = 0xFF,
    CONFIG_EN_CRC = 1u << 3, CONFIG_PWR_UP = 1u << 1,
    STATUS_RX_DR = 1u << 6, STATUS_TX_DS = 1u << 5, STATUS_MAX_RT = 1u << 4,
    PIPE0 = 1u << 0, ADDR_WIDTH_5_BYTES = 0x03, RF_2MBPS_0DBM = 0x0E,
};

static uint8_t command(uint8_t opcode, const uint8_t *data, size_t size)
{
    uint8_t status;
    gpio_put(NRF_CSN, 0);
    spi_write_read_blocking(NRF_SPI, &opcode, &status, 1);
    if (size) {
        spi_write_blocking(NRF_SPI, data, size);
    }
    gpio_put(NRF_CSN, 1);
    return status;
}

static void write_reg(uint8_t reg, uint8_t value)
{
    command(CMD_WRITE_REG | reg, &value, sizeof(value));
}

static uint8_t read_reg(uint8_t reg)
{
    uint8_t value;
    gpio_put(NRF_CSN, 0);
    spi_write_blocking(NRF_SPI, &reg, 1);
    spi_read_blocking(NRF_SPI, CMD_NOP, &value, 1);
    gpio_put(NRF_CSN, 1);
    return value;
}

bool nrf24_init(void)
{
    static const uint8_t tx_addr[] = NRF_TADDR;
    static const uint8_t rx_addr[] = NRF_RADDR;
    _Static_assert(sizeof(tx_addr) == NRF_ADDR_SIZE + 1 && sizeof(rx_addr) == NRF_ADDR_SIZE + 1,
                   "nRF24 addresses must have five bytes");
    gpio_init(NRF_CE);
    gpio_put(NRF_CE, 0);
    gpio_set_dir(NRF_CE, GPIO_OUT);
    gpio_init(NRF_CSN);
    gpio_put(NRF_CSN, 1);
    gpio_set_dir(NRF_CSN, GPIO_OUT);
    spi_init(NRF_SPI, NRF_SPI_HZ);
    spi_set_format(NRF_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(NRF_SCK, GPIO_FUNC_SPI);
    gpio_set_function(NRF_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(NRF_MISO, GPIO_FUNC_SPI);
    sleep_ms(NRF_POWER_ON_WAIT_MS);

    write_reg(REG_CONFIG, CONFIG_EN_CRC);
    write_reg(REG_EN_AA, PIPE0);
    write_reg(REG_EN_RXADDR, PIPE0);
    write_reg(REG_SETUP_AW, ADDR_WIDTH_5_BYTES);
    write_reg(REG_SETUP_RETR, NRF_RETRY_SETUP);
    write_reg(REG_RF_CH, NRF_RF_CHANNEL);
    write_reg(REG_RF_SETUP, RF_2MBPS_0DBM);
    write_reg(REG_RX_PW_P0, NRF_PAYLOAD_SIZE);
    write_reg(REG_DYNPD, 0);
    write_reg(REG_FEATURE, 0);
    command(CMD_WRITE_REG | REG_TX_ADDR, tx_addr, NRF_ADDR_SIZE);
    command(CMD_WRITE_REG | REG_RX_ADDR_P0, tx_addr, NRF_ADDR_SIZE);
    command(CMD_WRITE_REG | REG_RX_ADDR_P1, rx_addr, NRF_ADDR_SIZE);
    command(CMD_FLUSH_TX, NULL, 0);
    command(CMD_FLUSH_RX, NULL, 0);
    write_reg(REG_STATUS, STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT);
    write_reg(REG_CONFIG, CONFIG_EN_CRC | CONFIG_PWR_UP);
    sleep_us(NRF_POWER_UP_WAIT_US);

    return read_reg(REG_CONFIG) == (CONFIG_EN_CRC | CONFIG_PWR_UP)
        && read_reg(REG_RF_CH) == NRF_RF_CHANNEL
        && read_reg(REG_SETUP_AW) == ADDR_WIDTH_5_BYTES;
}

nrf24_tx_result_t nrf24_send(const uint8_t payload[NRF_PAYLOAD_SIZE])
{
    command(CMD_TX_PAYLOAD, payload, NRF_PAYLOAD_SIZE);
    gpio_put(NRF_CE, 1);
    sleep_us(NRF_CE_PULSE_US);
    gpio_put(NRF_CE, 0);

    const absolute_time_t deadline = make_timeout_time_us(NRF_TX_WAIT_US);
    uint8_t status;
    do {
        status = command(CMD_NOP, NULL, 0);
        if (status & (STATUS_TX_DS | STATUS_MAX_RT)) {
            break;
        }
    } while (!time_reached(deadline));

    write_reg(REG_STATUS, STATUS_TX_DS | STATUS_MAX_RT);
    if ((status & (STATUS_TX_DS | STATUS_MAX_RT)) == STATUS_TX_DS) {
        return NRF24_SENT;
    }
    command(CMD_FLUSH_TX, NULL, 0);
    return (status & STATUS_MAX_RT) ? NRF24_MAX_RETRIES : NRF24_TIMED_OUT;
}
