/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-02     yangjie      the first version
 */

#include <rtdevice.h>
#include <spi_flash.h>
#include "drv_mtd_nand.h"

#define DBG_TAG     "drv_nand_qspi"
#define DBG_LVL     DBG_LOG
#include <rtdbg.h>

#ifndef RT_NAND_DEFAULT_SPI_CFG

#ifndef RT_NAND_SPI_MAX_HZ
    #define RT_NAND_SPI_MAX_HZ 50000000
#endif

#define RT_NAND_DEFAULT_SPI_CFG                  \
{                                                \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,          \
    .data_width = 8,                             \
    .max_hz = RT_NAND_SPI_MAX_HZ,                \
}
#endif /* RT_NAND_DEFAULT_SPI_CFG */



#ifdef NAND_USING_QSPI

#define RT_NAND_DEFAULT_QSPI_CFG                 \
{                                                \
    RT_NAND_DEFAULT_SPI_CFG,                     \
    .medium_size = 0x800000,                     \
    .ddr_mode = 0,                               \
    .qspi_dl_width = 4,                          \
}


//TODO : qspi_wr modify
static void qspi_set_cmd_format(nand_flash *flash, rt_uint8_t ins, rt_uint8_t ins_lines, rt_uint8_t addr_lines,
                                rt_uint8_t dummy_cycles, rt_uint8_t data_lines)
{

}

//TODO : qspi_wr modify
static rt_err_t nand_qspi_fast_read_enable(nand_flash *flash, rt_uint8_t data_line_width)
{
    rt_err_t result = RT_EOK;

    return result;
}
#endif /* NAND_USING_QSPI */

static rt_err_t spi_write_read(const nand_spi *spi,
                               const rt_uint8_t *write_buf,
                               rt_size_t write_size,
                               rt_uint8_t *read_buf,
                               rt_size_t read_size)
{
    rt_err_t result = RT_EOK;
    nand_flash_t nand_dev = (nand_flash_t)(spi->user_data);
    struct spi_nand_flash_mtd *rtt_dev = (struct spi_nand_flash_mtd *)(nand_dev->user_data);

    RT_ASSERT(spi);
    RT_ASSERT(nand_dev);
    RT_ASSERT(rtt_dev);
#ifdef NAND_USING_QSPI
    struct rt_qspi_device *qspi_dev = RT_NULL;
#endif
    if (write_size)
    {
        RT_ASSERT(write_buf);
    }
    if (read_size)
    {
        RT_ASSERT(read_buf);
    }

#ifdef NAND_USING_QSPI
    if (rtt_dev->rt_spi_device->bus->mode & RT_SPI_BUS_MODE_QSPI)
    {
        qspi_dev = (struct rt_qspi_device *)(rtt_dev->rt_spi_device);
        if (write_size && read_size)
        {
            if (rt_qspi_send_then_recv(qspi_dev, write_buf, write_size, read_buf, read_size) <= 0)
            {
                result = -RT_ETIMEOUT;
            }
        }
        else if (write_size)
        {
            if (rt_qspi_send(qspi_dev, write_buf, write_size) <= 0)
            {
                result = -RT_ETIMEOUT;
            }
        }
    }
    else
#endif
    {
        if (write_size && read_size)
        {
            if (rt_spi_send_then_recv(rtt_dev->rt_spi_device, write_buf, write_size, read_buf, read_size) != RT_EOK)
                result = -RT_ETIMEOUT;
        }
        else if (write_size)
        {
            if (rt_spi_send(rtt_dev->rt_spi_device, write_buf, write_size) <= 0)
                result = -RT_ETIMEOUT;
        }
        else
        {
            if (rt_spi_recv(rtt_dev->rt_spi_device, read_buf, read_size) <= 0)
                result = -RT_ETIMEOUT;
        }
    }
    return result;
}

#ifdef NAND_USING_QSPI
//TODO : qspi_wr modify
static rt_err_t qspi_read_write(const nand_spi *spi,
                                rt_uint32_t addr,
                                nand_qspi_cmd_format *qspi_cmd_format,
                                rt_uint8_t *write_buf,
                                rt_size_t write_size,
                                rt_uint8_t *read_buf,
                                rt_size_t read_size)
{
    rt_err_t result = RT_EOK;

    return result;

}
#endif


