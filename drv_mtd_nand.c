/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-02     yangjie      the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <spi_flash.h>
#include <string.h>
#include "drv_mtd_nand.h"
#include <rthw.h>

#define DBG_TAG     "drv_mtd_nand"
#define DBG_LVL     DBG_LOG
#include <rtdbg.h>

const nand_flash_chip       nand_flash_chip_table[] = SPI_NAND_FLASH_CHIP_TABLE;
const nand_flash_chip_info  nand_flash_info_table[] = SPI_NAND_FLASH_CHIP_INFO;

/*
 * NAND set feature cmd
 */
#define NAND_PROTECT_ENABLE   1
#define NAND_PROTECT_DISABLE  2
#define NAND_ECC_ENABLE       3
#define NAND_ECC_DISABLE      4
#define NAND_BUF_ENABLE       5     /* Just for winbond */
#define NAND_BUF_DISABLE      6

/*
 * spi_nand_get_feature: read status, or get feature.
 * sr_addr: status register addr
 * sr_value: status register value
 */
int spi_nand_get_feature(struct rt_mtd_nand_device *device, rt_uint8_t sr_addr, rt_uint8_t *sr_value)
{
    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    rt_uint8_t cmd_data[2];

    if ((sr_addr != NAND_SR1_ADDR) && (sr_addr != NAND_SR2_ADDR) && (sr_addr != NAND_SR3_ADDR))
    {
        return RT_EINVAL;
    }

    cmd_data[0] = NAND_GET_FEATURE;
    cmd_data[1] = sr_addr;

    nand_dev->spi.wr(spi, cmd_data, sizeof(cmd_data), sr_value, 1);
    return RT_EOK;
}

int spi_nand_set_feature(struct rt_mtd_nand_device *device, rt_uint8_t cmd)
{
    rt_uint8_t cmd_data[3];
    rt_uint8_t sr_addr = 0;
    rt_uint8_t sr_value;

    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    switch (cmd)
    {
    case NAND_PROTECT_ENABLE:
        sr_addr = (nand_dev->chip_info.bp_bit >> 8) & 0xff;
        spi_nand_get_feature(device, sr_addr, &sr_value);
        sr_value |= ((nand_dev->chip_info.bp_bit) & 0xff);
        break;

    case NAND_PROTECT_DISABLE:
        sr_addr = (nand_dev->chip_info.bp_bit >> 8) & 0xff;
        spi_nand_get_feature(device, sr_addr, &sr_value);
        sr_value &= (~(nand_dev->chip_info.bp_bit) & 0xff);
        break;

    case NAND_ECC_ENABLE:
        sr_addr = (nand_dev->chip_info.ecc_bit >> 8) & 0xff;
        spi_nand_get_feature(device, sr_addr, &sr_value);
        sr_value |= ((nand_dev->chip_info.ecc_bit) & 0xff);
        break;

    case NAND_ECC_DISABLE:
        sr_addr = (nand_dev->chip_info.ecc_bit >> 8) & 0xff;
        spi_nand_get_feature(device, sr_addr, &sr_value);
        sr_value &= (~(nand_dev->chip_info.ecc_bit) & 0xff);
        break;

    case NAND_BUF_ENABLE:
        sr_addr = NAND_SR2_ADDR;
        spi_nand_get_feature(device, sr_addr, &sr_value);
        sr_value |= 0x08;
        break;

    case NAND_BUF_DISABLE:
        sr_addr = NAND_SR2_ADDR;
        spi_nand_get_feature(device, sr_addr, &sr_value);
        sr_value &= (~0x08);
        break;

    default:
        LOG_E("ERROR: cmd not support.");
        return RT_ERROR;
        break;

    }
    cmd_data[0] = NAND_SET_FEATURE;
    cmd_data[1] = sr_addr;
    cmd_data[2] = sr_value;

    nand_dev->spi.wr(spi, cmd_data, sizeof(cmd_data), 0, 0);
    return RT_EOK;
}

static rt_err_t spi_nand_wait_busy(struct rt_mtd_nand_device *device)
{
    rt_uint8_t sr_addr = 0;
    rt_uint8_t sr_value = 0;
    rt_uint8_t sr_busy_bit_mask = 0;

    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;

    sr_addr = (nand_dev->chip_info.busy_bit >> 8) & 0xff;
    sr_busy_bit_mask = (nand_dev->chip_info.busy_bit) & 0xff;

    while (1)
    {
        spi_nand_get_feature(device, sr_addr, &sr_value);
        if ((sr_value & sr_busy_bit_mask) == 0)
        {
            return RT_EOK;
        }
    }
}

