#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/slab.h>

#define DRIVER_NAME "my-spi-test"

struct my_spi_data {
    struct spi_device *spi;
};

/* ============================================================
 * PROBE
 * ============================================================*/
static int my_spi_probe(struct spi_device *spi)
{
    struct my_spi_data *data;
    int ret;
    u8 txbuf[1] = { 0x00 };
    u8 rxbuf[1] = { 0 };

    dev_info(&spi->dev,
             "Probing SPI test device (mode %u, max_speed %u)\n",
             spi->mode, spi->max_speed_hz);

    data = devm_kzalloc(&spi->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    spi_set_drvdata(spi, data);
    data->spi = spi;

    /* Configure SPI */
    spi->bits_per_word = 8;

    ret = spi_setup(spi);
    if (ret) {
        dev_err(&spi->dev, "spi_setup failed: %d\n", ret);
        return ret;
    }

    /* Simple test transfer */
    ret = spi_write_then_read(spi, txbuf, sizeof(txbuf),
                              rxbuf, sizeof(rxbuf));
    if (ret < 0) {
        dev_warn(&spi->dev,
                 "SPI transfer failed (device may be absent), err=%d\n",
                 ret);
    } else {
        dev_info(&spi->dev, "SPI test read returned 0x%02x\n", rxbuf[0]);
    }

    return 0;
}

/* ============================================================
 * REMOVE  (kernel 6.x uses void return type)
 * ============================================================*/
static void my_spi_remove(struct spi_device *spi)
{
    dev_info(&spi->dev, "SPI test device removed\n");
}

/* ============================================================
 * SPI ID TABLE
 * ============================================================*/
static const struct spi_device_id my_spi_id[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, my_spi_id);

/* ============================================================
 * DEVICE TREE MATCH
 * ============================================================*/
static const struct of_device_id my_spi_of_match[] = {
    { .compatible = "mycompany,my-spi-test" },
    { }
};
MODULE_DEVICE_TABLE(of, my_spi_of_match);

/* ============================================================
 * SPI DRIVER STRUCT
 * ============================================================*/
static struct spi_driver my_spi_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = my_spi_of_match,
    },
    .probe    = my_spi_probe,
    .remove   = my_spi_remove,
    .id_table = my_spi_id,
};

module_spi_driver(my_spi_driver);

MODULE_LICENSE("GPL");

