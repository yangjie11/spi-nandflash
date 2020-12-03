/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-02     yangjie      the first version
 */
#ifndef DRV_NAND_FLASH_H_
#define DRV_NAND_FLASH_H_

#include <rtdef.h>

/* read cmd */
#define NAND_READ_ID                    0x9f    /* Read id */
#define NAND_READ_PAGE_TO_CACHE         0x13    /* Read Page Data to cache */
#define NAND_READ_FROM_CACHE            0x03    /* Read data from cache*/
#define NAND_QUAD_READ                  0x6b

/* write cmd */
#define NAND_WRITE_ENABLE               0x06
#define NAND_WRITE_DISABLE              0x04
#define NAND_WRITE                      0x02
#define NAND_QUAD_WRITE                 0x32
#define NAND_WRITE_EXECUTE              0x10

/* Erase cmd */
#define NAND_BLOCK_ERASE                0xd8
/* Reset cmd */
#define NAND_RESET                      0xff
/* Dummy cmd */
#define DUMMY_CMD                       0x00

/* Features cmd */
#define NAND_GET_FEATURE                0x0f    /* Read Status Register (Get features) */
#define NAND_SET_FEATURE                0x1f    /* Write Status Register(Set features) */

/* Status register addr cmd */
#define NAND_SR1_ADDR                   0xa0    /* Status Register:Protection Register Addr */
#define NAND_SR2_ADDR                   0xb0    /* Status Register:Configuration Register Addr */
#define NAND_SR3_ADDR                   0xc0    /* Status Register:Status */

/* Status register bit mask */
#define NAND_SR1_TB_BIT_MASK            0x04
#define NAND_SR1_BP_BIT_MASK            0x78
#define NAND_SR2_BUF_BIT_MASK           0x08
#define NAND_SR2_ECC_BIT_MASK           0x10
#define NAND_SR2_QE_BIT_MASK            0x01
#define NAND_SR2_OTPL_BIT_MASK          0x80
#define NAND_SR3_BUSY_BIT_MASK          0x01
#define NAND_SR3_WEL_BIT_MASK           0x02



/* Nand flash config */
#define NAND_PAGE_DATA_SIZE           (2048)
#define NAND_PAGES_PER_BLOCK          (64)
#define NAND_PLANE_NUM                (1)
#define NAND_PAGE_OOB_SIZE            (64)
#define NAND_PAGE_OOB_FREE            (64-(2048*3/256))
#define NAND_BLOCK_NUM                (1024)

/* qspi cmd format struct */
#ifdef NAND_USING_QSPI
/**
 * QSPI flash Data cmd format
 */
typedef struct
{
    rt_uint8_t instruction;
    rt_uint8_t instruction_lines;
    rt_uint8_t address_size;
    rt_uint8_t address_lines;
    rt_uint8_t alternate_bytes_lines;
    rt_uint8_t dummy_cycles;
    rt_uint8_t data_lines;
} nand_qspi_cmd_format;

enum nand_qspi_wr_mode
{
    NORMAL_SPI_READ = 1 << 0,               /**< mormal spi read mode */
    DUAL_OUTPUT = 1 << 1,                   /**< qspi fast read dual output */
    DUAL_IO = 1 << 2,                       /**< qspi fast read dual input/output */
    QUAD_OUTPUT = 1 << 3,                   /**< qspi fast read quad output */
    QUAD_IO = 1 << 4,                       /**< qspi fast read quad input/output */
};
#endif /* NAND_USING_QSPI */

/**
 * SPI device
 */
typedef struct __nand_spi
{
    /* SPI device name */
    char *name;
    /* SPI bus write read data function */
    rt_err_t (*wr)(const struct __nand_spi *spi, const rt_uint8_t *write_buf, rt_size_t write_size,
                   rt_uint8_t *read_buf, rt_size_t read_size);
#ifdef NAND_USING_QSPI
    /* Quad Load Program */
    rt_err_t (*qspi_wr)(const struct __nand_spi *spi,  rt_uint32_t addr, nand_qspi_cmd_format *qspi_cmd_format,
                        rt_uint8_t *write_buf, rt_size_t write_size, rt_uint8_t *read_buf, rt_size_t read_size);
#endif
    /* lock SPI bus */
    void (*lock)(const struct __nand_spi *spi);
    /* unlock SPI bus */
    void (*unlock)(const struct __nand_spi *spi);
    /* some user data */
    void *user_data;
} nand_spi, *nand_spi_t;