static rt_err_t _read_id(struct rt_mtd_nand_device *device)
{
    rt_uint8_t recv_buff[4] = { 0 };
    int i = 0;
    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    rt_uint8_t cmd_data = NAND_READ_ID;

    nand_dev->spi.wr(spi, &cmd_data, 1, recv_buff, 4);

    LOG_I("Nand flash device id is 0x%x%x%x.", recv_buff[1], recv_buff[2], recv_buff[3]);

    /* find device from chip table */
    for (i = 0; i < sizeof(nand_flash_chip_table) / sizeof(nand_flash_chip); i++)
    {
        if ((nand_flash_chip_table[i].mf_id == recv_buff[1])
                && (nand_flash_chip_table[i].type_id == recv_buff[2])
                && (nand_flash_chip_table[i].capacity_id == recv_buff[3]))
        {
            nand_dev->chip.name = nand_flash_chip_table[i].name;
            nand_dev->chip.mf_id = nand_flash_chip_table[i].mf_id;
            nand_dev->chip.type_id = nand_flash_chip_table[i].type_id;
            nand_dev->chip.capacity_id = nand_flash_chip_table[i].capacity_id;
            LOG_I("Nand flash device name is %s.", nand_dev->chip.name);
            return RT_EOK;
        }
        else
        {
            LOG_I("Unkonwn nand, device id is 0x%x%x%x.", recv_buff[1], recv_buff[2], recv_buff[3]);
            return RT_ERROR;
        }
    }

    return RT_EOK;
}