static void spi_lock(const nand_spi *spi)
{
    nand_spi *nand_dev = (nand_spi *)(spi->user_data);
    struct spi_nand_flash_mtd *rtt_dev = (struct spi_nand_flash_mtd *)(nand_dev->user_data);

    RT_ASSERT(spi);
    RT_ASSERT(nand_dev);
    RT_ASSERT(rtt_dev);

    rt_mutex_take(&(rtt_dev->lock), RT_WAITING_FOREVER);
}

static void spi_unlock(const nand_spi *spi)
{
    nand_flash *nand_dev = (nand_flash *)(spi->user_data);
    struct spi_nand_flash_mtd *rtt_dev = (struct spi_nand_flash_mtd *)(nand_dev->user_data);

    RT_ASSERT(spi);
    RT_ASSERT(nand_dev);
    RT_ASSERT(rtt_dev);

    rt_mutex_release(&(rtt_dev->lock));
}

static void retry_delay_100us(void)
{
    /* 100 microsecond delay */
    rt_thread_delay((RT_TICK_PER_SECOND * 1 + 9999) / 10000);
}

rt_err_t _spi_nand_bus_init(nand_flash_t flash)
{
    rt_err_t result = RT_EOK;

    RT_ASSERT(flash);

    /* port SPI device interface */
    flash->spi.wr = spi_write_read;
#ifdef NAND_USING_QSPI
    flash->spi.qspi_wr = qspi_read_write;
#endif
    flash->spi.lock = spi_lock;
    flash->spi.unlock = spi_unlock;
    flash->spi.user_data = flash;
    if (RT_TICK_PER_SECOND < 1000)
    {
        rt_kprintf("[SNFUD] Warning: The OS tick(%d) is less than 1000. So the flash write will take more time.\n", RT_TICK_PER_SECOND);
    }
    /* 100 microsecond delay */
    flash->retry.delay = retry_delay_100us;
    /* 60 seconds timeout */
    flash->retry.times = 60 * 10000;

    return result;
}

rt_spi_nand_flash_device_t rt_spi_nand_probe_ex(const char *spi_nand_dev_name, const char *spi_nand_bus_name,
                                                struct rt_spi_configuration *spi_cfg, struct rt_qspi_configuration *qspi_cfg)
{
    struct spi_nand_flash_mtd *rtt_dev = RT_NULL;
    nand_flash *nand_dev = RT_NULL;
    char *spi_flash_dev_name_bak = RT_NULL, *spi_dev_name_bak = RT_NULL;

#ifdef NAND_USING_QSPI
    struct rt_qspi_device *qspi_dev = RT_NULL;
#endif
    rt_err_t result = RT_EOK;

    RT_ASSERT(spi_nand_dev_name);
    RT_ASSERT(spi_nand_bus_name);

    rtt_dev = (rt_spi_nand_flash_device_t) rt_malloc(sizeof(struct spi_nand_flash_mtd));
    nand_dev = (nand_flash_t) rt_malloc(sizeof(nand_flash));
    spi_flash_dev_name_bak = (char *) rt_malloc(rt_strlen(spi_nand_dev_name) + 1);
    spi_dev_name_bak = (char *) rt_malloc(rt_strlen(spi_nand_bus_name) + 1);

    if (rtt_dev)
    {
        rt_memset(rtt_dev, 0, sizeof(struct spi_flash_device));
        /* initialize lock */
        rt_mutex_init(&(rtt_dev->lock), spi_nand_dev_name, RT_IPC_FLAG_FIFO);
    }

