/*
 * my-i2c-test-bbb.c
 * Simple I2C test driver for BeagleBone Black (Kernel 6.x)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/slab.h>

#define DRIVER_NAME "my-i2c-test"

struct my_test_data {
    struct i2c_client *client;
};

/* ============================================================
 * PROBE
 * Kernel 6.x signature
 * ============================================================*/
static int my_test_probe(struct i2c_client *client)
{
    struct my_test_data *data;
    int ret;

    dev_info(&client->dev,
             "Probing I2C test device at 0x%02x (bus %d)\n",
             client->addr, client->adapter->nr);

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    i2c_set_clientdata(client, data);
    data->client = client;

    /* Test read */
    ret = i2c_smbus_read_byte_data(client, 0x00);
    if (ret < 0)
        dev_warn(&client->dev,
                 "No real hardware detected (expected). Read failed.\n");
    else
        dev_info(&client->dev,
                 "Read register 0x00 = 0x%x\n", ret);

    return 0;
}

/* ============================================================
 * REMOVE
 * ============================================================*/
static void my_test_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "I2C test device removed\n");
}

/* ============================================================
 * I2C ID TABLE (non-DT)
 * ============================================================*/
static const struct i2c_device_id my_test_id[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, my_test_id);

/* ============================================================
 * DEVICE TREE MATCH TABLE
 * ============================================================*/
static const struct of_device_id my_test_of_match[] = {
    { .compatible = "mycompany,my-i2c-test" },
    { }
};
MODULE_DEVICE_TABLE(of, my_test_of_match);

/* ============================================================
 * I2C DRIVER STRUCT
 * ============================================================*/
static struct i2c_driver my_test_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = my_test_of_match,
    },
    .probe    = my_test_probe,
    .remove   = my_test_remove,
    .id_table = my_test_id,
};

/* ============================================================
 * MODULE INIT / EXIT
 * ============================================================*/
module_i2c_driver(my_test_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shiva");
MODULE_DESCRIPTION("Simple I2C Test Driver for BeagleBone Black");