static rt_err_t _read_page(struct rt_mtd_nand_device *device,
                           rt_off_t page,
                           rt_uint8_t *data,
                           rt_uint32_t data_len,
                           rt_uint8_t *spare,
                           rt_uint32_t spare_len)
{
    int res = RT_EOK;
    rt_uint8_t sr2;

    rt_uint8_t page_data[4], column_data[4];
    rt_uint16_t column_addr = 0;
    rt_uint8_t oob[NAND_PAGE_OOB_SIZE];

    RT_ASSERT(device != NULL);
    RT_ASSERT(data_len <= device->page_size);
    RT_ASSERT(spare_len <= device->oob_size);

    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    page = page + (device->block_start) * (device->pages_per_block);
    if (page >= (device->block_end) * device->pages_per_block)
    {
        LOG_E("failed to read page, the page %d is out of bound.", page);
        return -RT_ERROR;
    }

    res = spi_nand_get_feature(device, NAND_SR2_ADDR, &sr2);
    if (res != RT_EOK)
    {
        return res;
    }

    /* 0x13 dummy[8bit] page_addr[16bit] */
    page_data[0] = NAND_READ_PAGE_TO_CACHE;
    page_data[1] = DUMMY_CMD;
    page_data[2] = (page >> 8) & 0xff;
    page_data[3] = page & 0xff;

    nand_dev->spi.wr(spi, page_data, 4, 0, 0);
#ifdef NAND_USING_QSPI
    //TODO: Organize commands and data,and send them through QSPI
    nand_dev->spi.qspi_wr(spi, 0, (nand_qspi_cmd_format *)&nand_dev->qspi_cmd_format, &page_data, 4, 0, 0);
#endif

    rt_hw_us_delay(500);

    res = strcmp(nand_dev->chip.name, "W25N01GV");
    if ((res) || ((!res) && (sr2 & NAND_SR2_BUF_BIT_MASK)))
    {
        if (data != RT_NULL && data_len != 0)
        {
            /* 0x03 cl_addr[16bit] dummy[8bit] */
            column_data[0] = NAND_READ_FROM_CACHE;
            column_data[1] = (column_addr >> 8) & 0x0f; //only CA[11:0] is effective
            column_data[2] = column_addr & 0xff;
            column_data[3] = DUMMY_CMD;

            nand_dev->spi.wr(spi, column_data, 4, data, (data_len));
#ifdef NAND_USING_QSPI
            //TODO: Organize commands and data,and send them through QSPI
            nand_dev->spi.qspi_wr(spi, 0, 0, &column_data, 4, data, (data_len));
#endif

            rt_hw_us_delay(500);
            column_addr = NAND_PAGE_DATA_SIZE;
            /* 0x03 cl_addr[16bit] dummy[8bit] */
            column_data[0] = NAND_READ_FROM_CACHE;
            column_data[1] = (column_addr >> 8) & 0x0f; /* only CA[11:0] is effective */
            column_data[2] = column_addr & 0xff;
            column_data[3] = DUMMY_CMD;

            nand_dev->spi.wr(spi, column_data, 4, oob, (NAND_PAGE_OOB_SIZE));
#ifdef NAND_USING_QSPI
            //TODO: Organize commands and data,and send them through QSPI
            nand_dev->spi.qspi_wr(spi, 0, 0, &column_data, 4, oob, (NAND_PAGE_OOB_SIZE));
#endif

            /* verify ECC */
#ifdef RT_USING_NFTLaa
            if (nftl_ecc_verify256(data, NAND_PAGE_DATA_SIZE, oob) != RT_MTD_EOK)
            {
                res = -RT_MTD_EECC;
                LOG_E("ECC failed!, page:%d", page);
            }
#endif

        }
        if (spare != RT_NULL && spare_len != 0)
        {
            column_addr = NAND_PAGE_DATA_SIZE;
            /* 0x03 cl_addr[16bit] dummy[8bit] */
            column_data[0] = NAND_READ_FROM_CACHE;
            column_data[1] = (column_addr >> 8) & 0x0f; /* only CA[11:0] is effective */
            column_data[2] = column_addr & 0xff;
            column_data[3] = DUMMY_CMD;

            nand_dev->spi.wr(spi, column_data, 4, oob, (NAND_PAGE_OOB_SIZE));
#ifdef NAND_USING_QSPI
            //TODO: Organize commands and data,and send them through QSPI
            nand_dev->spi.qspi_wr(spi, 0, 0, &column_data, 4, oob, (NAND_PAGE_OOB_SIZE));
#endif

            if (spare != RT_NULL)
            {
                rt_memcpy(spare, oob, NAND_PAGE_OOB_SIZE);
            }
        }

    }
    else
    {
        // TODO: this is only in winbond(BUF=0,read begin with the page, end with all nand)
        /* 0x03 dummy[24bit] */
        column_data[0] = NAND_READ_FROM_CACHE;
        column_data[1] = DUMMY_CMD;
        column_data[2] = DUMMY_CMD;
        column_data[3] = DUMMY_CMD;
        rt_kprintf("BUF bit=0,will read one page to end of the nand,TODO......\n");
        return RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t spi_nand_write_enable(struct rt_mtd_nand_device *device)
{
    rt_uint8_t cmd_data = NAND_WRITE_ENABLE;

    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    nand_dev->spi.wr(spi, &cmd_data, 1, 0, 0);

    return RT_EOK;
}

static rt_err_t spi_nand_write_disable(struct rt_mtd_nand_device *device)
{
    rt_uint8_t cmd_data = NAND_WRITE_DISABLE;//0x04

    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    nand_dev->spi.wr(spi, &cmd_data, 1, 0, 0);
    return RT_EOK;
}

rt_err_t _write_page(struct rt_mtd_nand_device *device,
                     rt_off_t page,
                     const rt_uint8_t *data, rt_uint32_t data_len,
                     const rt_uint8_t *spare, rt_uint32_t spare_len)
{
    rt_uint8_t cmd_data[3], execute_data[4];
    rt_uint16_t column_addr = 0;
    rt_uint8_t oob[NAND_PAGE_OOB_SIZE];

    RT_ASSERT(data_len <= device->page_size);
    RT_ASSERT(spare_len <= device->oob_size);

    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    page = page + device->block_start * device->pages_per_block;
    if (page >= (device->block_end) * device->pages_per_block)
    {
        return -RT_ERROR;
    }

    if (data != RT_NULL && data_len != 0)   /* write data */
    {
        column_addr = 0;
        /* 0x02 cl_addr[16bit] write_buff */
        cmd_data[0] = NAND_WRITE;
        cmd_data[1] = (column_addr >> 8) & 0x0f; /* only CA[11:0] is effective */
        cmd_data[2] = column_addr & 0xff;

        spi_nand_set_feature(device, NAND_PROTECT_DISABLE);
        spi_nand_write_enable(device);
        rt_hw_us_delay(900);

        rt_spi_send_then_send(rtt_dev->rt_spi_device, cmd_data, sizeof(cmd_data), data, (data_len));

        /* Progrom Excute: 0x10 dummy[8bit] page_addr[16bit] */
        execute_data[0] = NAND_WRITE_EXECUTE;
        execute_data[1] = DUMMY_CMD;
        execute_data[2] = (page >> 8) & 0xff;
        execute_data[3] = page & 0xff;

        nand_dev->spi.wr(spi, execute_data, sizeof(execute_data), 0, 0);
#ifdef NAND_USING_QSPI
        //TODO: Organize commands and data,and send them through QSPI
        nand_dev->spi.qspi_wr(spi, 0, 0, &execute_data, sizeof(execute_data), 0, 0);
#endif

        /* wait busy */
        spi_nand_wait_busy(device);
        spi_nand_write_disable(device);
        spi_nand_set_feature(device, NAND_PROTECT_ENABLE);

        rt_hw_us_delay(500);
        if (spare != RT_NULL && spare_len != 0)         /* write spare */
        {
            if (spare != RT_NULL && spare_len > 0)
            {
                memcpy(oob, spare, spare_len);
            }

#ifdef RT_USING_NFTLxx
            nftl_ecc_compute256(data, NAND_PAGE_DATA_SIZE, oob);
#endif
            column_addr = NAND_PAGE_DATA_SIZE;

            /* 0x02 cl_addr[16bit] write_buff */
            cmd_data[0] = NAND_WRITE;
            cmd_data[1] = (column_addr >> 8) & 0x0f;  /* only CA[11:0] is effective */
            cmd_data[2] = column_addr & 0xff;

            spi_nand_set_feature(device, NAND_PROTECT_DISABLE);
            spi_nand_write_enable(device);
            rt_hw_us_delay(900);

            rt_spi_send_then_send(rtt_dev->rt_spi_device, cmd_data, sizeof(cmd_data), oob, (spare_len));

            /* Progrom Excute: 0x10 dummy[8bit] page_addr[16bit]  */
            execute_data[0] = NAND_WRITE_EXECUTE;
            execute_data[1] = DUMMY_CMD;
            execute_data[2] = (page >> 8) & 0xff;
            execute_data[3] = page & 0xff;

            nand_dev->spi.wr(spi, execute_data, sizeof(execute_data), 0, 0);
#ifdef NAND_USING_QSPI
            //TODO: Organize commands and data,and send them through QSPI
            nand_dev->spi.qspi_wr(spi, 0, 0, &execute_data, sizeof(execute_data), 0, 0);
#endif

            /* wait busy */
            spi_nand_wait_busy(device);
            spi_nand_write_disable(device);
            spi_nand_set_feature(device, NAND_PROTECT_ENABLE);
        }
    }
    else if (spare != RT_NULL && spare_len != 0)   /* write spare */
    {
        column_addr = NAND_PAGE_DATA_SIZE;

        /* 0x02 cl_addr[16bit] write_buff */
        cmd_data[0] = NAND_WRITE;
        cmd_data[1] = (column_addr >> 8) & 0x0f;  /* only CA[11:0] is effective */
        cmd_data[2] = column_addr & 0xff;

        spi_nand_set_feature(device, NAND_PROTECT_DISABLE);
        spi_nand_write_enable(device);
        rt_hw_us_delay(900);

        rt_spi_send_then_send(rtt_dev->rt_spi_device, cmd_data, sizeof(cmd_data), spare, (spare_len));

        /* Progrom Excute: 0x10 dummy[8bit] page_addr[16bit] */
        execute_data[0] = NAND_WRITE_EXECUTE;
        execute_data[1] = DUMMY_CMD;
        execute_data[2] = (page >> 8) & 0xff;
        execute_data[3] = page & 0xff;

        nand_dev->spi.wr(spi, execute_data, sizeof(execute_data), 0, 0);
#ifdef NAND_USING_QSPI
        //TODO: Organize commands and data,and send them through QSPI
        nand_dev->spi.qspi_wr(spi, 0, 0, &execute_data, sizeof(execute_data), 0, 0);
#endif

        /* wait busy */
        spi_nand_wait_busy(device);
        spi_nand_write_disable(device);
        spi_nand_set_feature(device, NAND_PROTECT_ENABLE);
    }

    rt_hw_us_delay(900);

    return 0;
}

rt_err_t _erase_block(struct rt_mtd_nand_device *device, rt_uint32_t block)
{
    rt_uint8_t erase_cmd[4];
    int res = RT_EOK;
    rt_uint16_t page_addr = 0;

    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    /* write enable */
    spi_nand_set_feature(device, NAND_PROTECT_DISABLE);

    spi_nand_write_enable(device);
    rt_hw_us_delay(200);

    /* block erase: 0xd8 dummy[8bit] page_addr[16bit] */
    block = block + device->block_start;
    page_addr = block * (device->pages_per_block);

    erase_cmd[0] = NAND_BLOCK_ERASE;
    erase_cmd[1] = DUMMY_CMD;
    erase_cmd[2] = (page_addr >> 8) & 0xff;
    erase_cmd[3] = page_addr & 0xff;

    res = nand_dev->spi.wr(spi, erase_cmd, sizeof(erase_cmd), 0, 0);
    if (res != 0)
    {
        LOG_E("erase block err. err num %x.", res);
        return -RT_ERROR;
    }

    /* wait busy */
    spi_nand_wait_busy(device);
    /* write disable */
    spi_nand_write_disable(device);
    spi_nand_set_feature(device, NAND_PROTECT_ENABLE);

    return RT_EOK;
}

rt_err_t _move_page(struct rt_mtd_nand_device *device, rt_off_t src_page, rt_off_t dst_page)
{
    return RT_EOK;
}

rt_err_t _check_block(struct rt_mtd_nand_device *device, rt_uint32_t block)
{
    return RT_EOK;
}

rt_err_t _mark_badblock(struct rt_mtd_nand_device *device, rt_uint32_t block)
{
    return RT_EOK;
}

int spi_erase_all_nand(struct rt_mtd_nand_device *device)
{
    int i = 0;

    for (i = 0; i < NAND_BLOCK_NUM; i++) //1024 blocks
    {
        _erase_block(device, i * 64);
    }
    return 0;
}
void spi_nand_reset(struct rt_mtd_nand_device *device)
{
    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;
    const nand_spi *spi = &nand_dev->spi;

    rt_uint8_t cmd_data = NAND_RESET;

    nand_dev->spi.wr(spi, &cmd_data, 1, 0, 0);
}

static const struct rt_mtd_nand_driver_ops nand_ops =
{
    _read_id,
    _read_page,
    _write_page,
    0,
    _erase_block,
    0,
    0,
};

int rt_hw_nand_init(struct rt_mtd_nand_device *device)
{
    rt_err_t result = RT_EOK;
    rt_uint8_t i = 0;

    struct spi_nand_flash_mtd *rtt_dev =  rt_container_of(device, struct spi_nand_flash_mtd, mtd_nand_device);
    nand_flash_t nand_dev = (nand_flash_t)rtt_dev->user_data;

    /* get nand device id information */
    result = _read_id(device);
    if (result != RT_EOK)
    {
        return -RT_ERROR;
    }

    /* get nand device capacity and register information */
    for (i = 0; i < sizeof(nand_flash_info_table) / sizeof(nand_flash_chip_info); i++)
    {
        if (nand_dev->chip.name == nand_flash_info_table[i].name)
        {
            nand_dev->chip_info.name = nand_dev->chip.name;
            nand_dev->chip_info.capacity = nand_flash_info_table[i].capacity;
            nand_dev->chip_info.ecc_bit = nand_flash_info_table[i].ecc_bit;
            nand_dev->chip_info.bp_bit = nand_flash_info_table[i].bp_bit;
            nand_dev->chip_info.busy_bit = nand_flash_info_table[i].busy_bit;
            nand_dev->chip_info.qe_bit = nand_flash_info_table[i].qe_bit;

            LOG_I("Nand flash capacity is %d Gbit.", nand_dev->chip_info.capacity);
            break;
        }
    }

    if (nand_dev->chip_info.capacity == 1)
    {
        device->page_size       = 2048;
        device->pages_per_block = 64;
        device->plane_num       = 1;
        device->oob_size        = 64;
        device->oob_free        = 64 - ((2048) * 3 / 256);
        device->block_start     = 0;
        device->block_end       = 1024;
        device->block_total     = 1024;
    }
    else
    {
        // TODO: add other capacity config
        LOG_I("TODO: other capacity config.");
    }

    device->ops = &nand_ops;
    result = rt_mtd_nand_register_device(nand_dev->name, device);
    if (result != RT_EOK)
    {
        rt_device_unregister((rt_device_t)device);
        return -RT_ERROR;
    }

#ifdef NAND_USING_HW_ECC
    //spi_nand_ecc_enable(device);
    spi_nand_set_feature(device, NAND_ECC_ENABLE);
#else
    //spi_nand_ecc_disable(device);
    spi_nand_set_feature(device, NAND_ECC_DISABLE);
#endif

    spi_nand_set_feature(device, NAND_BUF_ENABLE);

    LOG_I("Nand flash init success.");
    return RT_EOK;
}


