/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-02     yangjie      the first version
 */

#define USING_QSPI
#define USING_SPI

#include "drv_mtd_nand.h"
#include "drv_common.h"
#include "drv_nand_qspi.h"
#include "rtdef.h"

#include "drv_spi.h"

#define SPI_BUS_NAME "spi1"
#define SPI_NAND_FLASH_BUS_NAME "spi10"
#define SPI_NAND_FLASH_DEV_NAME "nand0"

// TODO:Enter QSPI mode
#ifdef NAND_USING_QSPI

#define QSPI_BUS_NAME   "qspi1"
#define QSPI_NAND_FLASH_BUS_NAME   "qspi10"
#define QSPI_CS_PIN_NUM   GET_PIN(G,6)

char w25nxx_read_status_register1(struct rt_qspi_device *device)
{
    rt_uint8_t cmd_data[2], res;
    char recv_buf;

    cmd_data[0] = NAND_GET_FEATURE;
    cmd_data[1] = NAND_SR1_ADDR;

    res = rt_qspi_send_then_recv(device, &cmd_data, sizeof(cmd_data), &recv_buf, 1);
    rt_kprintf("w25nxx_read_status_register1,res=%x\n", res);
    if (res <= 0)
    {
        rt_kprintf("rt_qspi_send_then_recv error,res = 0x%x \n", res);
    }
    rt_kprintf("recv_buf is 0x%x \n", recv_buf);
    return recv_buf;
}


char w25nxx_write_status_register1(struct rt_qspi_device *device, char reg)
{
    rt_uint8_t cmd_data[3], res;

    cmd_data[0] = NAND_SET_FEATURE;
    cmd_data[1] = NAND_SR1_ADDR;
    cmd_data[2] = reg;

    rt_qspi_send(device, &cmd_data, sizeof(cmd_data));
    rt_kprintf("w25nxx_write_status_register1,res=%x\n", res);
    if (res <= 0)
    {
        rt_kprintf("rt_qspi_send error,res = 0x%x \n", res);
    }
    rt_kprintf("rt_qspi_send ok \n");
}

void w25nxx_write_enable(struct rt_qspi_device *device)
{
    char res;
    /* 0x06 write enable */
    char instruction = 0x06;

    res = rt_qspi_send(device, &instruction, 1);
    rt_kprintf("w25nxx_write_enable,res=%x\n", res);
}

void w25nxx_enter_qspi_mode(struct rt_qspi_device *device)
{
    char status = 0;
    char write_status2_buf[2] = {0};

    w25nxx_write_enable(device);
    status = w25nxx_read_status_register1(device);
    if (!(status & 0x02))
    {
        status |= 1 << 1;
        w25nxx_write_status_register1(device, status);
        rt_kprintf("flash already enter qspi mode\n");
        rt_thread_mdelay(10);
    }
}
#endif


int rt_hw_w25n01_init(void)
{
    rt_uint8_t result = RT_EOK;

#ifdef NAND_USING_QSPI
    /* attatch qspi */
    result = stm32_qspi_bus_attach_device(QSPI_BUS_NAME, QSPI_NAND_FLASH_BUS_NAME, QSPI_CS_PIN_NUM, w25nxx_enter_qspi_mode, RT_NULL);
    if (result != RT_EOK)
    {
        rt_kprintf("nand dev attach to bus failed.\n");
        return RT_ERROR;
    }
#else
    /* attatch spi */
    result = rt_hw_spi_device_attach(SPI_BUS_NAME, SPI_NAND_FLASH_BUS_NAME, GPIOA, GPIO_PIN_4);
    if (result != RT_EOK)
    {
        rt_kprintf("nand dev attach to bus failed.\n");
        return RT_ERROR;
    }
#endif

    rt_spi_nand_probe(SPI_NAND_FLASH_DEV_NAME, SPI_NAND_FLASH_BUS_NAME);
    return RT_EOK;
}
INIT_APP_EXPORT(rt_hw_w25n01_init);

