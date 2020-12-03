from building import * 

# get current dir path
cwd = GetCurrentDir()

src = []
inc = [cwd]

src += ['drv_mtd_nand.c', 'drv_nand_qspi.c']

if GetDepend(['PKG_USING_SPI_NANDFLASH_SAMPLE']):
    src += ['nand_dev_samples.c']

group = DefineGroup('SPI-Nand', src, depend = ['PKG_USING_SPI_NANDFLASH'], CPPPATH = inc)
Return('group')