    if (rtt_dev && nand_dev && spi_flash_dev_name_bak && spi_dev_name_bak)
    {
        rt_memset(nand_dev, 0, sizeof(nand_flash));
        rt_strncpy(spi_flash_dev_name_bak, spi_nand_dev_name, rt_strlen(spi_nand_dev_name));
        rt_strncpy(spi_dev_name_bak, spi_nand_bus_name, rt_strlen(spi_nand_bus_name));
        /* make string end sign */
        spi_flash_dev_name_bak[rt_strlen(spi_nand_dev_name)] = '\0';
        spi_dev_name_bak[rt_strlen(spi_nand_bus_name)] = '\0';
        /* SPI configure */
        {
            /* RT-Thread SPI device initialize */
            rtt_dev->rt_spi_device = (struct rt_spi_device *) rt_device_find(spi_nand_bus_name);
            if (rtt_dev->rt_spi_device == RT_NULL || rtt_dev->rt_spi_device->parent.type != RT_Device_Class_SPIDevice)
            {
                LOG_E("ERROR: SPI device %s not found!", spi_nand_bus_name);
                goto error;
            }
            nand_dev->spi.name = spi_dev_name_bak;

#ifdef NAND_USING_QSPI
            /* set the qspi line number and configure the QSPI bus */
            if (rtt_dev->rt_spi_device->bus->mode & RT_SPI_BUS_MODE_QSPI)
            {
                qspi_dev = (struct rt_qspi_device *)rtt_dev->rt_spi_device;
                qspi_cfg->qspi_dl_width = qspi_dev->config.qspi_dl_width;
                rt_qspi_configure(qspi_dev, qspi_cfg);
            }
            else
#endif
                rt_spi_configure(rtt_dev->rt_spi_device, spi_cfg);
        }
        /* NAND flash device initialize */
        {
            nand_dev->name = spi_flash_dev_name_bak;
            /* accessed each other */
            rtt_dev->user_data = nand_dev;
            rtt_dev->rt_spi_device->user_data = rtt_dev;
            nand_dev->user_data = rtt_dev;
            nand_dev->spi.user_data = nand_dev;
            /* initialize NAND device */
            if (_spi_nand_bus_init(nand_dev) != RT_EOK)
            {
                LOG_E("ERROR: SPI flash probe failed by SPI device %s.", spi_nand_bus_name);
                goto error;
            }

#ifdef NAND_USING_QSPI
            //TODO : qspi config
            /* reconfigure the QSPI bus for medium size */
            if (rtt_dev->rt_spi_device->bus->mode & RT_SPI_BUS_MODE_QSPI)
            {
                // qspi_cfg->medium_size = 128*1021*1024;
                rt_qspi_configure(qspi_dev, qspi_cfg);
                if (qspi_dev->enter_qspi_mode != RT_NULL)
                {
                    qspi_dev->enter_qspi_mode(qspi_dev);
                }
                /* set data lines width */
                nand_qspi_fast_read_enable(nand_dev, qspi_dev->config.qspi_dl_width);
            }
#endif /* NAND_USING_QSPI */
        }

        extern int rt_hw_nand_init(struct rt_mtd_nand_device *device);
        result = rt_hw_nand_init(&rtt_dev->mtd_nand_device);
        if (result != RT_EOK)
        {
            LOG_E("ERROR: hardware nand init error.");
            goto error;
        }
        LOG_I("Probe SPI flash %s by SPI device %s success.", spi_nand_dev_name, spi_nand_bus_name);
        return rtt_dev;
    }
    else
    {
        LOG_E("ERROR: Low memory.");
        goto error;
    }

error:

    if (rtt_dev)
    {
        rt_mutex_detach(&(rtt_dev->lock));
    }
    /* may be one of objects memory was malloc success, so need free all */
    rt_free(rtt_dev);
    rt_free(nand_dev);
    rt_free(spi_flash_dev_name_bak);
    rt_free(spi_dev_name_bak);

    return RT_NULL;
}

/*
 * spi_nand_dev_name: spi flash device name,such as nand0, nand1.
 * spi_nand_bus_name: spi flash slave name, such as spi10, qspi10.
 */
rt_spi_nand_flash_device_t rt_spi_nand_probe(const char *spi_nand_dev_name, const char *spi_nand_bus_name)
{
    struct rt_spi_configuration cfg = RT_NAND_DEFAULT_SPI_CFG;

#ifndef NAND_USING_QSPI
    return rt_spi_nand_probe_ex(spi_nand_dev_name, spi_nand_bus_name, &cfg, RT_NULL);
#else
    struct rt_qspi_configuration qspi_cfg = RT_NAND_DEFAULT_QSPI_CFG;

    return rt_spi_nand_probe_ex(spi_nand_dev_name, spi_nand_bus_name, &cfg, &qspi_cfg);
#endif

}