/* nand flash chip ID information */
typedef struct
{
    char *name;                                  /**< flash chip name */
    rt_uint8_t mf_id;                               /**< manufacturer ID */
    rt_uint8_t type_id;                             /**< memory type ID */
    rt_uint8_t capacity_id;                         /**< capacity ID */
} nand_flash_chip;

/* nand flash chip REGISTER information */
typedef struct
{
    char *name;                                  /**< flash chip name */
    rt_uint8_t  capacity;                            /**< flash chip capacity */
    rt_uint16_t bp_bit;
    rt_uint16_t ecc_bit;
    rt_uint16_t qe_bit;
    rt_uint16_t busy_bit;
} nand_flash_chip_info;

typedef struct
{
    char *name;                                  /**< nand flash name */
    rt_size_t index;                                /**< index of flash device information table  @see flash_table */
    nand_flash_chip chip;                        /**< flash chip information */
    nand_flash_chip_info chip_info;
    nand_spi spi;                                /**< SPI device */
    rt_bool_t init_ok;                                /**< initialize OK flag */
    rt_bool_t addr_in_4_byte;                         /**< flash is in 4-Byte addressing */

    struct
    {
        void (*delay)(void);                     /**< every retry's delay */
        rt_size_t times;                            /**< default times for error retry */
    } retry;

    void *user_data;                             /**< some user data */

#ifdef NAND_USING_QSPI
    nand_qspi_cmd_format qspi_cmd_format;        /**< fast read cmd format */
#endif

} nand_flash, *nand_flash_t;


/*
 * FLASH id info
 *
 * | name | mf_id | type_id | capacity_id
 */
#define SPI_NAND_FLASH_CHIP_TABLE                 \
{                                                 \
    {"W25N01GV",          0xef, 0xaa, 0x21},      \
    {"TC58CYG0S3HRAIJ",   0x98, 0xd2, 0x40},      \
}

/*
 * FLASH register mask info
 *
 * | name | capacity | ((SR_ADDR<<8)|SR-MASK)
 *
 *capacity:
 *      1: nand capacity is 1Gbit
 *      2: nand capacity is 2Gbit
 *      4: nand capacity is 4Gbit
 *      other.
 * Status Register addr and bit-MASK:
 *      (BP)          chip blk protect  bits mask
 *      (ECC-EN)      ecc enable        bit  mask
 *      (QE)          qspi enable       bit  mask
 *      (OIP/BUSY)    chip busy         bit  mask
 */
#define SPI_NAND_FLASH_CHIP_INFO                                           \
{                                                                          \
    {"W25N01GV",          1,  (NAND_SR1_ADDR<<8)|NAND_SR1_BP_BIT_MASK,     \
                              (NAND_SR2_ADDR<<8)|NAND_SR2_ECC_BIT_MASK,    \
                              (NAND_SR2_ADDR<<8)|NAND_SR2_QE_BIT_MASK,     \
                              (NAND_SR3_ADDR<<8)|NAND_SR3_BUSY_BIT_MASK},  \
    {"TC58CYG0S3HRAIJ",   1,  (NAND_SR1_ADDR<<8)|0x38,                     \
                              (NAND_SR2_ADDR<<8)|NAND_SR2_ECC_BIT_MASK,    \
                              (NAND_SR2_ADDR<<8)|NAND_SR2_QE_BIT_MASK,     \
                              (NAND_SR3_ADDR<<8)|NAND_SR3_BUSY_BIT_MASK},  \
}

#endif /* DRV_NAND_FLASH_H_ */





