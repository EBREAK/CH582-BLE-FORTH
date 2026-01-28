CROSS_COMPILE ?= riscv-none-elf-
CC = $(CROSS_COMPILE)gcc
OD = $(CROSS_COMPILE)objdump
OC = $(CROSS_COMPILE)objcopy
SZ = $(CROSS_COMPILE)size
DB = gdb

CH583_SDK ?= EVT/EXAM

#LINK_SCRIPT ?= $(CH583_SDK)/SRC/Ld/Link.ld
#LINK_SCRIPT ?= $(CH583_SDK)/BLE/OnlyUpdateApp_Peripheral/Ld/Link.ld
LINK_SCRIPT ?= Link.ld

#STARTUP_ASM ?= $(CH583_SDK)/SRC/Startup/startup_CH583.S
#STARTUP_ASM ?= $(CH583_SDK)/BLE/OnlyUpdateApp_Peripheral/Startup/startup_CH583.S
STARTUP_ASM ?= startup_CH583.S

CFLAGS += \
	-Wall -Wextra \
	-Wno-unused-variable \
	-Wno-unused-parameter \
	-Wno-pointer-to-int-cast \
	-Wno-discarded-qualifiers \
	-Os -ggdb \
	-march=rv32imac_zicsr_zifencei \
	-mabi=ilp32 \
	-pipe -nostartfiles \
	-Xlinker --gc-sections \
	-T $(LINK_SCRIPT) \
	--specs=nano.specs \

SRCS += \
	$(CH583_SDK)/SRC/RVMSIS/core_riscv.c \
	$(CH583_SDK)/SRC/StdPeriphDriver/CH58x_adc.c \
	$(CH583_SDK)/SRC/StdPeriphDriver/CH58x_clk.c \
	$(CH583_SDK)/SRC/StdPeriphDriver/CH58x_gpio.c \
	$(CH583_SDK)/SRC/StdPeriphDriver/CH58x_pwr.c \
	$(CH583_SDK)/SRC/StdPeriphDriver/CH58x_sys.c \
	$(CH583_SDK)/SRC/StdPeriphDriver/CH58x_uart1.c \
	$(CH583_SDK)/SRC/StdPeriphDriver/CH58x_usbdev.c \
	$(CH583_SDK)/BLE/HAL/SLEEP.c \
	$(CH583_SDK)/BLE/HAL/MCU.c \
	$(CH583_SDK)/BLE/HAL/RTC.c \

INCS += \
	-I $(CH583_SDK)/SRC/RVMSIS \
	-I $(CH583_SDK)/SRC/StdPeriphDriver/inc \
	-I $(CH583_SDK)/BLE/HAL/include \
	-I $(CH583_SDK)/BLE/LIB \

LIBS += \
	-L $(CH583_SDK)/SRC/StdPeriphDriver \
	-lISP583 \
	-L $(CH583_SDK)/BLE/LIB \
	#-lCH58xBLE \

SRCS += \
	app/main.c \
	app/debug.c \
	app/exception.c \
	app/kpram.c \
	app/crc.c \
	app/usbdev.c \
	app/fifo.c \
	app/forth.c \
	app/forth_dict.S \

INCS += \
	-I app/ \

CFLAGS += \
	-DDEBUG=1 \
	-DINT_SOFT \
	-DLIB_FLASH_BASE_ADDRESSS=0x00040000 \
	-DCH58xBLE_ROM=1 \
	-DBLE_MEMHEAP_SIZE=8192 \
	-DCENTRAL_MAX_CONNECTION=1 \
	-DPERIPHERAL_MAX_CONNECTION=1 \

elf: clean
	$(CC) $(CFLAGS) $(STARTUP_ASM) $(INCS) $(SRCS) $(LIBS) -o fw.elf
	$(OD) -d -s fw.elf > fw.dis
	$(OC) -O binary fw.elf fw.bin
	$(OC) -O ihex fw.elf fw.hex
	$(SZ) -G fw.elf

patch:
	sed -i -e 's/CH58xBLE_LIB.H/CH58xBLE_LIB.h/g' \
		$(CH583_SDK)/BLE/HAL/include/config.h
	sed -i -e 's/CONFIG.h/config.h/g' \
		$(CH583_SDK)/BLE/HAL/include/HAL.h
	sed -i -e 's/CONFIG.h/config.h/g' \
		$(CH583_SDK)/BLE/HAL/include/SLEEP.h
	sed -i -e 's/CH58xBLE_ROM.H/CH58xBLE_ROM.h/g' \
		$(CH583_SDK)/BLE/HAL/include/config.h
	sed -i -e 's/ptrdiff_t/int32_t/g' \
		$(CH583_SDK)/SRC/StdPeriphDriver/CH58x_sys.c 

ocd:
	openocd-wch -f wch-riscv.cfg

db:
	$(DB)

flash-ble-stack:
	wlink flash $(CH583_SDK)/BLE/LIB/CH58xBLE_ROMx.hex

flash:
	wlink flash fw.hex

clean:
	rm -vf *.elf *.bin *.out *.dis *.map *.hex
