/*
 * Microchip maXTouch Touchscreen driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Copyright (C) 2011-2014 Atmel Corporation
 * Copyright (C) 2012 Google, Inc.
 * Copyright (C) 2016 Zodiac Inflight Innovations
 *Read Info Block failed
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/* Note: Changes by Pitter.Liao@microchip.com Dlogic 336UD-HA project(marked with <DLm> Tag for platform dedicated only):
	v4.01 (2021/12/04):
	<1> Confilct SEQ Num variance in multi-thread process
	<2> State machinism optimization for interrupt handler
	<3> Finished the uncompleted flow chart for the config_update
	<4> Re-pack the reg read/write function to make code more readable
	v4.04 (2021/12/09):
	<1> Resync when Seqnum or CRC error in I2C communication with 3 methods to recovery.
	<2> Firmware and config updated from `/sys/` is support
	v4.05 (2021/12/13)
	<1> mxt_read_info_block() with retries
	<2> mxt_lookup_ha_chips() for detecting the `HA` chip with family and variant id.
	v4.06 (2021/1214)
	<1> adjust the mxt_reset() function with soft and hardware reset aligned
	<2> mxt_resync_comm() will do hardware reset if ID infomation is mis-matched
	<3> mxt_init_t7_power_cfg() will get value from former setting if it's zero
	<4> Firmware upadte flag removed only when interrupt is disabled to avoid confict the seq in interrupt handler
	<5> config update improve when config mismatch with firmware
	v4.07 (20211222)
	<1> `BTN_TOUCH` touch is default registerred in mxt_initialize_input_device() and use CONFIG_INPUT_DEVICE_SINGLE_TOUCH control single touch registeration
	<2> Re-wrote the __mxt_reset() to with flag parameter to distinguish soft and hard mode.
		Note the soft mode reset should diable irq first to avoid Seq Num synchonization issue. 
	<3> `crc_enabled` flag set and clear by T144 and T44 object
	<4> use CONFIG_ACPI to control mxt_input_open/close() registeration
	<5> use `u8` for  __mxt_update_seq_num() 2nd parameter, which could overun automatically
	<6> mxt_reset() timeout issue if flag mis-match 
	<7> use `INIT_COMPLETION` macro
	<8> Optimized the mxt_resycn_comm(), which should will be used in the ID information block
	v4.08 (20211223)
	<1> use gpio_set_value() instead of gpiod_set_value() to avoid compatible issue in transplat code in Kernel version < 4.0 
		and move gpio init operation to mxt_parse_gpio_properties()
	<2> use INIT_COMPLETION() macro for Kernel version compatible
	<3> use `use_retrigen_workaround` to call mxt_check_retriggen() when reset occurred
	<4> Fixed the issue resync wrong return when at first init failed
	<5> Re-write mxt_resycn_comm() caused by the issue infoblock checksum matched but the seq num might be incorrect
	v4.09 (20211224)
	<1> Fixe a critical issue there will be memory leaked in mxt_process_messages_t44_t144() of operating `msg_buf`
	<2> Use CONFIG_INPUT_DEVICE2_SINGLE_TOUCH to control whehter Input device 2 is a single touch device, remove CONFIG_INPUT_DEVICE_SINGLE_TOUCH Macro and will not register `BTN_TOUCH`
	v4.10 (20211229)
	<1> T15 support 2 instances preliminarily (test first instance)
	<2> T10/T25 selftest
	<3> T38 config version show
	<4> change `irq_processing` as atomic_t type varible, to limit the action of `debug_irq` node to be called by up level app （e.g. bridge client)
	<5> call mxt_remove() when failed at mxt_proble()
	<6> mxt_process_messages_t44_t144(): remove the first message reading process to make code more readable
	<7> update_fw will call extra and reset 
	v4.11 (20220107)
	<1> in mxt_resync_comm() set msleep(200) when i2c communication failed and remove it in <Check Point B.1> for speeding up
	<2> set `INPUT_PROP_DIRECT` in kernel less than 4.0, to make it's a touch panel instead of a tracking pad
	v4.12 (20220303)
	<1> Support MPTT (tiny3217) device
		<a> Remove `max_reportid` limited in mxt_read_and_process_messages() and mxt_process_messages_t44_t144()
		<b>	Object accessed with address check first (T6/T7/T18/T38)
		<c> T9/T15 xsize set to 1 if zero
		<d> Register device if T15 only existed
		<e> add `mptt` in i2c id list
	<2> Add `chg_gpio` description for un-standard kernel alternative IRQ 
	<3> Make mxt_resync_comm skip to step <0,1> for non-HA device for speeding up
	<4> Remove redundant mxt_init_t7_power_cfg() at the begin of mxt_configure_objects()
	v4.12a (20220421)
	<1> Temp patch for the bug whichs exist in mail branch, when update config file with uncontinous dynamical object, data will be chaos --- patch supplied by `Rocup.Wan@microchip.com`
	v4.12b (20220519)
	<1> Patch for improve the config update compitable. --- patch supplied by `Rocup.Wan@microchip.com`
	v4.12c (20220608)
	<1> Fixed a gpio config bug at using `chg_gpio` mode (kernel version < 4.0) --- bug found by `xcqiu@goworld-lcd.com`
	<2> Using hard code irqflags(IRQF_TRIGGER_LOW) if `chg_gpio` is assgined(kernel version >= 4.0)
	v4.12d (20220802)
	<1> issue of set irqflags to ZERO when using `chg_gpio`
	<2> Remove some warning of size_t, ssize_t, error
	v4.12e (20230418)
	<1> a corruption issue in mxt_clear_cfg() when bootup at bootloader mode fixed
	v4.12f (20230608)
	<1> Suport to set the keymap value of 0 for skipping the key reported
	v4.12g (20230803)
	<1> Added a lock for config/firmware updating to avoid the overlapping 
	<2> Default enable t6 message kernel print
	v4.12h (20240522)
	<1> POR reset check with CHG to speed up:
		<a> define CONFIG_MXT_POR_CHG_WAINTING_LEVEL
		<b> verify `chg_gpio` in device tree and valid level
	<2> Selftest messages are splitted with the situation of <POST, OnCommand, BIST>
		<a> T10 message is verified
	v4.12i (20240613)
	<1> Bring up the mxt_proc_t10_messages and mxt_proc_t25_messages before input device alloc, to avoid lost the information of POST message
	v4.12j (20240708)
	<1> Print chg_gpio status in `debug_irq` node
	<2> Added legacy t10 parser(336UD) with Pinfault group test code 8
	<3> Added mxt_reset_slots() in mxt_stop() to remove redundant points
	v6.0 (20241213)
	<1> kernel 6.x support
	
	Tested:
		<1> compatible with `non-HA` series --- tested in v4.10
		<2> compatible with `MPTT framework` --- tested in v4.12
		<3> resync checked:
			<a> Soft reset/Hard reset --- tested in v4.10
			<b> Issue T6 reset directly --- tested in v4.10
			<c> SCL and SDA short to Ground or each other --- tested in v4.10
			<d> Short I2C and bootup --- 4.10
			<e> Latch CHG and bootup and recover --- 4.10
			<e> lost seqnum at maxtouch side --- TBD
			<f> Sync without hard reset supported --- tested in 4.10
		<4> config update both from request_firmware_nowait() or echo from `/sys` --- tested in 4.10
		<5> firmware update with `HA` and `non-HA` chips --- `HA` is tested in v4.10, `no-HA` is tested in v4.10
		<6> T15 2 instances --- Worked with Instance 1(Not fully tested in maxtouch but `MPTT` works of v4.12)
*/

#define DRIVER_VERSION_NUMBER "6.0"

#include <linux/version.h>
#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/irq.h>	
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
#include <linux/gpio/consumer.h>
#else
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#endif
#include <asm/unaligned.h>
#ifdef CONFIG_TOUCHSCREEN_MICROCHIP_MXT_DEBUG
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#endif
#include <linux/string.h>

/* Firmware files */
#define MXT_FW_NAME		"maxtouch.fw"

/* Configuration file */
#define MXT_CFG_NAME		"maxtouch.cfg"
#define MXT_CFG_MAGIC		"OBP_RAW V1"

/* Registers */
#define MXT_OBJECT_START	0x07
#define MXT_OBJECT_SIZE		6
#define MXT_INFO_CHECKSUM_SIZE	3
#define MXT_MAX_BLOCK_WRITE	256

/* Objects */
#define MXT_GEN_MESSAGE_T5		5
#define MXT_GEN_COMMAND_T6		6
#define MXT_GEN_POWER_T7		7
#define MXT_GEN_ACQUIRE_T8		8
#define MXT_TOUCH_MULTI_T9		9
#define MXT_SPT_SELFTESTCONTROL_T10	10
#define MXT_SPT_SELFTESTPINFAULT_T11	11
#define MXT_SPT_SELFTESTSIGLIMIT_T12	12
#define MXT_PROCI_KEYTHRESHOLD_T14	14
#define MXT_TOUCH_KEYARRAY_T15		15
#define MXT_SPT_COMMSCONFIG_T18		18
#define MXT_SPT_GPIOPWM_T19		19
#define MXT_TOUCH_PROXIMITY_T23		23
#define MXT_TOUCH_PROXKEY_T52		52
#define MXT_PROCI_GRIPFACE_T20		20
#define MXT_PROCG_NOISE_T22		22
#define MXT_PROCI_ONETOUCH_T24		24
#define MXT_SPT_SELFTEST_T25		25
#define MXT_PROCI_TWOTOUCH_T27		27
#define MXT_SPT_CTECONFIG_T28		28
#define MXT_DEBUG_DIAGNOSTIC_T37	37
#define MXT_SPT_USERDATA_T38		38
#define MXT_PROCI_GRIP_T40		40
#define MXT_PROCI_PALM_T41		41
#define MXT_PROCI_TOUCHSUPPRESSION_T42	42
#define MXT_SPT_DIGITIZER_T43		43
#define MXT_SPT_MESSAGECOUNT_T44	44
#define MXT_SPT_CTECONFIG_T46		46
#define MXT_PROCI_STYLUS_T47		47
#define MXT_PROCG_NOISESUPPRESSION_T48	48
#define MXT_GEN_DATASOURCE_T53		53
#define MXT_PROCI_SHIELDLESS_T56	56
#define MXT_SPT_TIMER_T61		61
#define MXT_PROCI_LENSBENDING_T65	65
#define MXT_SPT_SERIALDATACOMMAND_T68	68
#define MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70 70
#define MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71  71
#define MXT_NOISESUPPRESSION_T72		   72
#define MXT_SPT_CTESCANCONFIG_T77		   77
#define MXT_PROCI_GLOVEDETECTION_T78		   78
#define MXT_SPT_TOUCHEVENTTRIGGER_T79		   79
#define MXT_PROCI_RETRANSMISSIONCOMPENSATION_T80   80
#define MXT_PROCG_NOISESUPACTIVESTYLUS_T86	   86
#define MXT_PROCI_SYMBOLGESTUREPROCESSOR_T92	   92
#define MXT_PROCI_TOUCHSEQUENCELOGGER_T93	   93
#define MXT_TOUCH_MULTITOUCHSCREEN_T100 	   100
#define MXT_SPT_AUXTOUCHCONFIG_T104		   104
#define MXT_PROCG_NOISESUPSELFCAP_T108		   108
#define MXT_SPT_SELFCAPGLOBALCONFIG_T109	   109
#define MXT_PROCI_ACTIVESTYLUS_T107		   107
#define MXT_SPT_CAPTUNINGPARAMS_T110		   110
#define MXT_SPT_SELFCAPCONFIG_T111		   111
#define MXT_PROCI_SELFCAPGRIPSUPPRESSION_T112	   112
#define MXT_SPT_PROXMEASURECONFIG_T113		   113
#define MXT_ACTIVESTYLUSMEASCONFIG_T114		   114
#define MXT_DATACONTAINER_T117					117
#define MXT_SPT_DATACONTAINERCTRL_T118		   118
#define MXT_PROCI_HOVERGESTUREPROCESSOR_T129	   129
#define MXT_SPT_SELCAPVOLTAGEMODE_T133		   133
#define PROCG_IGNORENODES_T141			   141
#define MXT_SPT_MESSAGECOUNT_T144		   144
#define MXT_SPT_IGNORENODESCONTROL_T145		   145

/* MXT_GEN_MESSAGE_T5 object */
#define MXT_RPTID_NOMSG		0xff
#define MXT_RPTID_RVSD		0x00

/* Message cache types */
enum {
	MESSAGE_CACHE_TEST_START,
	MESSAGE_CACHE_TEST_POST = MESSAGE_CACHE_TEST_START,
	MESSAGE_CACHE_TEST_ON_DEMAND,
	MESSAGE_CACHE_TEST_BIST,
	NUM_MESSAGE_CACHE_TYPES 
};

#define MESSAGE_CACHE(_cache_ptr, _each_cache_block_size, _cache_type) ((u8*)(_cache_ptr) + (_each_cache_block_size) * (_cache_type))
#define GET_MESSAGE_CACHE_DATA(_cache_ptr, _each_cache_block_size, _cache_type, _off) (MESSAGE_CACHE((_cache_ptr), (_each_cache_block_size), (_cache_type))[(_off)])
#define SET_MESSAGE_CACHE_DATA(_cache_ptr, _each_cache_block_size, _cache_type, _off, _val) (MESSAGE_CACHE((_cache_ptr), (_each_cache_block_size), (_cache_type))[(_off)] = (_val))
#define CLR_ONE_MESSAGE_CACHE(_cache_ptr, _each_cache_block_size, _cache_type) (memset(MESSAGE_CACHE((_cache_ptr), (_each_cache_block_size), (_cache_type)), 0, (_each_cache_block_size)))
#define CLR_ALL_MESSAGE_CACHE(_cache_ptr, _each_cache_block_size) (memset((_cache_ptr), 0, (_each_cache_block_size) * NUM_MESSAGE_CACHE_TYPES))

/* MXT_GEN_COMMAND_T6 field */
#define MXT_COMMAND_RESET	0
#define MXT_COMMAND_BACKUPNV	1
#define MXT_COMMAND_CALIBRATE	2
#define MXT_COMMAND_REPORTALL	3
#define MXT_COMMAND_DIAGNOSTIC	5

/* Define for T6 status byte */
#define MXT_T6_STATUS_RESET	BIT(7)
#define MXT_T6_STATUS_OFL	BIT(6)
#define MXT_T6_STATUS_SIGERR	BIT(5)
#define MXT_T6_STATUS_CAL	BIT(4)
#define MXT_T6_STATUS_CFGERR	BIT(3)
#define MXT_T6_STATUS_COMSERR	BIT(2)

/* MXT_GEN_POWER_T7 field */
struct t7_config {
	u8 idle;
	u8 active;
} __packed;

#define MXT_POWER_CFG_RUN		0
#define MXT_POWER_CFG_DEEPSLEEP		1

/* MXT_TOUCH_MULTI_T9 field */
#define MXT_T9_CTRL		0
#define MXT_T9_XSIZE		3
#define MXT_T9_YSIZE		4
#define MXT_T9_ORIENT		9
#define MXT_T9_RANGE		18

/* MXT_TOUCH_MULTI_T9 status */
#define MXT_T9_UNGRIP		BIT(0)
#define MXT_T9_SUPPRESS		BIT(1)
#define MXT_T9_AMP		BIT(2)
#define MXT_T9_VECTOR		BIT(3)
#define MXT_T9_MOVE		BIT(4)
#define MXT_T9_RELEASE		BIT(5)
#define MXT_T9_PRESS		BIT(6)
#define MXT_T9_DETECT		BIT(7)

struct t9_range {
	__le16 x;
	__le16 y;
} __packed;

/* MXT_TOUCH_MULTI_T9 orient */
#define MXT_T9_ORIENT_SWITCH	BIT(0)
#define MXT_T9_ORIENT_INVERTX	BIT(1)
#define MXT_T9_ORIENT_INVERTY	BIT(2)

/* MXT_SPT_SELFTESTCONTROL_T10 */
#define MXT_SELFTEST_T10_MESSAGE_INIT 0x0

#define MXT_SELFTEST_T10_MESSAGE_POST_PASS 0x11
#define MXT_SELFTEST_T10_MESSAGE_POST_FAILED 0x12

#define MXT_SELFTEST_T10_MESSAGE_BIST_PASS 0x21
#define MXT_SELFTEST_T10_MESSAGE_BIST_FAILED 0x22

#define MXT_SELFTEST_T10_MESSAGE_ON_DEMAND_PASS 0x31
#define MXT_SELFTEST_T10_MESSAGE_ON_DEMAND_FAILED 0x32

#define MXT_SELFTEST_T10_MESSAGE_TEST_CODE_INVALID 0x3F


#define MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_CLOCK 2
#define MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_FLASH 3
#define MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_RAM 4
#define MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_CTE 5
#define MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_SIGNAL_LIMIT 6
#define MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_POWER 7
#define MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_PIN_FAULT_LEGACY 8
#define MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_PIN_FAULT 9

/* MXT_TOUCH_KEYARRAY_T15 */
#define MXT_T15_CTRL		0
#define MXT_T15_XSIZE		3
#define MXT_T15_YSIZE		4

#define MXT_T15_ENABLE_BIT_MASK	0x01

/* MXT_SPT_COMMSCONFIG_T18 */
#define MXT_COMMS_CTRL		0
#define MXT_COMMS_CMD		1
#define MXT_COMMS_RETRIGEN	BIT(6)

/* MXT_SPT_SELFTEST_T25 */
#define MXT_SELFTEST_CTRL		0
#define MXT_SELFTEST_CMD		1

#define MXT_T25_MSG_STATUS 1

#define MXT_SELFTEST_T25_MESSAGE_TEST_INIT 0x0
#define MXT_SELFTEST_T25_MESSAGE_TEST_PASS 0xFE

#define MXT_SELFTEST_T25_MESSAGE_TEST_POWER_SUPPLY 0x01
#define MXT_SELFTEST_T25_MESSAGE_TEST_UNSUPPORT 0xFD
#define MXT_SELFTEST_T25_MESSAGE_TEST_PIN_FAULT 0x12
#define MXT_SELFTEST_T25_MESSAGE_TEST_OPEN_PIN_FAULT 0x14
#define MXT_SELFTEST_T25_MESSAGE_TEST_SIGNAL_LIMIT 0x17

/* MXT_DEBUG_DIAGNOSTIC_T37 */
#define MXT_DIAGNOSTIC_PAGEUP	0x01
#define MXT_DIAGNOSTIC_DELTAS	0x10
#define MXT_DIAGNOSTIC_REFS	0x11
#define MXT_DIAGNOSTIC_SIZE	128

#define MXT_FAMILY_1386			160
#define MXT1386_COLUMNS			3
#define MXT1386_PAGES_PER_COLUMN	8

struct t37_debug {
#ifdef CONFIG_TOUCHSCREEN_MICROCHIP_MXT_DEBUG
	u8 mode;
	u8 page;
	u8 data[MXT_DIAGNOSTIC_SIZE];
#endif
};

/* Define for MXT_GEN_COMMAND_T6 */
#define MXT_BOOT_VALUE		0xa5
#define MXT_RESET_VALUE		0x01
#define MXT_BACKUP_VALUE	0x55
#define MXT_BACKUP_W_STOP	0x33

/* Define for MXT_PROCI_TOUCHSUPPRESSION_T42 */
#define MXT_T42_MSG_TCHSUP	BIT(0)

/* T100 Multiple Touch Touchscreen */
#define MXT_T100_CTRL		0
#define MXT_T100_CFG1		1
#define MXT_T100_TCHAUX		3
#define MXT_T100_XSIZE		9
#define MXT_T100_XRANGE		13
#define MXT_T100_YSIZE		20
#define MXT_T100_YRANGE		24
#define MXT_T100_AUX_OFFSET	6
#define MXT_RSVD_RPTIDS		2
#define MXT_MIN_RPTID_SEC	18

#define MXT_T100_CFG_SWITCHXY	BIT(5)
#define MXT_T100_CFG_INVERTY	BIT(6)
#define MXT_T100_CFG_INVERTX	BIT(7)

#define MXT_T100_TCHAUX_VECT	BIT(0)
#define MXT_T100_TCHAUX_AMPL	BIT(1)
#define MXT_T100_TCHAUX_AREA	BIT(2)
#define MXT_T100_DETECT		BIT(7)

#define MXT_T100_TYPE_MASK		0x70
#define MXT_T100_ENABLE_BIT_MASK	0x01


enum t100_type {
	MXT_T100_TYPE_FINGER		= 1,
	MXT_T100_TYPE_PASSIVE_STYLUS	= 2,
	MXT_T100_TYPE_ACTIVE_STYLUS	= 3,
	MXT_T100_TYPE_HOVERING_FINGER	= 4,
	MXT_T100_TYPE_GLOVE		= 5,
	MXT_T100_TYPE_LARGE_TOUCH	= 6,
};

#define MXT_DISTANCE_ACTIVE_TOUCH	0
#define MXT_DISTANCE_HOVERING		1

#define MXT_TOUCH_MAJOR_DEFAULT		1
#define MXT_PRESSURE_DEFAULT		1

/* Gen2 Active Stylus */
#define MXT_T107_STYLUS_STYAUX		42
#define MXT_T107_STYLUS_STYAUX_PRESSURE	BIT(0)
#define MXT_T107_STYLUS_STYAUX_PEAK	BIT(4)

#define MXT_T107_STYLUS_HOVER		BIT(0)
#define MXT_T107_STYLUS_TIPSWITCH	BIT(1)
#define MXT_T107_STYLUS_BUTTON0		BIT(2)
#define MXT_T107_STYLUS_BUTTON1		BIT(3)

/* Delay times */
#define MXT_BACKUP_TIME		50	/* msec */
#define MXT_RESET_GPIO_TIME	20	/* msec */
#define MXT_RESET_INVALID_CHG	1000	/* msec */
#define MXT_RESET_TIME		200	/* msec */
#define MXT_RESET_CHG_CHECK_TIME	20	/* msec */
#define MXT_RESET_TIMEOUT	3000	/* msec */
#define MXT_CRC_TIMEOUT		1000	/* msec */
#define MXT_FW_FLASH_TIME	1000	/* msec */
#define MXT_FW_RESET_TIME	3000	/* msec */
#define MXT_FW_CHG_TIMEOUT	300	/* msec */
#define MXT_BOOTLOADER_WAIT	3000	/* msec */
#define MXT_SELFTEST_TIME	50	/* msec */

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB	0xaa
#define MXT_UNLOCK_CMD_LSB	0xdc

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define MXT_FRAME_CRC_CHECK	0x02
#define MXT_FRAME_CRC_FAIL	0x03
#define MXT_FRAME_CRC_PASS	0x04
#define MXT_APP_CRC_FAIL	0x40	/* valid 7 8 bit only */
#define MXT_BOOT_STATUS_MASK	0x3f
#define MXT_BOOT_EXTENDED_ID	BIT(5)
#define MXT_BOOT_ID_MASK	0x1f

/* Touchscreen absolute values */
#define MXT_MAX_AREA		0xff
#define MXT_PIXELS_PER_MM	20

/* Debug message size max */
#define DEBUG_MSG_MAX		200

struct mxt_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt_object {
	u8 type;
	u16 start_address;
	u8 size_minus_one;
	u8 instances_minus_one;
	u8 num_report_ids;
} __packed;

#ifdef CONFIG_TOUCHSCREEN_MICROCHIP_MXT_DEBUG
struct mxt_dbg {
	u16 t37_address;
	u16 diag_cmd_address;
	struct t37_debug *t37_buf;
	unsigned int t37_pages;
	unsigned int t37_nodes;

	struct v4l2_device v4l2;
	struct v4l2_pix_format format;
	struct video_device vdev;
	struct vb2_queue queue;
	struct mutex lock;
	int input;
};

enum v4l_dbg_inputs {
	MXT_V4L_INPUT_DELTAS,
	MXT_V4L_INPUT_REFS,
	MXT_V4L_INPUT_MAX,
};

static const struct v4l2_file_operations mxt_video_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.unlocked_ioctl = video_ioctl2,
	.read = vb2_fop_read,
	.mmap = vb2_fop_mmap,
	.poll = vb2_fop_poll,
};
#endif

enum mxt_suspend_mode {
	MXT_SUSPEND_DEEP_SLEEP	= 0,
	MXT_SUSPEND_T9_CTRL	= 1,
};

/* Config update context */
struct mxt_cfg {
	u8 *raw;
	size_t raw_size;
	off_t raw_pos;

	u8 *mem;
	size_t mem_size;
	int start_ofs;

	struct mxt_info info;
	u16 object_skipped_ofs;
};

/* Security content */
struct mxt_crc {
	u8 txseq_num;
};

/* Each client has this additional data */
struct mxt_data {
	struct i2c_client *client;
	/* Using I2C lock to avoid the conflict I2C bus access of Seq Num */
	struct mutex i2c_lock;
	struct input_dev *input_dev;
	struct input_dev *input_dev_sec;
	char phys[64];		/* device physical location */
	struct mxt_object *object_table;
	struct mxt_info *info;
	struct mxt_crc msg_num;
	void *raw_info_block;
	unsigned int irq;
	unsigned int max_x;
	unsigned int max_y;
	bool invertx;
	bool inverty;
	bool xy_switch;
	u8 xsize;
	u8 ysize;
	bool in_bootloader;
	u16 mem_size;
	u8 t100_aux_ampl;
	u8 t100_aux_area;
	u8 t100_aux_vect;
	struct bin_attribute mem_access_attr;
	bool crc_enabled;
	bool debug_enabled;
	bool debug_v2_enabled;
	u8 *debug_msg_data;
	u16 debug_msg_count;
	struct bin_attribute debug_msg_attr;
	struct mutex debug_msg_lock;
	u8 max_reportid;
	u32 config_crc;
	u32 info_crc;
	u8 bootloader_addr;
	/* message buffer for storing T5 message content readback */
	u8 *msg_buf;
	/* message cache for storing some message will be extra processed later */
	u8 *msg_cache;
	u8 t6_status;
	bool update_input;
	bool update_input_sec;
	u8 last_message_count;
	u8 num_touchids;
	u8 multitouch;
	struct t7_config t7_cfg;
	unsigned long t15_keystatus;
	u8 stylus_aux_pressure;
	u8 stylus_aux_peak;
	#ifdef CONFIG_TOUCHSCREEN_MICROCHIP_MXT_DEBUG
	struct mxt_dbg dbg;
	#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct gpio_desc *reset_gpio;
	struct gpio_desc *chg_gpio;
#else
	int reset_gpio;
	int chg_gpio;
#endif

	/* Cached parameters from object table */
	u16 T5_address;
	u8 T5_msg_size;
	u8 T6_reportid;
	u16 T6_address;
	u16 T7_address;
	u16 T14_address;
	u16 T38_address;
	u16 T71_address;
	u8 T9_reportid_min;
	u8 T9_reportid_max;
	u16 T10_address;
	u8 T10_reportid_min;
	u8 T15_reportid_min;
	u8 T15_reportid_max;
	u16 T18_address;
	u8 T19_reportid_min;
	u8 T24_reportid_min;
	u8 T24_reportid_max;
	u16 T25_address;
	u8 T25_reportid_min;
	u8 T27_reportid_min;
	u8 T27_reportid_max;
	u8 T42_reportid_min;
	u8 T42_reportid_max;
	u16 T44_address;
	u8 T46_reportid_min;	
	u8 T48_reportid_min;
	u8 T56_reportid_min;
	u8 T61_reportid_min;
	u8 T61_reportid_max;
	u8 T65_reportid_min;
	u8 T65_reportid_max;
	u8 T68_reportid_min;
	u8 T70_reportid_min;
	u8 T70_reportid_max;
	u8 T72_reportid_min;
	u8 T80_reportid_min;
	u8 T80_reportid_max;
	u16 T92_address;
	u8 T92_reportid_min;
	u16 T93_address;
	u8 T93_reportid_min;
	u8 T100_reportid_min;
	u8 T100_reportid_max;
	u16 T107_address;
	u8 T108_reportid_min;
	u8 T109_reportid_min;
	u8 T112_reportid_min;
	u8 T112_reportid_max;
	u8 T129_reportid_min;
	u8 T133_reportid_min;
	u16 T144_address;
	/* message count size of T44/144 */
	u8 msg_count_size;

	/* workaround of retrigen in T18 */
	bool use_retrigen_workaround;

	/* Cached instance parameter */
	u8 T100_instances;
	u8 T15_instances;

	/* for fw and config updating lock */
	struct mutex update_lock;

	/* for fw update in bootloader */
	struct completion bl_completion;

	/* for reset handling */
	struct completion reset_completion;

	/* for config update handling */
	struct completion crc_completion;

	u32 *t19_keymap;
	unsigned int t19_num_keys;

	u32 *t15_keymap;
	unsigned int t15_num_keys;
	unsigned int t15_num_keys_inst0;

	enum mxt_suspend_mode suspend_mode;

	/* use to track the IRQ status */
	atomic_t irq_processing;
	bool mxt_reset_state;

	/* Debugfs variables */
	struct dentry *debug_dir;

};
#ifdef CONFIG_TOUCHSCREEN_MICROCHIP_MXT_DEBUG
struct mxt_vb2_buffer {
	struct vb2_buffer	vb;
	struct list_head	list;
};
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	#define INIT_COMPLETION(_x)	reinit_completion(&(_x))
#endif

static size_t mxt_obj_size(const struct mxt_object *obj)
{
	return obj->size_minus_one + 1;
}

static size_t mxt_obj_instances(const struct mxt_object *obj)
{
	return obj->instances_minus_one + 1;
}

static bool mxt_object_readable(unsigned int type)
{
	switch (type) {
	case MXT_GEN_COMMAND_T6:
	case MXT_GEN_POWER_T7:
	case MXT_GEN_ACQUIRE_T8:
	case MXT_GEN_DATASOURCE_T53:
	case MXT_TOUCH_MULTI_T9:
	case MXT_SPT_SELFTESTCONTROL_T10:
	case MXT_SPT_SELFTESTPINFAULT_T11:
	case MXT_SPT_SELFTESTSIGLIMIT_T12:
	case MXT_TOUCH_KEYARRAY_T15:
	case MXT_TOUCH_PROXIMITY_T23:
	case MXT_TOUCH_PROXKEY_T52:
	case MXT_TOUCH_MULTITOUCHSCREEN_T100:
	case MXT_PROCI_GRIPFACE_T20:
	case MXT_PROCG_NOISE_T22:
	case MXT_PROCI_ONETOUCH_T24:
	case MXT_PROCI_TWOTOUCH_T27:
	case MXT_PROCI_GRIP_T40:
	case MXT_PROCI_PALM_T41:
	case MXT_PROCI_TOUCHSUPPRESSION_T42:
	case MXT_PROCI_STYLUS_T47:
	case MXT_PROCG_NOISESUPPRESSION_T48:
	case MXT_SPT_COMMSCONFIG_T18:
	case MXT_SPT_GPIOPWM_T19:
	case MXT_SPT_SELFTEST_T25:
	case MXT_SPT_CTECONFIG_T28:
	case MXT_SPT_USERDATA_T38:
	case MXT_SPT_DIGITIZER_T43:
	case MXT_SPT_CTECONFIG_T46:
	case MXT_PROCI_SHIELDLESS_T56:
	case MXT_SPT_TIMER_T61:
	case MXT_PROCI_LENSBENDING_T65:
	case MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70:
	case MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71:
	case MXT_NOISESUPPRESSION_T72:
	case MXT_PROCI_GLOVEDETECTION_T78:
	case MXT_SPT_TOUCHEVENTTRIGGER_T79:
	case MXT_PROCI_RETRANSMISSIONCOMPENSATION_T80:

		return true;
	default:
		return false;
	}
}

static void mxt_dump_message(struct mxt_data *data, u8 *message)
{
	dev_info(&data->client->dev, "message: %*ph\n",
		data->T5_msg_size, message);
}

static void mxt_debug_msg_enable(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;

	if (data->debug_v2_enabled)
		return;

	mutex_lock(&data->debug_msg_lock);

	data->debug_msg_data = kcalloc(DEBUG_MSG_MAX,
				data->T5_msg_size, GFP_KERNEL);
	if (!data->debug_msg_data)
		return;

	data->debug_v2_enabled = true;
	mutex_unlock(&data->debug_msg_lock);

	dev_dbg(dev, "Enabled message output\n");
}

static void mxt_debug_msg_disable(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;

	if (!data->debug_v2_enabled)
		return;

	data->debug_v2_enabled = false;

	mutex_lock(&data->debug_msg_lock);
	kfree(data->debug_msg_data);
	data->debug_msg_data = NULL;
	data->debug_msg_count = 0;
	mutex_unlock(&data->debug_msg_lock);
	dev_dbg(dev, "Disabled message output\n");
}

static void mxt_debug_msg_add(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;

	mutex_lock(&data->debug_msg_lock);

	if (!data->debug_msg_data) {
		dev_err(dev, "No buffer!\n");
		return;
	}

	if (data->debug_msg_count < DEBUG_MSG_MAX) {
		memcpy(data->debug_msg_data +
		       data->debug_msg_count * data->T5_msg_size,
		       msg,
		       data->T5_msg_size);
		data->debug_msg_count++;
	} else {
		dev_dbg(dev, "Discarding %u messages\n", data->debug_msg_count);
		data->debug_msg_count = 0;
	}

	mutex_unlock(&data->debug_msg_lock);

	sysfs_notify(&data->client->dev.kobj, NULL, "debug_notify");
}

static ssize_t mxt_debug_msg_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off,
	size_t count)
{
	return -EIO;
}

static ssize_t mxt_debug_msg_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t bytes)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int count;
	size_t bytes_read;

	if (!data->debug_msg_data) {
		dev_err(dev, "No buffer!\n");
		return 0;
	}

	count = bytes / data->T5_msg_size;

	if (count > DEBUG_MSG_MAX)
		count = DEBUG_MSG_MAX;

	mutex_lock(&data->debug_msg_lock);

	if (count > data->debug_msg_count)
		count = data->debug_msg_count;

	bytes_read = count * data->T5_msg_size;

	memcpy(buf, data->debug_msg_data, bytes_read);
	data->debug_msg_count = 0;

	mutex_unlock(&data->debug_msg_lock);

	return bytes_read;
}

static int mxt_debug_msg_init(struct mxt_data *data)
{
	sysfs_bin_attr_init(&data->debug_msg_attr);
	data->debug_msg_attr.attr.name = "debug_msg";
	data->debug_msg_attr.attr.mode = 0666;
	data->debug_msg_attr.read = mxt_debug_msg_read;
	data->debug_msg_attr.write = mxt_debug_msg_write;
	data->debug_msg_attr.size = data->T5_msg_size * DEBUG_MSG_MAX;

	if (sysfs_create_bin_file(&data->client->dev.kobj,
				  &data->debug_msg_attr) < 0) {
		dev_err(&data->client->dev, "Failed to create %s\n",
			data->debug_msg_attr.attr.name);
		return -EINVAL;
	}

	return 0;
}

static void mxt_debug_msg_remove(struct mxt_data *data)
{
	if (data->debug_msg_attr.attr.name) {
		sysfs_remove_bin_file(&data->client->dev.kobj,
				      &data->debug_msg_attr);
	}
}

static int mxt_wait_for_completion(struct mxt_data *data,
				   struct completion *comp,
				   unsigned int timeout_ms)
{
	struct device *dev = &data->client->dev;
	unsigned long timeout = msecs_to_jiffies(timeout_ms);
	long ret;

	ret = wait_for_completion_interruptible_timeout(comp, timeout);
	if (ret < 0) {
		return ret;
	} else if (ret == 0) {
		dev_err(dev, "Wait for completion timed out.\n");
		return -ETIMEDOUT;
	}
	return 0;
}

static u8 mxt_curr_seq_num(struct mxt_data *data)
{
	return data->msg_num.txseq_num;
}

static u8 __mxt_update_seq_num(struct mxt_data *data, bool reset_counter, u8 counter_value)
{
	u8 current_val;

	current_val = data->msg_num.txseq_num;

	if (reset_counter) {
		dev_info(&data->client->dev,
			"<Seqnum>: %d -> %d\n", data->msg_num.txseq_num, counter_value);

		data->msg_num.txseq_num = counter_value;
	} else {
		// u8 could be overrun when it reaches 0xff
		data->msg_num.txseq_num++;
	}

	return current_val;
}

static u8 mxt_update_seq_num_lock(struct mxt_data *data, bool reset_counter, u8 counter_value)
{
	u8 val;

	mutex_lock(&data->i2c_lock);

	val = __mxt_update_seq_num(data, reset_counter, counter_value);

	mutex_unlock(&data->i2c_lock);	
	
	return val;
}

static bool mxt_lookup_ha_chips(const struct mxt_info *info)
{
	u8 family_id;
	u8 variant_id;
	bool is_ha = false;
	
	if (!info) {
		return false;
	}
	
	family_id = info->family_id;
	variant_id = info->variant_id;
	
	switch (family_id) {
		case 0xA6:
			if (variant_id == 0x14) {
				// "336UD-HA"
				is_ha = true;
			}
		break;
		default:
			;
	}
	return is_ha;
}


static int mxt_bootloader_read(struct mxt_data *data,
			       u8 *val, unsigned int count)
{
	int ret;
	struct i2c_msg msg;

	msg.addr = data->bootloader_addr;
	msg.flags = data->client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = val;

	ret = i2c_transfer(data->client->adapter, &msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&data->client->dev, "%s: i2c recv failed (%d)\n",
			__func__, ret);
	}

	return ret;
}

static int mxt_bootloader_write(struct mxt_data *data,
				const u8 * const val, unsigned int count)
{
	int ret;
	struct i2c_msg msg;
	u8 *data_buf;

	data_buf = kmalloc(count, GFP_KERNEL);
	if (!data_buf)
		return -ENOMEM;
	
	memcpy(&data_buf[0], val, count);

	msg.addr = data->bootloader_addr;
	msg.flags = data->client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = data_buf;

	ret = i2c_transfer(data->client->adapter, &msg, 1);

	if (ret == 1) {
		ret = 0;
	} else {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&data->client->dev, "%s: i2c send failed (%d)\n",
			__func__, ret);
	}

	return ret;
}

static int mxt_lookup_bootloader_address(struct mxt_data *data, bool retry)
{
	u8 appmode = data->client->addr;
	u8 bootloader;
	u8 family_id = data->info ? data->info->family_id : 0;

	switch (appmode) {
	case 0x4a:
	case 0x4b:
		/* Chips after 1664S use different scheme */
		if (retry || family_id >= 0xa2) {
			bootloader = appmode - 0x24;
			break;
		}
		/* Fall through for normal case */
		/* fall through */
	case 0x4c:
	case 0x4d:
	case 0x5a:
	case 0x5b:
		bootloader = appmode - 0x26;
		break;

	default:
		dev_err(&data->client->dev,
			"Appmode i2c address 0x%02x not found\n",
			appmode);
		return -EINVAL;
	}

	data->bootloader_addr = bootloader;

	dev_info(&data->client->dev, "Bootloader address: %x\n", bootloader);

	return 0;
}

static int mxt_probe_bootloader(struct mxt_data *data, bool alt_address)
{
	struct device *dev = &data->client->dev;
	int error;
	u8 val;
	bool crc_failure;

	error = mxt_lookup_bootloader_address(data, alt_address);
	if (error)
		return error;

	error = mxt_bootloader_read(data, &val, 1);
	if (error)
		return error;

	/* Check app crc fail mode */
	crc_failure = (val & ~MXT_BOOT_STATUS_MASK) == MXT_APP_CRC_FAIL;

	dev_err(dev, "Detected bootloader, status:%02X%s\n",
			val, crc_failure ? ", APP_CRC_FAIL" : "");

	return 0;
}

static u8 mxt_get_bootloader_version(struct mxt_data *data, u8 val)
{
	struct device *dev = &data->client->dev;
	u8 buf[3];

	if (val & MXT_BOOT_EXTENDED_ID) {
		if (mxt_bootloader_read(data, &buf[0], 3) != 0) {
			dev_err(dev, "%s: i2c failure\n", __func__);
			return val;
		}

		dev_dbg(dev, "Bootloader ID:%d Version:%d\n", buf[1], buf[2]);

		return buf[0];
	} else {
		dev_dbg(dev, "Bootloader ID:%d\n", val & MXT_BOOT_ID_MASK);

		return val;
	}
}

static int mxt_check_bootloader(struct mxt_data *data, unsigned int state,
				bool wait)
{
	struct device *dev = &data->client->dev;
	u8 val;
	int ret;

recheck:
	if (wait) {
		/*
		 * In application update mode, the interrupt
		 * line signals state transitions. We must wait for the
		 * CHG assertion before reading the status byte.
		 * Once the status byte has been read, the line is deasserted.
		 */
		ret = mxt_wait_for_completion(data, &data->bl_completion,
					      MXT_FW_CHG_TIMEOUT);
		if (ret) {
			/*
			 * TODO: handle -ERESTARTSYS better by terminating
			 * fw update process before returning to userspace
			 * by writing length 0x000 to device (iff we are in
			 * WAITING_FRAME_DATA state).
			 */
			dev_err(dev, "Update wait error %d\n", ret);
			return ret;
		}
	}

	ret = mxt_bootloader_read(data, &val, 1);
	if (ret)
		return ret;

	if (state == MXT_WAITING_BOOTLOAD_CMD)
		val = mxt_get_bootloader_version(data, val);

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
	case MXT_WAITING_FRAME_DATA:
	case MXT_APP_CRC_FAIL:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK) {
			goto recheck;
		} else if (val == MXT_FRAME_CRC_FAIL) {
			dev_err(dev, "Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(dev, "Invalid bootloader state %02X != %02X\n",
			val, state);
		return -EINVAL;
	}

	return 0;
}

static int mxt_send_bootloader_cmd(struct mxt_data *data, bool unlock)
{
	int ret;
	u8 buf[2];

	if (unlock) {
		buf[0] = MXT_UNLOCK_CMD_LSB;
		buf[1] = MXT_UNLOCK_CMD_MSB;
	} else {
		buf[0] = 0x01;
		buf[1] = 0x01;
	}

	ret = mxt_bootloader_write(data, buf, 2);
	if (ret)
		return ret;

	return 0;
}

static u8 __mxt_calc_crc8(unsigned char crc, unsigned char data)
{

	static const u8 crcpoly = 0x8C;
	u8 index;
	u8 fb;
	index = 8;
		
	do {
		fb = (crc ^ data) & 0x01;
		data >>= 1;
		crc >>= 1;
		if (fb)
			crc ^= crcpoly;
	} while (--index);
	
	return crc;
}
	

static int __mxt_read_reg(struct i2c_client *client,
			       u16 reg, u16 len, void *val)
{
	struct i2c_msg xfer[2];
	u8 buf[2];
	int ret;

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;

	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = buf;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = len;
	xfer[1].buf = val;

	ret = i2c_transfer(client->adapter, xfer, 2);
	if (ret == 2) {
		ret = 0;
	} else {
		if (ret >= 0)
			ret = -EIO;
		dev_err(&client->dev, "%s: i2c transfer failed (%d)\n",
			__func__, ret);
	}

	return ret;
}

static int mxt_resync_comm(struct mxt_data *data);

#define F_R_SEQ BIT(0)
#define F_R_CRC BIT(1)

static int __mxt_read_reg_crc(struct i2c_client *client,
			       u16 reg, u16 len, void *val, struct mxt_data *data, u8 flag)
{
	u8 *buf;
	size_t count;
	u8 crc_data = 0;
	char *ptr_data;
	int ret = 0;
	int i;

	count = 4;	//16bit addr, tx_seq_num, 8bit crc

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	
	mutex_lock(&data->i2c_lock);

	if (flag & F_R_SEQ) {
		buf[0] = reg & 0xff;
		buf[1] = ((reg >> 8) & 0xff);
		buf[2] = __mxt_update_seq_num(data, false, 0x00);

		for (i = 0; i < (count-1); i++) {
			crc_data = __mxt_calc_crc8(crc_data, buf[i]);
			dev_dbg(&client->dev, "Write: Data = [%x], crc8 =  %x\n", buf[i], crc_data);
		}

		buf[3] = crc_data;

		ret = i2c_master_send(client, buf, count);

		if (ret == count) {
			ret = 0;
		} else {
			ret = -EIO;

			dev_err(&client->dev, "%s: i2c send failed (%d) reg (0x%04x) len (%d)\n",
				__func__, ret, reg, len);
			/* If the write reg address failed, that means slave may not get the packet, reverse the seq num 
				Note this is not granteed operation, that meanings seq num might mis-matched
			*/ 
			__mxt_update_seq_num(data, true, buf[2]);
			goto end_reg_read;
		}
	}

	//Read data and check 8bit CRC
	ret = i2c_master_recv(client, val, len);
	if (ret == len) {		
		ptr_data = val;

		if (flag & F_R_CRC) {

			crc_data = 0;

			for (i = 0; i < (len - 1); i++){
				crc_data = __mxt_calc_crc8(crc_data, ptr_data[i]);
				dev_dbg(&client->dev, "Read: Data = [%x], crc8 =  %x\n", ((char *) ptr_data)[i], crc_data);
			}

			if (crc_data == ptr_data[len - 1]) {
				dev_dbg(&client->dev, "Read CRC Passed\n");
				ret = 0;
				goto end_reg_read;
			} else {
				dev_err(&client->dev, "Read CRC Failed at seq_num[%d] crc8 = %02x, calculated = %02x\n", 
					buf[2], ptr_data[len - 1], crc_data);
				dev_info(&client->dev, "reg (0x%04x) data(%d): %*ph\n",
					reg, len, len, ptr_data);
				ret = -EINVAL;
				goto end_reg_read;
			}	
		}

		ret = 0;	/* Needed for crc8 = false */

	} else {
			ret = -EIO;
			dev_err(&client->dev, "%s: i2c receive failed (%d) reg (0x%04x) len (%d)\n",
				__func__, ret, reg, len);
	}

end_reg_read:
	mutex_unlock(&data->i2c_lock);

	kfree(buf);
	return ret;
}

static int mxt_read_reg_auto(struct i2c_client *client,
			       u16 reg, u16 len, void *val, struct mxt_data *data)
{
	u8 flag = 0;

	if (data->crc_enabled) {
		flag = F_R_SEQ;
		if (reg == data->T5_address || reg == data->T144_address) {
			flag |= F_R_CRC;
		}

		return __mxt_read_reg_crc(client, reg, len, val, data, flag);
	} else {
		return __mxt_read_reg(client, reg, len, val);
	}
}

static int __mxt_write_reg(struct i2c_client *client, u16 reg, u16 len,
			   const void *val)
{
	u8 *buf;
	size_t count;
	int ret;

	count = len + 2;
	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;
	memcpy(&buf[2], val, len);

	ret = i2c_master_send(client, buf, count);
	if (ret == count) {
		ret = 0;
	} else {
		if (ret >= 0)
			ret = -EIO;
		dev_err(&client->dev, "%s: i2c send failed (%d)\n",
			__func__, ret);
	}

	kfree(buf);
	return ret;
}

static int __mxt_write_reg_crc(struct i2c_client *client, u16 reg, u16 length,
			   const void *val, struct mxt_data *data)
{
	u8 msgbuf[15];
	u8 *databuf;
	size_t msg_count;
	size_t len;
	int ret;
	u8 crc_data = 0;
	int i,j;
	u16 retry_counter = 0;
	u16 bytesToWrite = 0;
	u16 write_addr = 0;
	u16 bytesWritten = 0;
	u8 max_data_length = 11;
	volatile u16 message_length = 0;

	len = length + 2;
	bytesToWrite = length;

	//Limit size of data packet
	if (length > max_data_length){
		message_length = 11;
	} else {
		message_length = length;	
	}

	msg_count = message_length + 4;

	//Allocate memory for full length message
	databuf = kmalloc(len, GFP_KERNEL);

	if (!databuf)
		return -ENOMEM;

	mutex_lock(&data->i2c_lock);

	if (!(length == 0x00)) {	//Need this or else memory crash
		memcpy(&databuf[0], val, length);	//Copy only first message to databuf
	}

	do {
		write_addr = reg + bytesWritten;

		msgbuf[0] = write_addr & 0xff;
		msgbuf[1] = (write_addr >> 8) & 0xff;
		msgbuf[msg_count-2] = __mxt_update_seq_num(data, false, 0x00);

		j = 0;

		while (j < message_length) {
			//Copy current messasge into msgbuffer
			msgbuf[2 + j] = databuf[bytesWritten + j];	
			j++; 
		}

		crc_data = 0;

		for (i = 0; i < (msg_count-1); i++) {
			crc_data = __mxt_calc_crc8(crc_data, msgbuf[i]);

			dev_dbg(&client->dev, "Write CRC: Data[%d] = %x, crc = 0x%x\n",
				i, msgbuf[i], crc_data);
		}
	
		msgbuf[msg_count-1] = crc_data;

		ret = i2c_master_send(client, msgbuf, msg_count);
	
		if (ret == msg_count) {
			ret = 0;
			bytesWritten = bytesWritten + message_length;	//Track only bytes in buf
			bytesToWrite = bytesToWrite - message_length;

			//dev_info(&client->dev, "bytesWritten %i, bytesToWrite %i", bytesWritten, bytesToWrite);
			retry_counter = 0;

			if (bytesToWrite < message_length){
				message_length = bytesToWrite;
				msg_count = message_length + 4;
			}
		} else {
				ret = -EIO;
				dev_err(&client->dev, "%s: i2c send failed (%d)\n",
					__func__, ret);
				/* If the i2c-write failed, that means slave may not get the packet, reverse the seq num 
					Note this is not granteed operation, that meanings seq num might mis-matched
				*/ 
				__mxt_update_seq_num(data, true, msgbuf[msg_count-2]);
		}

		retry_counter++;

		if (retry_counter == 10)
			break;

	} while (bytesToWrite > 0);
	
	mutex_unlock(&data->i2c_lock);
	
	kfree(databuf);	

	return ret;
}

static int mxt_write_reg_auto(struct i2c_client *client, u16 reg, u16 length,
			   const void *val, struct mxt_data *data)
{
	if (data->crc_enabled) {
		return __mxt_write_reg_crc(client, reg, length, val, data);
	} else {
		return __mxt_write_reg(client, reg, length, val);
	}
}

static int mxt_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	return __mxt_write_reg(client, reg, 1, &val);
}

static struct mxt_object *mxt_get_object(struct mxt_data *data, u8 type)
{
	struct mxt_object *object;
	int i;

	for (i = 0; i < data->info->object_num; i++) {
		object = data->object_table + i;
		if (object->type == type)
			return object;
	}

	dev_warn(&data->client->dev, "Invalid object type T%u\n", type);
	return NULL;
}

static int mxt_check_retrigen(struct mxt_data *data);

static void mxt_proc_t6_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];
	u32 crc = msg[2] | (msg[3] << 8) | (msg[4] << 16);

	if (crc != data->config_crc) {
		dev_info(dev, "T6 Config Checksum: 0x%06X\n", crc);
		if (crc) {
			// Fixme: some T6 messages haven't got CRC information?
			data->config_crc = crc;
		}
	}

	complete(&data->crc_completion);

	/* Detect reset */
	if (status & MXT_T6_STATUS_RESET) {
		// recheck the retrign workaround
		if (data->use_retrigen_workaround) {
			mxt_check_retrigen(data);
		}

		// Clear message cache when reset occured
		CLR_ALL_MESSAGE_CACHE(data->msg_cache, data->T5_msg_size);

		complete(&data->reset_completion);
	}

	/* Output debug if status has changed */
	if (status != data->t6_status)
		dev_info(dev, "T6 Status 0x%02X%s%s%s%s%s%s%s\n",
			status,
			status == 0 ? " OK" : "",
			status & MXT_T6_STATUS_RESET ? " RESET" : "",
			status & MXT_T6_STATUS_OFL ? " OFL" : "",
			status & MXT_T6_STATUS_SIGERR ? " SIGERR" : "",
			status & MXT_T6_STATUS_CAL ? " CAL" : "",
			status & MXT_T6_STATUS_CFGERR ? " CFGERR" : "",
			status & MXT_T6_STATUS_COMSERR ? " COMSERR" : "");

	if (status & MXT_T6_STATUS_COMSERR) {
		dev_err(dev, "T6 COMSERR Error found, Seqnum(%d)\n", mxt_curr_seq_num(data));
	}

	/* Save current status */
	data->t6_status = status;
}

static int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object || offset >= mxt_obj_size(object))
		return -EINVAL;

	reg = object->start_address;
	return mxt_write_reg(data->client, reg + offset, val);
}

static void mxt_input_button(struct mxt_data *data, u8 *message)
{
	struct input_dev *input = data->input_dev;
	int i;

	for (i = 0; i < data->t19_num_keys; i++) {
		if (data->t19_keymap[i] == KEY_RESERVED)
			continue;

		/* Active-low switch */
		input_report_key(input, data->t19_keymap[i],
				 !(message[1] & BIT(i)));
	}
}

static void mxt_input_sync(struct mxt_data *data)
{
	input_mt_report_pointer_emulation(data->input_dev,
					  data->t19_num_keys);
	
	if (data->update_input) 
		input_sync(data->input_dev);
	
	if (data->update_input_sec)
		input_sync(data->input_dev_sec);
	
}

static void mxt_proc_t9_message(struct mxt_data *data, u8 *message)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev = data->input_dev;
	int id;
	u8 status;
	int x;
	int y;
	int area;
	int amplitude;
	int tool;

	id = message[0] - data->T9_reportid_min;
	status = message[1];
	x = (message[2] << 4) | ((message[4] >> 4) & 0xf);
	y = (message[3] << 4) | ((message[4] & 0xf));

	/* Handle 10/12 bit switching */
	if (data->max_x < 1024)
		x >>= 2;
	if (data->max_y < 1024)
		y >>= 2;

	area = message[5];
	amplitude = message[6];

	dev_dbg(dev,
		"[%u] %c%c%c%c%c%c%c%c x: %5u y: %5u area: %3u amp: %3u\n",
		id,
		(status & MXT_T9_DETECT) ? 'D' : '.',
		(status & MXT_T9_PRESS) ? 'P' : '.',
		(status & MXT_T9_RELEASE) ? 'R' : '.',
		(status & MXT_T9_MOVE) ? 'M' : '.',
		(status & MXT_T9_VECTOR) ? 'V' : '.',
		(status & MXT_T9_AMP) ? 'A' : '.',
		(status & MXT_T9_SUPPRESS) ? 'S' : '.',
		(status & MXT_T9_UNGRIP) ? 'U' : '.',
		x, y, area, amplitude);

	input_mt_slot(input_dev, id);

	if (status & MXT_T9_DETECT) {
		/*
		 * Multiple bits may be set if the host is slow to read
		 * the status messages, indicating all the events that
		 * have happened.
		 */
		if (status & MXT_T9_RELEASE) {
			input_mt_report_slot_state(input_dev,
						   MT_TOOL_FINGER, 0);
			mxt_input_sync(data);
		}

		/* A size of zero indicates touch is from a linked T47 Stylus */
		if (area == 0) {
			area = MXT_TOUCH_MAJOR_DEFAULT;
			tool = MT_TOOL_PEN;
		} else {
			tool = MT_TOOL_FINGER;
		}

		/* if active, pressure must be non-zero */
		if (!amplitude)
			amplitude = MXT_PRESSURE_DEFAULT;

		/* Touch active */
		input_mt_report_slot_state(input_dev, tool, 1);
		input_report_abs(input_dev, ABS_MT_POSITION_X, x);
		input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
		input_report_abs(input_dev, ABS_MT_PRESSURE, amplitude);
		input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, area);
	} else {
		/* Touch no longer active, close out slot */
		input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 0);
	}

	data->update_input = true;
}

static void mxt_proc_t100_message(struct mxt_data *data, u8 *message)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev = data->input_dev;
	struct input_dev *input_dev_sec = data->input_dev_sec;
	int id = 0;
	int id_sec = 0;
	u8 status;
	u8 type = 0;
	u16 x;
	u16 y;
	int distance = 0;
	int tool = 0;
	u8 major = 0;
	u8 pressure = 0;
	u8 orientation = 0;
	bool hover = false;
	
	/* Determine id of touch messages only */
	id = (message[0] - data->T100_reportid_min - MXT_RSVD_RPTIDS);
	
	if (id >= MXT_MIN_RPTID_SEC) {
		id_sec = (message[0] - data->T100_reportid_min - MXT_MIN_RPTID_SEC - 
			MXT_RSVD_RPTIDS);
	}

	/* Skip SCRSTATUS events */
	if (id < 0 || id_sec < 0)
		return;

	status = message[1];
	x = get_unaligned_le16(&message[2]);
	y = get_unaligned_le16(&message[4]);
	
	/* Get other auxdata[] bytes if present */
	
	if (status & MXT_T100_DETECT) {
		type = (status & MXT_T100_TYPE_MASK) >> 4;

		switch (type) {
		case MXT_T100_TYPE_HOVERING_FINGER:
			tool = MT_TOOL_FINGER;
			distance = MXT_DISTANCE_HOVERING;
			hover = true;

			if (data->t100_aux_vect)
				orientation = message[data->t100_aux_vect];

			break;

		case MXT_T100_TYPE_FINGER:
		case MXT_T100_TYPE_GLOVE:
			tool = MT_TOOL_FINGER;
			distance = MXT_DISTANCE_ACTIVE_TOUCH;
			hover = false;

			if (data->t100_aux_area)
				major = message[data->t100_aux_area];

			if (data->t100_aux_ampl)
				pressure = message[data->t100_aux_ampl];

			if (data->t100_aux_vect)
				orientation = message[data->t100_aux_vect];

			break;

		case MXT_T100_TYPE_PASSIVE_STYLUS:
			tool = MT_TOOL_PEN;
			distance = MXT_DISTANCE_ACTIVE_TOUCH;
			hover = false;

			/*
			 * Passive stylus is reported with size zero so
			 * hardcode.
			 */
			major = MXT_TOUCH_MAJOR_DEFAULT;

			if (data->t100_aux_ampl)
				pressure = message[data->t100_aux_ampl];

			break;

		case MXT_T100_TYPE_ACTIVE_STYLUS:
			/* Report input buttons */
			input_report_key(input_dev, BTN_STYLUS,
					 message[6] & MXT_T107_STYLUS_BUTTON0);
			input_report_key(input_dev, BTN_STYLUS2,
					 message[6] & MXT_T107_STYLUS_BUTTON1);

			/* stylus in range, but position unavailable */
			if (!(message[6] & MXT_T107_STYLUS_HOVER))
				break;

			tool = MT_TOOL_PEN;
			distance = MXT_DISTANCE_ACTIVE_TOUCH;
			major = MXT_TOUCH_MAJOR_DEFAULT;

			if (!(message[6] & MXT_T107_STYLUS_TIPSWITCH)) {
				hover = true;
				distance = MXT_DISTANCE_HOVERING;
			} else if (data->stylus_aux_pressure) {
				pressure = message[data->stylus_aux_pressure];
			}

			break;

		case MXT_T100_TYPE_LARGE_TOUCH:
			/* Ignore suppressed touch */
			break;

		default:
			dev_dbg(dev, "Unexpected T100 type\n");
			return;
		}
	}

	/*
	 * Values reported should be non-zero if tool is touching the
	 * device
	 */
	if (!pressure && !hover)
		pressure = MXT_PRESSURE_DEFAULT;

	if (id >= MXT_MIN_RPTID_SEC) {
		input_mt_slot(input_dev_sec, id_sec);
	} else {
		input_mt_slot(input_dev, id);
	}
		
	if (status & MXT_T100_DETECT) {
		
		if (id >= MXT_MIN_RPTID_SEC) {
			dev_dbg(dev, "id:[%u] type:%u x:%u y:%u a:%02X p:%02X v:%02X\n",
			id_sec, type, x, y, major, pressure, orientation);
		} else {
			dev_dbg(dev, "id:[%u] type:%u x:%u y:%u a:%02X p:%02X v:%02X\n",
			id, type, x, y, major, pressure, orientation);
		}
		
		if (id >= MXT_MIN_RPTID_SEC) {	
			input_mt_report_slot_state(input_dev_sec, tool, 1);
		  	input_report_abs(input_dev_sec, ABS_MT_POSITION_X, x);
		  	input_report_abs(input_dev_sec, ABS_MT_POSITION_Y, y);
		  	input_report_abs(input_dev_sec, ABS_MT_TOUCH_MAJOR, major);
		  	input_report_abs(input_dev_sec, ABS_MT_PRESSURE, pressure);
		  	input_report_abs(input_dev_sec, ABS_MT_DISTANCE, distance);
		  	input_report_abs(input_dev_sec, ABS_MT_ORIENTATION, orientation);

#ifdef CONFIG_INPUT_DEVICE2_SINGLE_TOUCH
			if (id == MXT_MIN_RPTID_SEC) {
				input_report_abs(input_dev_sec, ABS_X, x);
				input_report_abs(input_dev_sec, ABS_Y, y);
				input_report_key(input_dev_sec, BTN_TOUCH, 1);
			}
#endif
		} else {
		  	input_mt_report_slot_state(input_dev, tool, 1);
		  	input_report_abs(input_dev, ABS_MT_POSITION_X, x);
		  	input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
		  	input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, major);
		  	input_report_abs(input_dev, ABS_MT_PRESSURE, pressure);
		  	input_report_abs(input_dev, ABS_MT_DISTANCE, distance);
		  	input_report_abs(input_dev, ABS_MT_ORIENTATION, orientation);
		}
	} else {
		 
		if (id >= MXT_MIN_RPTID_SEC) {
			dev_dbg(dev, "[%u] release\n", id_sec);

			/* close out slot */
			input_mt_report_slot_state(input_dev_sec, 0, 0);
#ifdef CONFIG_INPUT_DEVICE2_SINGLE_TOUCH			
			/* Set BTN_TOUCH to 0 */
			if (id == MXT_MIN_RPTID_SEC) {
				input_report_key(input_dev_sec, BTN_TOUCH, 0);
			}
#endif
		} else {
			dev_dbg(dev, "[%u] release\n", id);

			/* close out slot */
		  	input_mt_report_slot_state(input_dev, 0, 0);
		}
	}
	
	if (id >= MXT_MIN_RPTID_SEC) {
		data->update_input_sec = true;
	} else {
		data->update_input = true;
	}
}

static void mxt_proc_t15_messages(struct mxt_data *data, u8 *msg)
{
	struct input_dev *input_dev = data->input_dev;
	struct device *dev = &data->client->dev;
	int key;
	bool curr_state, new_state;
	bool sync = false;
	unsigned long keystates = (msg[2] | (u16)msg[3] << 8);
	unsigned int t15_num_keys;
	int id;
	u8 offset;
	
	id = msg[0] - data->T15_reportid_min;
	if (id == 0) {
		// Primary instance - using keystates low bits
		offset = 0;
		t15_num_keys = min(data->t15_num_keys_inst0, data->t15_num_keys);
	} else if (id == 1) {
		// Second instance - using keystates high bits
		offset = data->t15_num_keys_inst0;
		keystates <<= offset;
		t15_num_keys = data->t15_num_keys;
	} else {
		dev_err(dev, "Unknown T15 %d\n", id);
		return;
	}

	dev_dbg(dev, "T15 [%d] status (0x%lx - 0x%lx)\n", id, data->t15_keystatus, keystates);

	for (key = offset; key < t15_num_keys; key++) {
		curr_state = test_bit(key, &data->t15_keystatus);	// Note, the there maybe only support 32 keys(unsigned long) in total
		new_state = test_bit(key, &keystates);

		if (!curr_state && new_state) {
			dev_dbg(dev, "T15[%d] key press: %u\n", id, key);
			__set_bit(key, &data->t15_keystatus);
			if (data->t15_keymap[key]) {
				input_event(input_dev, EV_KEY,
						data->t15_keymap[key], 1);
				sync = true;
			}
		} else if (curr_state && !new_state) {
			dev_dbg(dev, "T15[%d] key release: %u\n", id, key);
			__clear_bit(key, &data->t15_keystatus);
			if (data->t15_keymap[key]) {
				input_event(input_dev, EV_KEY,
						data->t15_keymap[key], 0);
				sync = true;
			}
		}
	}

	if (sync) {
		input_sync(input_dev);
	}
}

static void mxt_proc_t10_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];
	u8 cmd = msg[2];
	u8 id = NUM_MESSAGE_CACHE_TYPES;

	/*
		Status:
			0x11: All POST tests have completed successfully
			0x12: A POST test has failed
			0x21: All BIST tests have completed successfully
			0x22: A BIST testd has failed
			0x23: BIST test cycle overrun
			0x31: All on-demand tests has passed
			0x32: An on demand test has failed
			0x3F: The test code supplied in the CMD field is not associated with a valid test
		CMD:
			2: Clock Related tests
			3: Flash Memory tests
			4: RAM memory tests
			5: CTE tests
			6: Signal Limit tests
			7: Power-related tests
			8: Pin Fault tests:
				The test failed becasue of pin fault. THe INFO fields indicated the first pin fault that was
					detected.Note that if the initial pin fault test fails, then the Self Test T25 object will
					generate a message with this result code on reset
					SEQ_NUM:
						0x01: Driven Ground
						0x02: Driven Hight
						0x03: Walking 1
						0x04: Walking 0
						0x07: Initial High Voltage
					X_PIN:
					Y_PIN:
						the number of the pin + 1(e.g. value 3 will mean X2)
						Both ZERO: DS pin
	*/

	if (status < MXT_SELFTEST_T10_MESSAGE_POST_PASS) {
		/* Un-supported */
	} else if (status < MXT_SELFTEST_T10_MESSAGE_BIST_PASS) {
		/* POST test */
		id = MESSAGE_CACHE_TEST_POST;
	} else if (status < MXT_SELFTEST_T10_MESSAGE_ON_DEMAND_PASS) {
		/* BIST test */
		id = MESSAGE_CACHE_TEST_BIST;
	} else if (status <= MXT_SELFTEST_T10_MESSAGE_TEST_CODE_INVALID) {
		/* On Demand test */
		id = MESSAGE_CACHE_TEST_ON_DEMAND;
	} else {
		/* Unknown command */
	}

	/* Output debug if status has changed */
	dev_info(dev, "T10 Status 0x%2x CMD %d Info: %02x %02x %02x, id %d\n",
		status, cmd, msg[3], msg[4], msg[5], id);

	/* Save message to cache */
	if (id < NUM_MESSAGE_CACHE_TYPES) {
		if (data->msg_cache) {
			memcpy(MESSAGE_CACHE(data->msg_cache, data->T5_msg_size, id), msg, data->T5_msg_size);
		}
	}
}

static void mxt_proc_t25_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1], cache_status;
	u8 id;

	/*
		T25 message:
			0xFE: All tests passed
			0xFD: The test code supplied in the CMD field is not associated with a valid test
			0x01: Avdd is not present.The failure is reported to the host every 200ms
			0x12: The test failed becasue of pin fault. THe INFO fields indicated the first pin fault that was
					detected.Note that if the initial pin fault test fails, then the Self Test T25 object will
					generate a message with this result code on reset
					SEQ_NUM:
						0x01: Driven Ground
						0x02: Driven Hight
						0x03: Walking 1
						0x04: Walking 0
						0x07: Initial High Voltage
					X_PIN:
					Y_PIN:
						the number of the pin + 1(e.g. value 3 will mean X2)
						Both ZERO: DS pin
			0x17: The test failed because of a signal limit fault.
	*/

	/* Output debug if status has changed */
	dev_info(dev, "T25 Status 0x%2x Info: %02x %02x %02x %02x %02x\n",
		status, msg[2], msg[3], msg[4], msg[5], msg[6]);

	/* Cache message if buffer exist */
	if (data->msg_cache) {
		/* Save message to cache, as T25 test all status is same, 
			so we should consider first message is POST message, and other message is onDemand whatever it's the periodic test */
		cache_status = GET_MESSAGE_CACHE_DATA(data->msg_cache, data->T5_msg_size, MESSAGE_CACHE_TEST_POST, MXT_T25_MSG_STATUS);
		if (cache_status == MXT_SELFTEST_T25_MESSAGE_TEST_INIT) {
			id = MESSAGE_CACHE_TEST_POST;
		} else {
			id = MESSAGE_CACHE_TEST_ON_DEMAND;
		}

		memcpy(MESSAGE_CACHE(data->msg_cache, data->T5_msg_size, id), msg, data->T5_msg_size);
		if (id == MESSAGE_CACHE_TEST_POST) {
			if (status == MXT_SELFTEST_T25_MESSAGE_TEST_INIT) {
				// There is a Firmware issue that POST test will report status 0x00 if Passed
				SET_MESSAGE_CACHE_DATA(data->msg_cache, data->T5_msg_size, MESSAGE_CACHE_TEST_POST, MXT_T25_MSG_STATUS, MXT_SELFTEST_T25_MESSAGE_TEST_PASS);
			}
		}
	}
}

static void mxt_proc_t42_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];

	if (status & MXT_T42_MSG_TCHSUP)
		dev_info(dev, "T42 suppress\n");
	else
		dev_info(dev, "T42 normal\n");
}

static int mxt_proc_t48_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status, state;

	status = msg[1];
	state  = msg[4];

	dev_dbg(dev, "T48 state %d status %02X %s%s%s%s%s\n", state, status,
		status & 0x01 ? "FREQCHG " : "",
		status & 0x02 ? "APXCHG " : "",
		status & 0x04 ? "ALGOERR " : "",
		status & 0x10 ? "STATCHG " : "",
		status & 0x20 ? "NLVLCHG " : "");

	return 0;
}

static void mxt_proc_t92_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];

	dev_info(dev, "T92 long stroke LSTR=%d %d\n",
		 (status & 0x80) ? 1 : 0,
		 status & 0x0F);
}

static void mxt_proc_t93_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];

	dev_info(dev, "T93 report double tap %d\n", status);
}

static int mxt_proc_message(struct mxt_data *data, u8 *message)
{
	struct device *dev = &data->client->dev;
	u8 report_id = message[0];
	bool dump = data->debug_enabled;

	if (report_id == MXT_RPTID_NOMSG || report_id == MXT_RPTID_RVSD) {
		return 0;
	}

	if (report_id == data->T6_reportid) {
		mxt_proc_t6_messages(data, message);
	} else if (report_id >= data->T42_reportid_min
		   && report_id <= data->T42_reportid_max) {
		mxt_proc_t42_messages(data, message);
	} else if (report_id == data->T48_reportid_min) {
		mxt_proc_t48_messages(data, message);
	}  else if (report_id == data->T10_reportid_min) {
		mxt_proc_t10_messages(data, message);
	} else if (report_id == data->T25_reportid_min) {
		mxt_proc_t25_messages(data, message);
	} else if (!data->input_dev) {
		/*
		 * Do not report events if input device
		 * is not yet registered.
		 */
		dev_dbg(dev, "Got message before input device registered:\n");
		if (dump) mxt_dump_message(data, message);
	} else if (report_id >= data->T9_reportid_min &&
		   report_id <= data->T9_reportid_max) {
		mxt_proc_t9_message(data, message);
	} else if (report_id >= data->T100_reportid_min &&
		   report_id <= data->T100_reportid_max) {
		mxt_proc_t100_message(data, message);
	} else if (report_id == data->T19_reportid_min) {
		mxt_input_button(data, message);
		data->update_input = true;
	} else if (report_id >= data->T15_reportid_min
		   && report_id <= data->T15_reportid_max) {
		mxt_proc_t15_messages(data, message);
	} else if (report_id == data->T92_reportid_min) {
		mxt_proc_t92_messages(data, message);
	} else if (report_id == data->T93_reportid_min) {
		mxt_proc_t93_messages(data, message);
	}else {
		dev_info(dev, "Unhandled message:\n");
		mxt_dump_message(data, message);
	}

	if (dump) {
		mxt_dump_message(data, message);
	}

	if (data->debug_v2_enabled) {
		mxt_debug_msg_add(data, message);
	}

	return 1;
}

static int mxt_read_and_process_messages(struct mxt_data *data, u8 count)
{
	struct device *dev = &data->client->dev;
	int ret;
	int i;
	u8 num_valid = 0;

	for (i = 0; i < count; i++) {
		ret = mxt_read_reg_auto(data->client, data->T5_address,
			data->T5_msg_size, data->msg_buf, data);

		if (ret && data->crc_enabled) {
			ret = mxt_resync_comm(data);
		}

		if (ret) {
			dev_err(dev, "Failed to read %u messages (%d)\n", count, ret);
			return ret;
		} 

		ret = mxt_proc_message(data, data->msg_buf);
		if (ret == 1) {
			num_valid++;
		} else {
			dev_dbg(dev, "Get: Invalid messages received(%d-%d):\n", count, i);
			mxt_dump_message(data, data->msg_buf);
			break;
		}
	}

	return num_valid;
}

static irqreturn_t mxt_process_messages_t44_t144(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int ret;
	u16 address;
	u8 count, num_left;

	/* Read T44 and T5 together for legacy devices */
	/* For new HA parts, read only T144 count */
	if (data->T144_address) {
		address = data->T144_address;
	} else {
		address = data->T44_address;
	}

	ret = mxt_read_reg_auto(data->client, address, 
			data->msg_count_size, data->msg_buf, data);
	if (ret) {
		dev_err(dev, "Failed to read T44/T144 and T5 (%d)\n", ret);
		if (data->crc_enabled) {
			// Recovery requires RETRIGEN bit to be enabled in config
			mxt_resync_comm(data);
		}
		return IRQ_HANDLED;
	}

	count = data->msg_buf[0];
	/*
	 * This condition may be caused by the CHG line being configured in
	 * Mode 0. It results in unnecessary I2C operations but it is benign.
	 */
	if (count == 0) {
	  	dev_warn(dev, "Interrupt occurred but no message\n");
	  	return IRQ_HANDLED;
	} else if (count > data->max_reportid) {
		dev_warn(dev, "T44/T144 count %d exceeded max report id (crc %d)\n", 
			count, data->crc_enabled);
		
		if (data->crc_enabled) {
			// Recovery requires RETRIGEN bit to be enabled in config
			ret = mxt_resync_comm(data);
			if (ret) {
				//Resync failed skipped next handled?
				return IRQ_HANDLED;
			}
		}
	}

	num_left = count;

	/* Process remaining messages if necessary */
	if (num_left) {
		dev_dbg(dev, "Remaining (%hhu) messages to process\n", num_left);

		ret = mxt_read_and_process_messages(data, num_left);
		if (ret < 0) {
			dev_err(dev, "Read: message failed (%d)\n", ret);
			goto end;
		} else if (ret != num_left) {
			dev_dbg(dev, "Read: Unexpected (%d) invalid message, expected (%d)\n", ret, num_left);
		}
	}

end:
	if (data->update_input || data->update_input_sec) {
		mxt_input_sync(data);
		
		if (data->update_input)
			data->update_input = false;
		
		if (data->update_input_sec)
			data->update_input_sec = false;
	}

	return IRQ_HANDLED;
}

static int mxt_process_messages_until_invalid(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int count, read;
	u8 tries = 2;

	count = data->max_reportid;

	/* Read messages until we force an invalid */
	do {
		read = mxt_read_and_process_messages(data, count);
		if (read < count)
			return 0;
	} while (--tries);

	if (data->update_input || data->update_input_sec ) {
		mxt_input_sync(data);
		
		if(data->update_input)
			data->update_input = false;
		
		if(data->update_input_sec)
			data->update_input_sec = false;
	}

	dev_err(dev, "CHG pin isn't cleared\n");
	return -EBUSY;
}

static irqreturn_t mxt_process_messages(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int total_handled, num_handled;
	u8 count = data->last_message_count;

	if (count < 1 || count > data->max_reportid)
		count = 1;

	/* include final invalid message */
	total_handled = mxt_read_and_process_messages(data, count + 1);
	if (total_handled < 0) {
	        dev_dbg(dev, "Interrupt occurred but no message\n");
		return IRQ_HANDLED;
	}

	/* if there were invalid messages, then we are done */
	else if (total_handled <= count)
		goto update_count;

	/* keep reading two msgs until one is invalid or reportid limit */
	do {
		num_handled = mxt_read_and_process_messages(data, 2);
		if (num_handled < 0) {
			dev_dbg(dev, "Interrupt occurred but no message\n");
			return IRQ_HANDLED;
		}

		total_handled += num_handled;

		if (num_handled < 2)
			break;
	} while (total_handled < data->num_touchids);

update_count:
	data->last_message_count = total_handled;

	if (data->update_input || data->update_input_sec) {
		mxt_input_sync(data);
		
		if(data->update_input)
			data->update_input = false;
		
		if(data->update_input_sec)
			data->update_input_sec = false;
	}

	return IRQ_HANDLED;
}

static irqreturn_t mxt_interrupt(int irq, void *dev_id)
{
	struct mxt_data *data = dev_id;

	if (data->in_bootloader) {
		/* bootloader state transition completion */
		complete(&data->bl_completion);
		return IRQ_HANDLED;
	}

	if (!data->object_table)
		return IRQ_HANDLED;

	if (atomic_read(&data->irq_processing) != 1 ) {
		dev_warn(&data->client->dev, "In IRQ thread tracking: irq_processing is incorrect(%d)\n", 
			atomic_read(&data->irq_processing));
	}

	if (data->T44_address || data->T144_address) {
			return mxt_process_messages_t44_t144(data);
	} else {
		return mxt_process_messages(data);
	}

	return IRQ_HANDLED;
}

static int mxt_t6_command(struct mxt_data *data, u16 cmd_offset,
			  u8 value, bool wait)
{
	u16 reg;
	u8 command_register;
	int timeout_counter = 0;
	int ret;

	if (!data->T6_address)
		return -EEXIST;

	reg = data->T6_address + cmd_offset;
	ret = mxt_write_reg_auto(data->client, reg, 1, &value, data);
	if (ret)
		return ret;

	if (!wait) {
		return 0;
	}

	do {
		msleep(20);

		ret = mxt_read_reg_auto(data->client, reg, 1, &command_register, data);
		if (ret)
			return ret;

	} while (command_register != 0 && timeout_counter++ <= 100);

	if (timeout_counter > 100) {
		dev_err(&data->client->dev, "Command failed!\n");
		return -EIO;
	}

	return 0;
}

static int mxt_acquire_irq(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	unsigned long irqflags = IRQF_TRIGGER_LOW;
	int error;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	/* Use irqd_get_trigger_type() to acquire the DTS setting automatically when not assigned #chg-gpios in dts,
		 otherwise you can ommit it and use hard coded way */
	if (IS_ERR_OR_NULL(data->chg_gpio)) {
		irqflags = IRQF_TRIGGER_NONE;
	}
#endif

	dev_dbg(&data->client->dev, "enable irq(%d): irq_processing(%d) irqflags(0x%lx)\n", 
		data->irq ? data->irq : client->irq, atomic_read(&data->irq_processing), irqflags);

	if (!data->irq) {
		atomic_set(&data->irq_processing, 1);
		error = devm_request_threaded_irq(&client->dev, client->irq,
					NULL, mxt_interrupt, irqflags| IRQF_ONESHOT,
					client->name, data);
		if (error) {
			atomic_set(&data->irq_processing, 0);
			dev_err(&client->dev, "Failed to register interrupt(%d) %d\n", client->irq, error);
			return error;
		}

		data->irq = client->irq;
	} else {
		atomic_inc(&data->irq_processing);
		enable_irq(data->irq);
	}

	return 0;
}

static void mxt_disable_irq(struct mxt_data *data)
{
	dev_dbg(&data->client->dev, "disable irq(%d): irq_processing(%d)\n", 
		data->irq, atomic_read(&data->irq_processing));

	if (data->irq) {
		disable_irq(data->irq);
		atomic_dec(&data->irq_processing);
	}
}

static void mxt_free_irq(struct mxt_data *data)
{
	dev_dbg(&data->client->dev, "free irq(%d)\n", 
		data->irq);

	if (data->irq) {
		devm_free_irq(&data->client->dev, data->irq, data);
		data->irq = 0;
	}
}

// BIT(0) is resersed for the compatibility with maxtouch studio of /sys/
#define F_R_RSV BIT(0)
#define F_R_SOFT BIT(1)
#define F_R_HARD BIT(2)
#define F_R_WAIT BIT(3)
#define F_R_CHECK BIT(4)
#define F_RST_SOFT	(F_R_SOFT | F_R_WAIT)
#define F_RST_HARD  (F_R_HARD | F_R_WAIT)
#define F_RST_ANY	(F_R_SOFT | F_R_HARD | F_R_WAIT)

static int __soft_reset(struct mxt_data *data, u8 flag) 
{
	struct device *dev = &data->client->dev;
	int ret;

	dev_info(dev, "Resetting chip(S)\n");

	ret = mxt_t6_command(data, MXT_COMMAND_RESET, MXT_RESET_VALUE, false);
	if (ret) {
		return ret;
	} else {
		/* After reset, need to update seq num to ZERO */
		// FIXME: there may be the thread sychronization issue for Seq num, unless it's called under irq disabled wrapped
		mxt_update_seq_num_lock(data, true, 0x00);
	}

	/* Ignore CHG line after reset */
	if (flag & F_R_WAIT) {
		msleep(MXT_RESET_INVALID_CHG);
	}

	return 0;
}

static int __hard_reset(struct mxt_data *data, u8 flag) 
{
	struct device *dev = &data->client->dev;
#ifdef CONFIG_MXT_POR_CHG_WAINTING_LEVEL
	int val, count;
#endif

	dev_info(dev, "Resetting chip(H)\n");

	if (!data->reset_gpio) {
		return -EIO;
	}

	// Low level for asserting HW reset, this set whether active of `reset-gpios` setting in DTS
	// Note 1: if you using the non-standard kernel, there may be not be compatible.
	// So that, you could consider to switch the active level
	// Now we use directly gpio control(discard DTS setting): Low --- Reset active; High --- Chip working

	// Note 2: Please be wared of that, the maxtouch need special POR sequence that you must stretch Reset low before VDD raised to target voltage(~3.3v)
	// If you don't match this in Por, the chip have chance to halt. Assert the hardware reset only is not benifit for POR.
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))	
	gpiod_set_value(data->reset_gpio, true);	//Reset active
#else
	gpio_set_value(data->reset_gpio, 0);	//Reset active
#endif
	msleep(MXT_RESET_GPIO_TIME);

	/* After reset, need to update seq num to ZERO */
	mxt_update_seq_num_lock(data, true, 0x00);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	gpiod_set_value(data->reset_gpio, false);	//Reset active
#else
	gpio_set_value(data->reset_gpio, 1);	//Reset inactive
#endif

	// Wait for Reset completed by timeout
	if (flag & F_R_WAIT) {
#ifdef CONFIG_MXT_POR_CHG_WAINTING_LEVEL
		if (data->chg_gpio) {
			// For initialized reset time
			msleep(MXT_RESET_TIME);

			// Wating for CHG to low
			for (count = 0; count <= (MXT_RESET_INVALID_CHG - MXT_RESET_TIME) / MXT_RESET_CHG_CHECK_TIME; count++) {
				// Check the CHG pin if it's low level, that means RESET completed
				val = 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
					gpiod_get_value(data->chg_gpio)
#else
					!gpio_get_value(data->chg_gpio)
#endif
				;
				if (val) {
					break;
				}

				msleep(MXT_RESET_CHG_CHECK_TIME);
			}

			dev_info(&data->client->dev,
				"Hardware Reset waiting for %d + (%d x %d ms)", MXT_RESET_TIME, count, MXT_RESET_CHG_CHECK_TIME);
		} else 
#endif
		{
			msleep(MXT_RESET_INVALID_CHG);
		}
	}

	return 0;
}

static int __mxt_reset(struct mxt_data *data, u8 flag) 
{
	struct device *dev = &data->client->dev;
	int ret = -EPERM;

	dev_info(dev, "Mxt Reset (Flag: %02X)\n", flag);

	/* Note this function usually should be called with irq disabled unless you don't think about the Seq num 
		and If the flag is not set whether a soft or a hard mode, it's meaningless
	*/

	if (flag & F_R_SOFT) {
		ret = __soft_reset(data, flag);
		if (!ret) {
			return 0;
		}
	}
	
	if (flag & F_R_HARD) {
		ret = __hard_reset(data, flag);
		if (!ret) {
			return 0;
		}
	}

	if (ret) {
		dev_err(dev, "Resetting failed(%d)\n", ret);
	}

	return ret;
}

static int mxt_reset(struct mxt_data *data, u8 flag) 
{
	struct device *dev = &data->client->dev;
	int ret = 0;

	dev_info(dev, "Resetting device(%02X)\n", flag);

	mxt_disable_irq(data);

	INIT_COMPLETION(data->reset_completion);

	ret =__mxt_reset(data, flag);

	mxt_acquire_irq(data);

	if (!ret) {
		ret = mxt_wait_for_completion(data, &data->reset_completion,
				      MXT_RESET_TIMEOUT);
		if (ret) {
			dev_err(dev, "Wait for Resetting timeout(%d)\n", ret);
			return ret;
		}
	} else {
		dev_err(dev, "Resetting device failed(%d)\n", ret);
			return ret;
	}

	return 0;
}

static void mxt_update_crc(struct mxt_data *data, u8 cmd, u8 value)
{
	/*
	 * On failure, CRC is set to 0 and config will always be
	 * downloaded.
	 */

	INIT_COMPLETION(data->crc_completion);

	mxt_t6_command(data, cmd, value, true);

	/*
	 * Wait for crc message. On failure, CRC is set to 0 and config will
	 * always be downloaded.
	 */
	mxt_wait_for_completion(data, &data->crc_completion, MXT_CRC_TIMEOUT);
}

static void mxt_calc_crc24(u32 *crc, u8 firstbyte, u8 secondbyte)
{
	static const unsigned int crcpoly = 0x80001B;
	u32 result;
	u32 data_word;

	data_word = (secondbyte << 8) | firstbyte;
	result = ((*crc << 1) ^ data_word);

	if (result & 0x1000000)
		result ^= crcpoly;

	*crc = result;
}

static u32 mxt_calculate_crc(u8 *base, off_t start_off, off_t end_off)
{
	u32 crc = 0;
	u8 *ptr = base + start_off;
	u8 *last_val = base + end_off - 1;

	if (end_off < start_off)
		return -EINVAL;

	while (ptr < last_val) {
		mxt_calc_crc24(&crc, *ptr, *(ptr + 1));
		ptr += 2;
	}

	/* if len is odd, fill the last byte with 0 */
	if (ptr == last_val)
		mxt_calc_crc24(&crc, *ptr, 0);

	/* Mask to 24-bit */
	crc &= 0x00FFFFFF;

	return crc;
}
				   							   	
static int mxt_check_retrigen(struct mxt_data *data)
{

	struct i2c_client *client = data->client;
	int error;
	int val;
	int buff;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	unsigned long irqflags;
#endif

	data->use_retrigen_workaround = false;

	/*Iqnore when using level triggered mode */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	irqflags = irq_get_trigger_type(data->irq);
	if (irqflags & IRQF_TRIGGER_LOW) {
		dev_info(&client->dev, "Level triggered\n");
	    return 0;
	} else {
		dev_info(&client->dev, "Get Irqflags 0x%lx, will check Retrigen mode\n", irqflags);
	}
#else
	//assume request_threaded_irq() default using IRQF_TRIGGER_LOW as trigger mode
	return 0;
#endif
	
	if (!data->T18_address)
		return -EEXIST;

	error = mxt_read_reg_auto(client,
		data->T18_address + MXT_COMMS_CTRL,
		1, &val, data);
	if (error)
		return error;

	if (val & MXT_COMMS_RETRIGEN) {
		dev_info(&client->dev, "RETRIGEN enabled\n");
		return 0;
	}

	buff = val | MXT_COMMS_RETRIGEN;
	error = mxt_write_reg_auto(client,
			data->T18_address + MXT_COMMS_CTRL,
			1, &buff, data);
	if (error)
		return error;

	dev_info(&client->dev, "RETRIGEN Enabled feature\n");
	data->use_retrigen_workaround = true;

	return 0;
}

static bool mxt_object_is_volatile(struct mxt_data *data, uint16_t object_type)
{
 
 	switch (object_type) {
  	case MXT_GEN_MESSAGE_T5:
  	case MXT_GEN_COMMAND_T6:
  	case MXT_DEBUG_DIAGNOSTIC_T37:
  	case MXT_SPT_MESSAGECOUNT_T44:
  	case MXT_DATACONTAINER_T117:
  	case MXT_SPT_MESSAGECOUNT_T144:
  
    	return true;

  	default:
    	return false;
    }	
}

static int mxt_prepare_cfg_mem(struct mxt_data *data, struct mxt_cfg *cfg)
{
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	unsigned int type, instance, size, byte_offset = 0;
	int offset, write_offset = 0;
	int totalBytesToWrite = 0;
	unsigned int first_obj_type = 0;
	unsigned int first_obj_addr = 0;
	int ret, i, error;
	u16 reg;
	u8 val;

	cfg->object_skipped_ofs = 0;

	/* Loop until end of raw file */
	while (cfg->raw_pos < cfg->raw_size) {
		/* Read type, instance, length */
		ret = sscanf(cfg->raw + cfg->raw_pos, "%x %x %x%n",
			     &type, &instance, &size, &offset);
		if (ret == 0) {
			/* EOF */
			break;
		} else if (ret != 3) {
			dev_err(dev, "Bad format: failed to parse object\n");
			return -EINVAL;
		}
		/* Update position in raw file to start of obj data */
		cfg->raw_pos += offset;

		object = mxt_get_object(data, type);

		/* Find first object in cfg file; if not first in device */
		if (first_obj_type == 0) {
			first_obj_type = type;
			first_obj_addr = object->start_address;

			dev_info(dev, "First object found T[%d]\n", type);

			if (first_obj_addr > cfg->start_ofs) {
				cfg->object_skipped_ofs = first_obj_addr - cfg->start_ofs;

				dev_dbg(dev, "cfg->object_skipped_ofs %d, first_obj_addr %d, cfg->start_ofs %d\n", 
					cfg->object_skipped_ofs, first_obj_addr, cfg->start_ofs);

				cfg->mem_size = cfg->mem_size - cfg->object_skipped_ofs;
			}
		}

		if(!object || (mxt_object_is_volatile(data, type))) {
			/* Skip object if not present in device or volatile */

			dev_info(dev, "Skipping object T[%d] Instance %d\n", type, instance);

			for (i = 0; i < size; i++) {
				ret = sscanf(cfg->raw + cfg->raw_pos, "%hhx%n",
					     &val, &offset);
				if (ret != 1) {
					dev_err(dev, "Bad format in T%d at %d\n",
						type, i);
					return -EINVAL;
				}
				/*Update position in raw file to next obj */
				cfg->raw_pos += offset;

				/* Adjust byte_offset for skipped objects */
				cfg->object_skipped_ofs = cfg->object_skipped_ofs + 1;;

				/* Adjust config memory size, less to program */
				/* Only for non-volatile T objects */
				cfg->mem_size--;
				dev_dbg(dev, "cfg->mem_size [%zd]\n", cfg->mem_size);

			}
			continue;
		}

		if (size > mxt_obj_size(object)) {
			/*
			 * Either we are in fallback mode due to wrong
			 * config or config from a later fw version,
			 * or the file is corrupt or hand-edited.
			 */
			dev_warn(dev, "Discarding %zu byte(s) in T%u\n",
				 size - mxt_obj_size(object), type);
		} else if (mxt_obj_size(object) > size) {
			/*
			 * If firmware is upgraded, new bytes may be added to
			 * end of objects. It is generally forward compatible
			 * to zero these bytes - previous behaviour will be
			 * retained. However this does invalidate the CRC and
			 * will force fallback mode until the configuration is
			 * updated. We warn here but do nothing else - the
			 * malloc has zeroed the entire configuration.
			 */
			dev_warn(dev, "Zeroing %zu byte(s) in T%d\n",
				 mxt_obj_size(object) - size, type);
		}

		if (instance >= mxt_obj_instances(object)) {
			dev_err(dev, "Object instances exceeded!\n");
			return -EINVAL;
		}

		reg = object->start_address + mxt_obj_size(object) * instance;
		
		for (i = 0; i < size; i++) {
			ret = sscanf(cfg->raw + cfg->raw_pos, "%hhx%n",
				     &val,
				     &offset);
			if (ret != 1) {
				dev_err(dev, "Bad format in T%d at %d\n",
					type, i);
				return -EINVAL;
			}
			/* Update position in raw file to next byte */
			cfg->raw_pos += offset;

			if (i > mxt_obj_size(object))
				continue;

			byte_offset = reg + i - cfg->start_ofs - cfg->object_skipped_ofs;
			// add write offset calculation for every object to suit for different raw format
			if (i == 0) {
				write_offset = byte_offset;
			}

			if (byte_offset >= 0) {
				*(cfg->mem + byte_offset) = val;
			} else {
				dev_err(dev, "Bad object: reg: %d, T%d, ofs=%d\n",
					reg, object->type, byte_offset);
				return -EINVAL;
			}
		}

		totalBytesToWrite = size;


		/* Write per object per instance per obj_size w/data in cfg.mem */
		while (totalBytesToWrite > 0) {

			if (totalBytesToWrite > MXT_MAX_BLOCK_WRITE)
				size = MXT_MAX_BLOCK_WRITE;
			else 
				size = totalBytesToWrite;

			error = mxt_write_reg_auto(data->client, reg, size, (cfg->mem + write_offset), data);
			if (error)
				return error;

			write_offset = write_offset + size;
			totalBytesToWrite = totalBytesToWrite - size;
		}

		msleep(20);

	} /* End of while loop */

	return 0;
}

static int mxt_init_t7_power_cfg(struct mxt_data *data);

/*
 * mxt_update_cfg - download configuration to chip
 *
 * Atmel Raw Config File Format
 *
 * The first four lines of the raw config file contain:
 *  1) Version
 *  2) Chip ID Information (first 7 bytes of device memory)
 *  3) Chip Information Block 24-bit CRC Checksum
 *  4) Chip Configuration 24-bit CRC Checksum
 *
 * The rest of the file consists of one line per object instance:
 *   <TYPE> <INSTANCE> <SIZE> <CONTENTS>
 *
 *   <TYPE> - 2-byte object type as hex
 *   <INSTANCE> - 2-byte object instance number as hex
 *   <SIZE> - 2-byte object size as hex
 *   <CONTENTS> - array of <SIZE> 1-byte hex values
 */
static int mxt_update_cfg(struct mxt_data *data, const struct firmware *fw)
{
	struct device *dev = &data->client->dev;
	struct mxt_cfg cfg;
	u32 info_crc, config_crc, calculated_crc;
	u16 crc_start = 0;
	int ret, error;
	int offset;
	int i;

	/* Make zero terminated copy of the OBP_RAW file */
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0)
	cfg.raw = kmemdup_nul(fw->data, fw->size, GFP_KERNEL);
	if (!cfg.raw)
		return -ENOMEM;
#else
	/* Make zero terminated copy of the OBP_RAW file */
	cfg.raw = kzalloc(fw->size + 1, GFP_KERNEL);
	if (!cfg.raw)
		return -ENOMEM;
	memcpy(cfg.raw, fw->data, fw->size);
	cfg.raw[fw->size] = '\0';
#endif
	cfg.raw_size = fw->size;

	// FIXME: the Report All command can't get out the config crc not
	mxt_update_crc(data, MXT_COMMAND_REPORTALL, 1);

	//Clear messages after update in cases /CHG low
	error = mxt_process_messages_until_invalid(data);
	if (error)
		dev_dbg(dev, "Unable to read CRC\n");

	if (strncmp(cfg.raw, MXT_CFG_MAGIC, strlen(MXT_CFG_MAGIC))) {
		dev_err(dev, "Unrecognised config file\n");
		ret = -EINVAL;
		goto release_raw;
	}

	cfg.raw_pos = strlen(MXT_CFG_MAGIC);

	/* Load 7byte infoblock from config file */
	for (i = 0; i < sizeof(struct mxt_info); i++) {
		ret = sscanf(cfg.raw + cfg.raw_pos, "%hhx%n",
			     (unsigned char *)&cfg.info + i,
			     &offset);
		if (ret != 1) {
			dev_err(dev, "Bad format\n");
			ret = -EINVAL;
			goto release_raw;
		}

		/* Update position in raw file to info CRC */
		cfg.raw_pos += offset;
	}

	/* Compare family id, file vs chip */
	if (cfg.info.family_id != data->info->family_id) {
		dev_err(dev, "Family ID mismatch!\n");
		ret = -EINVAL;
		goto release_raw;
	}

	/* Compare variant id, file vs chip */
	if (cfg.info.variant_id != data->info->variant_id) {
		dev_err(dev, "Variant ID mismatch!\n");
		ret = -EINVAL;
		goto release_raw;
	}

	/* Read Infoblock CRCs */
	ret = sscanf(cfg.raw + cfg.raw_pos, "%x%n", &info_crc, &offset);
	if (ret != 1) {
		dev_err(dev, "Bad format: failed to parse Info CRC\n");
		ret = -EINVAL;
		goto release_raw;
	}
	/* Update position in raw file to config CRC */
	cfg.raw_pos += offset;

	ret = sscanf(cfg.raw + cfg.raw_pos, "%x%n", &config_crc, &offset);
	if (ret != 1) {
		dev_err(dev, "Bad format: failed to parse Config CRC\n");
		ret = -EINVAL;
		goto release_raw;
	}
	/* Update position in raw file to first T object */
	cfg.raw_pos += offset;

	/*
	 * The Info Block CRC is calculated over mxt_info and the object
	 * table. If it does not match then we are trying to load the
	 * configuration from a different chip or firmware version, so
	 * the configuration CRC is invalid anyway.
	 */
	if (info_crc == data->info_crc) {
		if (config_crc == 0 || data->config_crc == 0) {
			dev_info(dev, "CRC zero, attempting to apply config\n");
		} else if (config_crc == data->config_crc) {
			dev_info(dev, "Config file CRC 0x%06X same as device CRC: No update required.\n",
				 data->config_crc);
			ret = 0;
			goto release_raw;
		} else {
			dev_info(dev, "Device config CRC 0x%06X: does not match file CRC 0x%06X: Updating...\n",
				 data->config_crc, config_crc);
		}
	} else {
		dev_warn(dev,
			 "Warning: Info CRC does not match: Error - device crc=0x%06X file=0x%06X\nFailed Config Programming\n",
			 data->info_crc, info_crc);
		ret = -EIO;
		goto release_raw; 
	}

	/* Stop T70 Dynamic Configuration before calculation of CRC */

	mxt_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_W_STOP);

	/* Malloc memory to store configuration */
	cfg.start_ofs = MXT_OBJECT_START +
			data->info->object_num * sizeof(struct mxt_object) +
			MXT_INFO_CHECKSUM_SIZE;

	cfg.mem_size = data->mem_size - cfg.start_ofs;

	cfg.mem = kzalloc(cfg.mem_size, GFP_KERNEL);
	if (!cfg.mem) {
		ret = -ENOMEM;
		goto release_mem;
	}

	dev_dbg(dev, "update_cfg: cfg.mem_size %zi, cfg.start_ofs %i, cfg.raw_pos %lld, offset %i", 
		cfg.mem_size, cfg.start_ofs, (long long)cfg.raw_pos, offset);

	/* Prepares and programs configuration */
	ret = mxt_prepare_cfg_mem(data, &cfg);
	if (ret)
		goto release_mem;

	/* Calculate crc of the config file */
	/* Config file must include all objects used in CRC calculation */

	if (data->T14_address)
		crc_start = data->T14_address;
	else if (data->T71_address)
		crc_start = data->T71_address;
	else if (data->T7_address)
		crc_start = data->T7_address;
	/* Set position to next line */
	else
		dev_warn(dev, "Could not find CRC start\n");

	dev_dbg(dev, "calculate_crc: crc_start %zd, cfg.object_skipped_ofs %d, cfg.mem_size %zi\n", 
		cfg.mem_size, cfg.object_skipped_ofs, cfg.mem_size);

	if (crc_start > cfg.start_ofs) {
		calculated_crc = mxt_calculate_crc(cfg.mem,
						   crc_start - cfg.start_ofs - cfg.object_skipped_ofs,
						   cfg.mem_size);

		if (config_crc > 0 && config_crc != calculated_crc)
			dev_warn(dev, "Config CRC in file inconsistent, calculated=%06X, file=%06X\n",
				 calculated_crc, config_crc);
	}

	msleep(50);	//Allow delay before issuing backup and reset

	mxt_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);

	msleep(200);	//Allow 200ms before issuing reset

	mxt_reset(data, F_RST_ANY);

	dev_info(dev, "Config successfully updated\n");
	
	/* Successfully updated */
	ret = 1;

release_mem:
	kfree(cfg.mem);
release_raw:
	kfree(cfg.raw);

	return ret;
}

static int mxt_clear_cfg(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct mxt_cfg config;
	int writeByteSize = 0;
	int write_offset = 0;
	int totalBytesToWrite = 0;
	int error;

	/* Avoid corruption if chip in bootloader at bootup*/
	if (!data->info) {
		return -ENOENT;
	}

	/* Start of first Tobject address */
	config.start_ofs = MXT_OBJECT_START +
			data->info->object_num * sizeof(struct mxt_object) +
			MXT_INFO_CHECKSUM_SIZE;

	config.mem_size = data->mem_size - config.start_ofs;
	totalBytesToWrite = config.mem_size;

	/* Allocate memory for full size of config space */
	config.mem = kzalloc(config.mem_size, GFP_KERNEL);
	if (!config.mem) {
		error = -ENOMEM;
		goto release_mem;
	}

	dev_dbg(dev, "clear_cfg: config.mem_size %zi, config.start_ofs %i\n", 
		config.mem_size, config.start_ofs);

	while (totalBytesToWrite > 0) {

		if (totalBytesToWrite > MXT_MAX_BLOCK_WRITE)
			writeByteSize = MXT_MAX_BLOCK_WRITE;
		else
			writeByteSize = totalBytesToWrite;

		/* clear memory using config.mem buffer */
		error = mxt_write_reg_auto(data->client, (config.start_ofs + write_offset), 
			writeByteSize, config.mem, data);
		if (error) {
			dev_info(dev, "Error writing configuration\n");
			goto release_mem;
		}

		write_offset = write_offset + writeByteSize;
		totalBytesToWrite = totalBytesToWrite - writeByteSize;
	}

	/* 
		// Just write to memory, never do backup
		
		mxt_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);

	  	msleep(300);
	*/

	dev_info(dev, "Config successfully cleared\n");

release_mem:
	kfree(config.mem);
	return error;
}

static void mxt_free_input_device(struct mxt_data *data)
{
	if (data->input_dev) {
		input_unregister_device(data->input_dev);
		data->input_dev = NULL;
	}
}

static void mxt_free_second_input_device(struct mxt_data *data)
{
	if (data->input_dev_sec) {
		input_unregister_device(data->input_dev_sec);
		data->input_dev_sec = NULL;
	}
}

static void mxt_free_object_table(struct mxt_data *data)
{
#ifdef CONFIG_TOUCHSCREEN_MICROCHIP_MXT_DEBUG
	video_unregister_device(&data->dbg.vdev);
	v4l2_device_unregister(&data->dbg.v4l2);
#endif
	data->object_table = NULL;
	data->info = NULL;
	if (data->raw_info_block) {
		kfree(data->raw_info_block);
		data->raw_info_block = NULL;
	}
	if (data->msg_buf) {
		kfree(data->msg_buf);
		data->msg_buf = NULL;
	}
	data->msg_cache = NULL;

	data->T5_address = 0;
	data->T5_msg_size = 0;
	data->T6_reportid = 0;
	data->T7_address = 0;
	data->T14_address = 0;
	data->T71_address = 0;
	data->T9_reportid_min = 0;
	data->T9_reportid_max = 0;
	data->T10_address = 0;
	data->T10_reportid_min = 0;
	data->T15_reportid_min = 0;
	data->T15_reportid_max = 0;
	data->T18_address = 0;
	data->T19_reportid_min = 0;
	data->T25_address = 0;
	data->T25_reportid_min = 0;
	data->T42_reportid_min = 0;
	data->T42_reportid_max = 0;
	data->T44_address = 0;
	data->T48_reportid_min = 0;
	data->T92_reportid_min = 0;
	data->T92_address = 0;
	data->T93_reportid_min = 0;
	data->T93_address = 0;
	data->T100_reportid_min = 0;
	data->T100_reportid_max = 0;
	data->max_reportid = 0;
	data->T144_address = 0;
}

static int mxt_parse_object_table(struct mxt_data *data,
				  struct mxt_object *object_table)
{
	struct i2c_client *client = data->client;
	int i, message_buf_size, message_selftest_cache_size;
	u8 reportid;
	u16 end_address;
	u8 num_instances;

	/* Valid Report IDs start counting from 1 */
	reportid = 1;
	data->mem_size = 0;

	for (i = 0; i < data->info->object_num; i++) {
		struct mxt_object *object = object_table + i;
		u8 min_id, max_id;

		le16_to_cpus(&object->start_address);

		num_instances = mxt_obj_instances(object);
		
		if (object->num_report_ids) {
			min_id = reportid;
			reportid += object->num_report_ids *
					num_instances;
			max_id = reportid - 1;
		} else {
			min_id = 0;
			max_id = 0;
		}

		dev_dbg(&data->client->dev,
			"T%u Start:%u Size:%zu Instances:%zu Report IDs:%u-%u\n",
			object->type, object->start_address,
			mxt_obj_size(object), mxt_obj_instances(object),
			min_id, max_id);

		switch (object->type) {
		case MXT_GEN_MESSAGE_T5:
			if (data->info->family_id == 0x80 &&
			    data->info->version < 0x20) {
				/*
				 * On mXT224 firmware versions prior to V2.0
				 * read and discard unused CRC byte otherwise
				 * DMA reads are misaligned.
				 */
				data->T5_msg_size = mxt_obj_size(object);
			} else {
				if (data->crc_enabled) {
					data->T5_msg_size = mxt_obj_size(object);
				} else { //Skip byte, CRC not enabled
					data->T5_msg_size = mxt_obj_size(object) - 1;
				}
			}
			data->T5_address = object->start_address;
			break;
		case MXT_GEN_COMMAND_T6:
			data->T6_reportid = min_id;
			data->T6_address = object->start_address;
			break;
		case MXT_GEN_POWER_T7:
			data->T7_address = object->start_address;
			break;
		case MXT_PROCI_KEYTHRESHOLD_T14:
			data->T14_address = object->start_address;
			break;
		case MXT_SPT_USERDATA_T38:
			data->T38_address = object->start_address;
			break;
		case MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71:
			data->T71_address = object->start_address;
			break;
		case MXT_TOUCH_MULTI_T9:
			data->multitouch = MXT_TOUCH_MULTI_T9;
			/* Only handle messages from first T9 instance */
			data->T9_reportid_min = min_id;
			data->T9_reportid_max = min_id +
						object->num_report_ids - 1;
			data->num_touchids = object->num_report_ids;
			break;
		case MXT_SPT_SELFTESTCONTROL_T10:
			data->T10_address = object->start_address;
			data->T10_reportid_min = min_id;
			break;
		case MXT_TOUCH_KEYARRAY_T15:
			if (!data->multitouch) {
				data->multitouch = MXT_TOUCH_KEYARRAY_T15;
			}
			data->T15_reportid_min = min_id;
			data->T15_reportid_max = max_id;
			data->T15_instances = num_instances;
			break;
		case MXT_SPT_COMMSCONFIG_T18:
			data->T18_address = object->start_address;
			break;
		case MXT_SPT_GPIOPWM_T19:
			data->T19_reportid_min = min_id;
			break;
		case MXT_PROCI_ONETOUCH_T24:
			data->T24_reportid_min = min_id;
			data->T24_reportid_max = max_id;
			break;
		case MXT_SPT_SELFTEST_T25:
			data->T25_address = object->start_address;
			data->T25_reportid_min = min_id;
			break;
		case MXT_PROCI_TWOTOUCH_T27:
			data->T27_reportid_min = min_id;
			data->T27_reportid_max = max_id;
			break;
		case MXT_PROCI_TOUCHSUPPRESSION_T42:
			data->T42_reportid_min = min_id;
			data->T42_reportid_max = max_id;
			break;
		case MXT_SPT_MESSAGECOUNT_T44:
			data->T44_address = object->start_address;
			data->msg_count_size = mxt_obj_size(object);
			// Mark crc method false for non HA-chips
			data->crc_enabled = false;
			break;
		case MXT_SPT_CTECONFIG_T46:
			data->T46_reportid_min = min_id;
			break;
		case MXT_PROCG_NOISESUPPRESSION_T48:
			data->T48_reportid_min = min_id;
			break;
		case MXT_PROCI_SHIELDLESS_T56:
			data->T56_reportid_min = min_id;
			break;
		case MXT_SPT_TIMER_T61:
			data->T61_reportid_min = min_id;
			data->T61_reportid_max = max_id;
			break;
		case MXT_PROCI_LENSBENDING_T65:
			data->T65_reportid_min = min_id;
			data->T65_reportid_max = max_id;
			break;
		case MXT_SPT_SERIALDATACOMMAND_T68:
			data->T68_reportid_min = min_id;
			break;
			case MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70:
			data->T70_reportid_min = min_id;
			data->T70_reportid_max = max_id;
			break;
		case MXT_NOISESUPPRESSION_T72:
			data->T72_reportid_min = min_id;
			break;
		case MXT_PROCI_RETRANSMISSIONCOMPENSATION_T80:
			data->T80_reportid_min = min_id;
			data->T80_reportid_max = max_id;
			break;				
		case MXT_PROCI_SYMBOLGESTUREPROCESSOR_T92:
			data->T92_reportid_min = min_id;
			data->T92_address = object->start_address;
			break;
		case MXT_PROCI_TOUCHSEQUENCELOGGER_T93:
			data->T93_reportid_min = min_id;
			data->T93_address = object->start_address;
			break;
		case MXT_TOUCH_MULTITOUCHSCREEN_T100:
			data->multitouch = MXT_TOUCH_MULTITOUCHSCREEN_T100;
			data->T100_reportid_min = min_id;
			data->T100_reportid_max = max_id;
			data->T100_instances = num_instances;
			/* first two report IDs reserved */
			data->num_touchids = object->num_report_ids - MXT_RSVD_RPTIDS;
			break;
		case MXT_PROCI_ACTIVESTYLUS_T107:
			data->T107_address = object->start_address;
			break;
		case MXT_PROCG_NOISESUPSELFCAP_T108:
			data->T108_reportid_min = min_id;
			break;
		case MXT_SPT_SELFCAPGLOBALCONFIG_T109:
			data->T109_reportid_min = min_id;
			break;
		case MXT_PROCI_SELFCAPGRIPSUPPRESSION_T112:
			data->T112_reportid_min = min_id;
			data->T112_reportid_max = max_id;
			break;
		case MXT_SPT_SELCAPVOLTAGEMODE_T133:
			data->T133_reportid_min = min_id;
			break;
		case MXT_PROCI_HOVERGESTUREPROCESSOR_T129:
			data->T129_reportid_min = min_id;
			break;
		case MXT_SPT_MESSAGECOUNT_T144:
			data->T144_address = object->start_address;
			data->msg_count_size = mxt_obj_size(object);
			data->crc_enabled = true;	// FIXME: if crc_enabled is marked true, it will never been marked false
			dev_info(&client->dev, "CRC enabled\n");
			break;
		}

		end_address = object->start_address
			+ mxt_obj_size(object) * mxt_obj_instances(object) - 1;

		if (end_address >= data->mem_size) {
			data->mem_size = end_address + 1;
		}
	}

	/* Store maximum reportid */
	data->max_reportid = reportid;
	
	/* If T44 exists, T5 position has to be directly after */
	if (data->T44_address && (data->T5_address != data->T44_address + 1)) {
		dev_err(&client->dev, "Invalid T44 position\n");
		return -EINVAL;
	}

	message_buf_size = data->max_reportid * data->T5_msg_size + data->msg_count_size;
	message_selftest_cache_size = data->T5_msg_size * NUM_MESSAGE_CACHE_TYPES;
	data->msg_buf = kzalloc(message_buf_size + message_selftest_cache_size, GFP_KERNEL);
	if (!data->msg_buf) {
		return -ENOMEM;
	}

	/* message cache for store some information processed later */
	data->msg_cache = data->msg_buf + message_buf_size;

	return 0;
}

typedef enum SYNCE_STATUS {
	SYNCED_UNKNOWN,
	SYNCED_FAILED,
	SYNCED_COMPLETED
} SYNCE_STATUS_T;

static int mxt_resync_comm(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;
	size_t size, dev_id_size, info_block_size, info_block_size_max;
	void *dev_id_buf = NULL, *buf, *sbuf;
	uint8_t num_objects;
	u32 info_crc, calculated_crc;
	u8 seqnum, seqnum_last, *crc_ptr;
	u16 seqnum_test, reg, i, j, k, step, first_step, count = 0;
	const struct mxt_info *info = data->info;
	SYNCE_STATUS_T synced = SYNCED_UNKNOWN;

	dev_info(&client->dev,"Resync: ++\n");

	// Store last know seq
	seqnum_last = mxt_curr_seq_num(data);

	/* Read 7-byte ID information block starting at address 0 */
	dev_id_size = sizeof(struct mxt_info);

	/* Directly using maximum size for sync */
	info_block_size_max = dev_id_size + (/*num_objects*/0xFF * sizeof(struct mxt_object))
					+ MXT_INFO_CHECKSUM_SIZE;

	dev_id_buf = kzalloc(info_block_size_max, GFP_KERNEL);
	if (!dev_id_buf) {
		error = -ENOMEM;
		dev_err(&client->dev,
			"Resync Alloc Info Block memory(%zd) failed", info_block_size_max);
		goto err_free_mem;
	}

	/*
		The search flowchart:
		<Round.0> The chip may occasionally reset (By ESD/BOD or some other things), the Seqnum is mostly Zero and nearby.
					We Search from with Seqnum 2 with 4 times loop. (CRC missed, and one debounce, so start from will be 2)
		<Round.1> The Seqnum is somewhere unknown.
					We search from latest known value with a step value debounce (seqnum_last + step), and `256 + step` times loop.
		<Round.2> The Seqnum is ZERO by Hardware reset.
					We search from Zero with 3 times loop

		In search, <i> the round index; <j> the group index, <k> in-group index, <step> is in group count, to retrive the correct Seqnum, at least loop 2 times.
	*/
	
	/* for none-crc mode, direct skip to step 2 */
	first_step = data->crc_enabled ? 0 : 2;
	/* Start check the Seq Num */
	for ( i = first_step; i < 3 && synced != SYNCED_COMPLETED; i ++ ){
		// 'I' Round, use overflow search or hardware reset 
		if (i == 0) {
			// <Round.0>: Assumed Seqnum change to `0` with unknown reason, so current is 2
			count = 1;
			step = 3;
			seqnum = 2;	// CRC missed, and one debouce, so start from will be 2
			seqnum_test = 0;
			dev_info(&client->dev,
				"Resync Round <I.0>: Assumed the seq round to 0, set (seq %d, step %d, distance %d)\n", seqnum, step, count);
			mxt_update_seq_num_lock(data, true, seqnum);
		}else if (i == 1) {
			// <Round.1>: use the overlfow method to retrieve the seq num, set `seqnum_last` + step
			count = 256;
			step = 8;
			seqnum = seqnum_last + step;
			seqnum_test = 0;
			dev_info(&client->dev,
				"Resync Round <I.1>: Using overflow to search seq, set (seq %d, step %d, distance %d)\n", seqnum, step, count);
			mxt_update_seq_num_lock(data, true, seqnum_last + step);
		} else {
			// <Round.2>: use Hardware reset to retrieve the seq number, set seq to 0
			count = 1;
			step = 3;	// try 3 times
			seqnum_test = 0;
			dev_info(&client->dev,
				"Resync Round <I.2>: Using hardware reset to sync\n");

			__mxt_reset(data, F_RST_HARD);
		}

		seqnum = mxt_curr_seq_num(data);
		for ( j = 0; j < count + step && synced != SYNCED_COMPLETED; j += step ) {
			// Use `step` to control step count of Seq number in `J` Round
			for ( k = 0; k < step; k++ ) {
				// Fix the Seq number in `K` Round		
				mxt_update_seq_num_lock(data, true, seqnum);

				/* 
					THe Seqnum verification flow chart:
						Is [Information block] in hand?:
							<B.1> Retrieve the ID information? ->
								<B.2> Retrieve the Information block (with offset)? ->
									Retrieved.
						[@ No]:
							<A> Retrieve the Information block with maximum length? -> 
								Is HA chip? ->
									<B.2> Retrieve the Information block (with offset)? ->
										Retrieved.
							[@ No]:
								End with Resync.
				*/
				/* If we have correct information block in hand, we can compare the buffer data with it directly. */
				if (info) {
					//  <Check Point B.1>
					if (memcmp(dev_id_buf, info, dev_id_size)) {
						// ID information mis-matched, read out ID information first
						reg = 0;
						size = dev_id_size;
						buf = dev_id_buf;
					} else {
						// ID information matched, to read out all left information block, will go to <Check Point B.2>
						num_objects = ((struct mxt_info *)info)->object_num;

						reg = dev_id_size;
						size = (num_objects * sizeof(struct mxt_object))
							+ MXT_INFO_CHECKSUM_SIZE;
						buf = dev_id_buf + dev_id_size;
					}
				} else {
					// <Check Point A> No information block yet, directly read out all information block with assumed size(maximum)
					reg = 0;
					size = info_block_size_max;
					buf = dev_id_buf;
				}

				dev_info(&client->dev,"Resync: Read Info block (%d, %zd): seq(%d) i(%d), j(%d), k(%d)\n", reg, size, mxt_curr_seq_num(data), i, j, k);
				// Start the read operation
				error = __mxt_read_reg_crc(client, reg, size, 
					buf, data, F_R_SEQ);
				if (error) {
					// The I2C communication encountered error, we will mark the counter and exit search of this round
					seqnum_test++;	// Marked invalid times
					dev_err(&client->dev,
						"Resync Read Info Block failed (I2C communication error?), Seqnum(%d)", mxt_curr_seq_num(data));
					
					msleep(200); // Slow down the re-trying speed if I2C error encounterred

					break;  // // End the K round since error
				} else {
					// The I2C communication valid, start to verify the data
					num_objects = ((struct mxt_info *)dev_id_buf)->object_num;

					info_block_size = dev_id_size + (num_objects * sizeof(struct mxt_object))
						+ MXT_INFO_CHECKSUM_SIZE;
					
					if (buf == dev_id_buf && size == dev_id_size) {
						// <Result B.1> verify the data
						if (info && !memcmp(dev_id_buf, info, dev_id_size)) {
							// <B.1> result is valid - ID information matched, loop again to read left information block part <B.2>
							print_hex_dump(KERN_DEBUG, "Resync Got ID Information: ", DUMP_PREFIX_NONE, 16, 1,
								dev_id_buf, dev_id_size, false);
							// The next loop will execute <B.2>
							seqnum += 1;	// Save the seqnum
							k -= 1;
						}
					} else {
						// <Result A or B.2>
						crc_ptr = dev_id_buf + info_block_size - MXT_INFO_CHECKSUM_SIZE;

						info_crc = crc_ptr[0] | (crc_ptr[1] << 8) | (crc_ptr[2] << 16);

						calculated_crc = mxt_calculate_crc(dev_id_buf, 0, info_block_size - MXT_INFO_CHECKSUM_SIZE);

						if (info_crc == calculated_crc) {
							// <Result A or B.2> result is valid
							dev_info(&client->dev,
								"Resync Info Block CRC Pass (%06X) info (%p)\n", info_crc, info);
							if (!info) {
								// <Result A> verified, first time to readout information block <B.2>
								if (mxt_lookup_ha_chips(dev_id_buf)) {
									// <A> found HA chips, save the info block valid and loop to check again
									dev_info(&client->dev, "Resync: Found HA chips\n");
									sbuf = kzalloc(info_block_size, GFP_KERNEL);
									if (!sbuf) {
										error = -ENOMEM;
										dev_err(&client->dev,
											"Resync Alloc Info Block memory(%zd) #2 failed", info_block_size);
										goto err_free_mem;
									}
									info = dev_id_buf;
									dev_id_buf = sbuf;
									// save the valid ID information for next around
									memcpy(dev_id_buf, info, dev_id_size);
									// We don't know it's by luck or seq num is actually synced
									// <Luck>: the sequm in chip is unknown and loop searching continued
									// <Actual>: the sequm number will be 1 more than current after this loop
									// We add 1 here for next seq num fixed beyond the chip num:
									seqnum += 1;
									k -= 1;
								} else {
									// <A> found non-HA chip, Resync is complete
									dev_info(&client->dev, "Resync Successfully(non-HA chips)\n");
									synced = SYNCED_COMPLETED;	// End resync at <Check Point A> (Non-HA)
									break;
								}
							} else {
								// <Result B.2> verified, second time to readout information block,  the Resync is completed
								seqnum_test  = (u16)seqnum + 256 - (j + k + 1) - seqnum_test;
								if (seqnum_test >= 256) {
									seqnum_test -= 256;
								}
								dev_info(&client->dev, "Resync Successfully (init %d, last %d, curr %d)\n", seqnum_test, seqnum_last, seqnum);
								synced = SYNCED_COMPLETED;	// End resync <Check Point B.2> (HA)
								break;
							}
						} else {
							// <Result A or B.2> invalid: Information block CRC check failed
							dev_info(&client->dev,
								"Resync Info Block(%zd) CRC error: Calculated(0x%06X) Read(0x%06X)\n",
									size, calculated_crc, data->info_crc);

							if ((size < info_block_size_max) && !k) {
								print_hex_dump(KERN_DEBUG, "Resync Info Block: ", DUMP_PREFIX_NONE, 16, 1,
									dev_id_buf, info_block_size, false);
							}

							synced = SYNCED_FAILED;
						}
					}
				}			
			}
		}
	}

	if (synced != SYNCED_COMPLETED) {
		dev_info(&client->dev,"Resync failed (%d)\n", synced);
		error = -EBUSY;
		goto err_free_mem;
	}

	error = 0;

err_free_mem:
	if (dev_id_buf) {
		kfree(dev_id_buf);
	}

	if (info) {
		if (info != data->info) {
			kfree(info);
		}
	}
	dev_info(&client->dev,"Resync: --\n");

	return error;
}

static int __mxt_read_info_block(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;
	size_t size;
	void *id_buf, *buf;
	uint8_t num_objects;
	u32 calculated_crc;
	u8 *crc_ptr;
	const struct mxt_info *info;

	/* If info block already allocated, free it */
	if (data->raw_info_block) {
		mxt_free_object_table(data);
	}

	/* Read 7-byte ID information block starting at address 0 */
	size = sizeof(struct mxt_info);
	id_buf = kzalloc(size, GFP_KERNEL);
	if (!id_buf) {
		dev_err(&client->dev,
			"Alloc ID infomation memory(%zd) failed", size);
		return -ENOMEM;
	}

	//Use master send and master receive, 8bit CRC is turned OFF
	error = __mxt_read_reg_crc(data->client, 0, size, id_buf, data, F_R_SEQ);
	if (error) {
		dev_err(&client->dev,
			"Read ID Infomation failed, Seqnum(%d)", mxt_curr_seq_num(data));
		goto err_free_mem;
	}

	//Local varible for debug output only
	info = (struct mxt_info *)id_buf;
	
	dev_info(&client->dev,
		 "Family: %u Variant: %u Firmware V%u.%u.%02X Objects: %u\n",
		 info->family_id, info->variant_id,
		 info->version >> 4, info->version & 0xf,
		 info->build, info->object_num);
	
	/* Resize buffer to give space for rest of info block */
	num_objects = ((struct mxt_info *)id_buf)->object_num;

	size += (num_objects * sizeof(struct mxt_object))
		+ MXT_INFO_CHECKSUM_SIZE;

	buf = krealloc(id_buf, size, GFP_KERNEL);
	if (!buf) {
		dev_err(&client->dev,
			"Alloc Info Block memory(%zd) failed", size);
		error = -ENOMEM;
		goto err_free_mem;
	}
	
	id_buf = buf;

	/* Read rest of info block after id block */
	error = __mxt_read_reg_crc(client, MXT_OBJECT_START, (size - MXT_OBJECT_START), 
		(id_buf + MXT_OBJECT_START), data, F_R_SEQ);

	if (error) {
		dev_err(&client->dev,
			"Read Info Block failed, Seqnum(%d)", mxt_curr_seq_num(data));
		goto err_free_mem;
	}

	/* Extract & calculate checksum */
	crc_ptr = id_buf + size - MXT_INFO_CHECKSUM_SIZE;

	data->info_crc = crc_ptr[0] | (crc_ptr[1] << 8) | (crc_ptr[2] << 16);

	calculated_crc = mxt_calculate_crc(id_buf, 0,
					   size - MXT_INFO_CHECKSUM_SIZE);

	dev_dbg(&client->dev, "Calculated crc %x\n", calculated_crc);
	print_hex_dump(KERN_DEBUG, "Info Block: ", DUMP_PREFIX_NONE, 16, 1,
		id_buf, size, false);
	/*
	 * CRC mismatch can be caused by data corruption due to I2C comms
	 * issue or else device is not using Object Based Protocol (eg i2c-hid)
	 */
	if ((data->info_crc == 0) || (data->info_crc != calculated_crc)) {
		dev_err(&client->dev,
			"Info Block CRC error calculated=0x%06X read=0x%06X\n",
			calculated_crc, data->info_crc);
		error = -EIO;
		goto err_free_mem;
	}

	data->raw_info_block = id_buf;
	data->info = (struct mxt_info *)id_buf;

	/* Parse object table information */
	error = mxt_parse_object_table(data, id_buf + MXT_OBJECT_START);
	if (error) {
		dev_err(&client->dev, "Error %d parsing object table\n", error);
		mxt_free_object_table(data);
		goto err_free_mem;
	}

	data->object_table = (struct mxt_object *)(id_buf + MXT_OBJECT_START);

	if (mxt_lookup_ha_chips(info)){
		dev_info(&client->dev, "Found HA chips\n");
	}

	return 0;

err_free_mem:
	kfree(id_buf);

	return error;
}

static int mxt_read_info_block(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error = 0;
	int i, retries = 2;

	/* read info block with retries */
	for ( i = 0; i < retries; i++) {
		error = __mxt_read_info_block(data);
		if (error) {
			dev_warn(&client->dev, "Read Info block check resync %d", i + 1);

			if (mxt_resync_comm(data)) {
				// resync failed directly exit
				dev_info(&client->dev, "Read Info block resync failed, exit");
				break;
			}
		} else {
			/* successful */
			break;
		}
	}

	return error;
}

static int mxt_read_t9_resolution(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;
	struct t9_range range;
	unsigned char orient;
	u8 xsize;
	struct mxt_object *object;

	object = mxt_get_object(data, MXT_TOUCH_MULTI_T9);
	if (!object)
		return -EINVAL;

	error = __mxt_read_reg(client,
			       object->start_address + MXT_T9_XSIZE,
			       sizeof(xsize), &xsize);
	if (error)
		return error;
	data->xsize = xsize ? xsize : 1; // For MPTT compatible which allows ZERO xsize

	error = __mxt_read_reg(client,
			       object->start_address + MXT_T9_YSIZE,
			       sizeof(data->ysize), &data->ysize);
	if (error)
		return error;

	error = __mxt_read_reg(client,
			       object->start_address + MXT_T9_RANGE,
			       sizeof(range), &range);
	if (error)
		return error;

	data->max_x = get_unaligned_le16(&range.x);
	data->max_y = get_unaligned_le16(&range.y);

	error =  __mxt_read_reg(client,
				object->start_address + MXT_T9_ORIENT,
				1, &orient);
	if (error)
		return error;

	data->xy_switch = orient & MXT_T9_ORIENT_SWITCH;
	data->invertx = orient & MXT_T9_ORIENT_INVERTX;
	data->inverty = orient & MXT_T9_ORIENT_INVERTY;

	return 0;
}

static int mxt_read_t15_num_keys_inst(struct mxt_data *data, u8 instance, unsigned int* nks)
{
	struct i2c_client *client = data->client;
	struct mxt_object *object;
	u8 xsize, ysize;
	u16 num_keys = 0, offset = 0;
	u8 T15_enable;
	int error;

	object = mxt_get_object(data, MXT_TOUCH_KEYARRAY_T15);
	if (!object) {
		return -EINVAL;
	}

	if (instance < data->T15_instances) {
		offset = mxt_obj_size(object) * instance;
		error = mxt_read_reg_auto(client,
			object->start_address + offset + MXT_T15_CTRL,
			sizeof(T15_enable), &T15_enable, data);
		if (error) {
			dev_err(&client->dev, "read T15 instance(%d) CTRL failed\n", instance);
			return error;
		}

		if ((T15_enable & MXT_T15_ENABLE_BIT_MASK) != 0x01 ){
			num_keys = 0;
			dev_info(&client->dev, "T15 instance(%d) input device not enabled\n", instance);
		} else {
			/* read first T15 size */
			error = mxt_read_reg_auto(client,
						object->start_address + offset + MXT_T15_XSIZE,
						sizeof(xsize), &xsize, data);
			if (error) {
				dev_err(&client->dev, "read T15 instance(%d) XSIZE failed\n", instance);
				return error;
			}

			error = mxt_read_reg_auto(client,
						object->start_address +  offset + MXT_T15_YSIZE,
						sizeof(ysize), &ysize, data);
			if (error) {
				dev_err(&client->dev, "read T15 instance(%d) YSIZE failed\n", instance);
				return error;
			}

			// For MPTT compatible which allows ZERO xsize
			xsize = xsize ? xsize : 1;

			num_keys = xsize * ysize;
			dev_info(&client->dev, "T15 instance(%d) has %d keys\n", instance, num_keys);
		}
		
		if (nks) {
			*nks = num_keys;
		}

		return 0;
	} else {
		dev_info(&client->dev, "T15 instance(%d) invalid\n", instance);
		return -ENODEV;
	}
}

static int mxt_set_up_active_stylus(struct input_dev *input_dev,
				    struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;
	struct mxt_object *object;
	u8 styaux;
	int aux;
	u8 ctrl;

	object = mxt_get_object(data, MXT_PROCI_ACTIVESTYLUS_T107);
	if (!object)
		return 0;

	error = __mxt_read_reg(client, object->start_address, 1, &ctrl);
	if (error)
		return error;

	/* Check enable bit */
	if (!(ctrl & 0x01))
		return 0;

	error = __mxt_read_reg(client,
			       object->start_address + MXT_T107_STYLUS_STYAUX,
			       1, &styaux);
	if (error)
		return error;

	/* map aux bits */
	aux = 7;

	if (styaux & MXT_T107_STYLUS_STYAUX_PRESSURE)
		data->stylus_aux_pressure = aux++;

	if (styaux & MXT_T107_STYLUS_STYAUX_PEAK)
		data->stylus_aux_peak = aux++;

	input_set_capability(input_dev, EV_KEY, BTN_STYLUS);
	input_set_capability(input_dev, EV_KEY, BTN_STYLUS2);
	input_set_abs_params(input_dev, ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0);

	dev_dbg(&client->dev,
		"T107 active stylus, aux map pressure:%u peak:%u\n",
		data->stylus_aux_pressure, data->stylus_aux_peak);

	return 0;
}

static int mxt_read_t100_config(struct mxt_data *data, u8 instance)
{
	struct i2c_client *client = data->client;
	int error;
	struct mxt_object *object;
	u16 range_x, range_y;
	u8 cfg, tchaux;
	u8 aux;
	u16 obj_size = 0;
	u8 T100_enable;

	object = mxt_get_object(data, MXT_TOUCH_MULTITOUCHSCREEN_T100);
	if (!object)
		return -EINVAL;
	
	if (instance == 2) {
		T100_enable = object->start_address + mxt_obj_size(object) + MXT_T100_CTRL;
		
		if ((T100_enable & MXT_T100_ENABLE_BIT_MASK) == 0x01 ){
			obj_size = mxt_obj_size(object);
		} else {
			dev_info(&client->dev, "T100 secondary input device not enabled\n");
			
			return 1;
		}	
	}
	
	/* read touchscreen dimensions */
	error = mxt_read_reg_auto(client,
			       object->start_address + obj_size + MXT_T100_XRANGE,
			       sizeof(range_x), &range_x, data);
	if (error)
		return error;

	data->max_x = get_unaligned_le16(&range_x);

	error = mxt_read_reg_auto(client,
			       object->start_address + obj_size + MXT_T100_YRANGE,
			       sizeof(range_y), &range_y, data);
	if (error)
		return error;

	data->max_y = get_unaligned_le16(&range_y);

	error = mxt_read_reg_auto(client,
			       object->start_address + obj_size + MXT_T100_XSIZE,
			       sizeof(data->xsize), &data->xsize, data);
	if (error)
		return error;

	error = mxt_read_reg_auto(client,
			       object->start_address + obj_size + MXT_T100_YSIZE,
			       sizeof(data->ysize), &data->ysize, data);
	if (error)
		return error;

	/* read orientation config */
	error =  mxt_read_reg_auto(client,
				object->start_address + obj_size + MXT_T100_CFG1,
				1, &cfg, data);
	if (error)
		return error;


	data->xy_switch = cfg & MXT_T100_CFG_SWITCHXY;
	data->invertx = cfg & MXT_T100_CFG_INVERTX;
	data->inverty = cfg & MXT_T100_CFG_INVERTY;

	/* allocate aux bytes */
	error =  mxt_read_reg_auto(client,
			object->start_address + obj_size+ MXT_T100_TCHAUX,
			1, &tchaux, data);
	if (error)
		return error;

	aux = MXT_T100_AUX_OFFSET;

	if (tchaux & MXT_T100_TCHAUX_VECT)
		data->t100_aux_vect = aux++;

	if (tchaux & MXT_T100_TCHAUX_AMPL)
		data->t100_aux_ampl = aux++;

	if (tchaux & MXT_T100_TCHAUX_AREA)
		data->t100_aux_area = aux++;

	dev_dbg(&client->dev,
		"T100 aux mappings vect:%u ampl:%u area:%u\n",
		data->t100_aux_vect, data->t100_aux_ampl, data->t100_aux_area);
				
	return 0;
}

#ifdef CONFIG_ACPI
static int mxt_input_open(struct input_dev *dev);
static void mxt_input_close(struct input_dev *dev);
#endif

static void mxt_set_up_as_touchpad(struct input_dev *input_dev,
				   struct mxt_data *data)
{
	int i;

	input_dev->name = "Atmel maXTouch Touchpad";

	__set_bit(INPUT_PROP_BUTTONPAD, input_dev->propbit);
	input_abs_set_res(input_dev, ABS_X, MXT_PIXELS_PER_MM);
	input_abs_set_res(input_dev, ABS_Y, MXT_PIXELS_PER_MM);
	input_abs_set_res(input_dev, ABS_MT_POSITION_X,
			  MXT_PIXELS_PER_MM);
	input_abs_set_res(input_dev, ABS_MT_POSITION_Y,
			  MXT_PIXELS_PER_MM);
	for (i = 0; i < data->t19_num_keys; i++)
		if (data->t19_keymap[i] != KEY_RESERVED)
			input_set_capability(input_dev, EV_KEY,
					     data->t19_keymap[i]);
}

static struct input_dev * mxt_initialize_input_device(struct mxt_data *data, bool primary)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev;
	int error;
	unsigned int num_mt_slots = 0;
	unsigned int mt_flags;
	int i;

	switch (data->multitouch) {
	case MXT_TOUCH_MULTI_T9:
		num_mt_slots = data->T9_reportid_max - data->T9_reportid_min + 1;
		error = mxt_read_t9_resolution(data);
		if (error)
			dev_warn(dev, "Failed to initialize T9 resolution\n");
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T100:
		num_mt_slots = (data->num_touchids);
		error = mxt_read_t100_config(data, 1);
		if (error)
			dev_warn(dev, "Failed to read T100 config\n");
		break;
	case MXT_TOUCH_KEYARRAY_T15:
		break;
	default:
		dev_err(dev, "Invalid multitouch object\n");
		return NULL;
	}

	/* Handle default values and orientation switch */
	if (data->max_x == 0)
		data->max_x = 1023;

	if (data->max_y == 0)
		data->max_y = 1023;

	if (data->xy_switch)
		swap(data->max_x, data->max_y);

	dev_info(dev, "Touchscreen size {x,y} = {%u,%u}\n", data->max_x, data->max_y);

	/* Register input device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(dev, "No memory for Allocating Input Device\n");
		return NULL;
	}

	input_dev->name = primary ? "Atmel maXTouch Touchscreen" : "maXTouch Secondary Touchscreen";
	input_dev->phys = data->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = dev;
#ifdef CONFIG_ACPI
	input_dev->open = mxt_input_open;
	input_dev->close = mxt_input_close;
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	set_bit(EV_ABS, input_dev->evbit);
#endif

#ifdef CONFIG_INPUT_DEVICE2_SINGLE_TOUCH
	// For single touch //
	input_set_capability(input_dev, EV_KEY, BTN_TOUCH);
	input_set_abs_params(input_dev, ABS_X, 0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, data->max_y, 0, 0);
#endif
	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
	    (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	     data->t100_aux_ampl)) {
		input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	}

	/* If device has buttons we assume it is a touchpad */
	if (primary && data->t19_num_keys) {
		mxt_set_up_as_touchpad(input_dev, data);
		mt_flags = INPUT_MT_POINTER;
	} else {
		mt_flags = INPUT_MT_DIRECT;
	}

	/* For multi touch */
	if (num_mt_slots) {
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
		error = input_mt_init_slots(input_dev, num_mt_slots, mt_flags);
	#else
		if (mt_flags == INPUT_MT_DIRECT) {
			__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
		}
		error = input_mt_init_slots(input_dev, num_mt_slots);
	#endif
		if (error) {
			dev_err(dev, "Error %d initialising slots\n", error);
			goto err_free_mem;
		}
	}

	if (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100) {
		input_set_abs_params(input_dev, ABS_MT_TOOL_TYPE,
					0, MT_TOOL_MAX, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_DISTANCE,
					MXT_DISTANCE_ACTIVE_TOUCH,
					MXT_DISTANCE_HOVERING,
					0, 0);
	}

	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
	    	(data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100)) {
		input_set_abs_params(input_dev, ABS_MT_POSITION_X,
					0, data->max_x, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
					0, data->max_y, 0, 0);
	}

	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
		(data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
		data->t100_aux_area)) {
		input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
					0, MXT_MAX_AREA, 0, 0);
	}

	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
		(data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
		data->t100_aux_ampl)) {
		input_set_abs_params(input_dev, ABS_MT_PRESSURE,
					0, 255, 0, 0);
	}

	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
		(data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
		data->t100_aux_vect)) {
		input_set_abs_params(input_dev, ABS_MT_ORIENTATION,
					0, 255, 0, 0);
	}

	if (primary) {
		/* For T107 Active Stylus */
		if (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
			data->T107_address) {
			error = mxt_set_up_active_stylus(input_dev, data);
			if (error)
				dev_warn(dev, "Failed to read T107 config\n");
		}

		/* For T15 Key Array */
		if (data->T15_reportid_min) {
			data->t15_keystatus = 0;
			if (data->t15_num_keys) {
				error = mxt_read_t15_num_keys_inst(data, 0, &data->t15_num_keys_inst0);
				if (error) {
					// Set key to num DTS defined keys.
					data->t15_num_keys_inst0 = data->t15_num_keys;
					dev_warn(dev, "Failed get t15 instance(0) numkeys, set to DTS default value %d\n", 
						data->t15_num_keys_inst0);
				}

				dev_info(dev, "T15 instance(0) got %d keys and %d registerred\n", 
						data->t15_num_keys_inst0, data->t15_num_keys);

				for (i = 0; i < data->t15_num_keys; i++) {
					if (data->t15_keymap[i]) { 
						input_set_capability(input_dev, EV_KEY,
								data->t15_keymap[i]);
					}
				}
			}
		}
	}
	input_set_drvdata(input_dev, data);

	error = input_register_device(input_dev);
	if (error) {
		dev_err(dev, "Error %d registering input device\n", error);
		goto err_free_mem;
	}

	return input_dev;

err_free_mem:
	input_free_device(input_dev);
	return NULL;
}

static int mxt_sysfs_init(struct mxt_data *data);
static void mxt_sysfs_remove(struct mxt_data *data);
static int mxt_configure_objects(struct mxt_data *data,
				 const struct firmware *cfg);
static void mxt_debug_init(struct mxt_data *data);

static void mxt_config_cb(const struct firmware *cfg, void *ctx)
{
	struct mxt_data *data = (struct mxt_data *)ctx;

	mutex_lock(&data->update_lock);

	mxt_configure_objects(ctx, cfg);
	release_firmware(cfg);

	mutex_unlock(&data->update_lock);
}

static int mxt_initialize(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int recovery_attempts = 0;
	int error;

	while (1) {
		error = mxt_read_info_block(data);
		if (!error) {
			dev_dbg(&client->dev, "Read Info Block success\n");
			break;
		} else {
			dev_err(&client->dev, "Read Info Block failed(%d), may check bootloader mode later.\n", error);
		}

		if (!data->crc_enabled) {
		/* Check bootloader state */
			error = mxt_probe_bootloader(data, false);
			if (error) {
				dev_info(&client->dev, "Trying alternate bootloader address\n");
				error = mxt_probe_bootloader(data, true);
				if (error) {
					/* Chip is not in appmode or bootloader mode */
					return error;
				}
			}

			/* OK, we are in bootloader, see if we can recover */
			if (++recovery_attempts > 1) {
				dev_err(&client->dev, "Could not recover from bootloader mode\n");
				/*
				* We can reflash from this state, so do not
				* abort initialization.
				*/
				data->in_bootloader = true;
				return 0;
			}

			/* Attempt to exit bootloader into app mode */
			mxt_send_bootloader_cmd(data, false);
			msleep(MXT_FW_RESET_TIME);
		}
	}

	error = mxt_acquire_irq(data);
	if (error) {
		dev_err(&client->dev, "Acquire irq %d failed(%d)\n", data->irq, error);
		return error;
	}
	
	if (true){
		/* As built-in driver, root filesystem may not be available yet */
		error = request_firmware_nowait(THIS_MODULE, true, MXT_CFG_NAME,
						&client->dev, GFP_KERNEL, data,
						mxt_config_cb);
		if (error) {
			dev_warn(&client->dev, "Failed to invoke firmware loader: %d\n",
				error);
		}
	} else {
		mutex_lock(&data->update_lock);

		mxt_configure_objects(data, NULL);

		mutex_unlock(&data->update_lock);
	}

	return 0;
}

static int mxt_set_t7_power_cfg(struct mxt_data *data, u8 sleep)
{
	struct device *dev = &data->client->dev;
	int error;
	struct t7_config *new_config;
	struct t7_config deepsleep = { .active = 0, .idle = 0 };

	if (!data->T7_address)
		return 0;

	if (sleep == MXT_POWER_CFG_DEEPSLEEP)
		new_config = &deepsleep;
	else
		new_config = &data->t7_cfg;

	error = mxt_write_reg_auto(data->client, data->T7_address,
		sizeof(data->t7_cfg), new_config, data);
	if (error)
		return error;

	dev_dbg(dev, "Set T7 ACTV:%d IDLE:%d\n",	
		new_config->active, new_config->idle);

	return 0;
}

static int mxt_init_t7_power_cfg(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct t7_config t7_cfg;
	int error;
	bool retry = false;

	if (!data->T7_address)
		return 0;

recheck:
	error = mxt_read_reg_auto(data->client, data->T7_address,
			sizeof(t7_cfg), &t7_cfg, data);

	if (error)
		return error;

	if (t7_cfg.active == 0 || t7_cfg.idle == 0) {
		if (!retry) {
			dev_info(dev, "T7 cfg zero, resetting\n");
			__mxt_reset(data, F_RST_SOFT);
			retry = true;
			goto recheck;
		} else {
			dev_info(dev, "T7 cfg zero after reset, overriding\n");
			if (data->t7_cfg.active == 0 || data->t7_cfg.idle == 0) { 	// Try lastest t7_cfg first
				data->t7_cfg.active = 20;
				data->t7_cfg.idle = 100;
			}
			return mxt_set_t7_power_cfg(data, MXT_POWER_CFG_RUN);
		}
	} else {
		memcpy(&data->t7_cfg, &t7_cfg, sizeof(t7_cfg));
	}

	dev_info(dev, "Initialized power cfg: ACTV %d, IDLE %d\n",
		data->t7_cfg.active, data->t7_cfg.idle);
	return 0;
}

static int mxt_set_selftest(struct mxt_data *data, u8 cmd, bool wait)
{
	struct device *dev = &data->client->dev;
	u16 reg;
	int timeout_counter = 0;
	int ret;
	u8  val;

	// Clear on demand test cache
	if (data->msg_cache) {
		CLR_ONE_MESSAGE_CACHE(data->msg_cache, data->T5_msg_size, MESSAGE_CACHE_TEST_ON_DEMAND);
	}

	if (data->T25_address) {
		reg = data->T25_address;
	} else if (data->T10_address) {
		reg = data->T10_address;
	} else {
		dev_err(dev, "No Selftest Object found");
		return -EEXIST;
	}

	val = cmd;
	ret = mxt_write_reg_auto(data->client, reg + MXT_SELFTEST_CMD, sizeof(val), &val, data);
	if (ret) {
		dev_err(dev, "Send Test Command %02x failed\n", cmd);
		return ret;
	}

	if (!wait) {
		return 0;
	}

	do {

		msleep(MXT_SELFTEST_TIME);
		ret = mxt_read_reg_auto(data->client, reg + MXT_SELFTEST_CMD, 1, &val, data);
		if (ret) {
			dev_err(dev, "Read Test Command %02x failed\n", cmd);
			return ret;
		}

	} while ((val != 0) && (timeout_counter++ <= 100));

	if (timeout_counter > 100) {
		dev_err(dev, "Test Command Timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

#ifdef CONFIG_TOUCHSCREEN_MICROCHIP_MXT_DEBUG
static u16 mxt_get_debug_value(struct mxt_data *data, unsigned int x,
			       unsigned int y)
{
	struct mxt_info *info = data->info;
	struct mxt_dbg *dbg = &data->dbg;
	unsigned int ofs, page;
	unsigned int col = 0;
	unsigned int col_width;

	if (info->family_id == MXT_FAMILY_1386) {
		col_width = info->matrix_ysize / MXT1386_COLUMNS;
		col = y / col_width;
		y = y % col_width;
	} else {
		col_width = info->matrix_ysize;
	}

	ofs = (y + (x * col_width)) * sizeof(u16);
	page = ofs / MXT_DIAGNOSTIC_SIZE;
	ofs %= MXT_DIAGNOSTIC_SIZE;

	if (info->family_id == MXT_FAMILY_1386)
		page += col * MXT1386_PAGES_PER_COLUMN;

	return get_unaligned_le16(&dbg->t37_buf[page].data[ofs]);
}

static int mxt_convert_debug_pages(struct mxt_data *data, u16 *outbuf)
{
	struct mxt_dbg *dbg = &data->dbg;
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int i, rx, ry;

	for (i = 0; i < dbg->t37_nodes; i++) {
		/* Handle orientation */
		rx = data->xy_switch ? y : x;
		ry = data->xy_switch ? x : y;
		rx = data->invertx ? (data->xsize - 1 - rx) : rx;
		ry = data->inverty ? (data->ysize - 1 - ry) : ry;

		outbuf[i] = mxt_get_debug_value(data, rx, ry);

		/* Next value */
		if (++x >= (data->xy_switch ? data->ysize : data->xsize)) {
			x = 0;
			y++;
		}
	}

	return 0;
}

static int mxt_read_diagnostic_debug(struct mxt_data *data, u8 mode,
				     u16 *outbuf)
{
	struct mxt_dbg *dbg = &data->dbg;
	int retries = 0;
	int page;
	int ret;
	u8 cmd = mode;
	struct t37_debug *p;
	u8 cmd_poll;

	for (page = 0; page < dbg->t37_pages; page++) {
		p = dbg->t37_buf + page;

		ret = mxt_write_reg(data->client, dbg->diag_cmd_address,
				    cmd);
		if (ret)
			return ret;

		retries = 0;
		msleep(20);
wait_cmd:
		/* Read back command byte */
		ret = __mxt_read_reg(data->client, dbg->diag_cmd_address,
				     sizeof(cmd_poll), &cmd_poll);
		if (ret)
			return ret;

		/* Field is cleared once the command has been processed */
		if (cmd_poll) {
			if (retries++ > 100)
				return -EINVAL;

			msleep(20);
			goto wait_cmd;
		}

		/* Read T37 page */
		ret = __mxt_read_reg(data->client, dbg->t37_address,
				     sizeof(struct t37_debug), p);
		if (ret)
			return ret;

		if (p->mode != mode || p->page != page) {
			dev_err(&data->client->dev, "T37 page mismatch\n");
			return -EINVAL;
		}

		dev_dbg(&data->client->dev, "%s page:%d retries:%d\n",
			__func__, page, retries);

		/* For remaining pages, write PAGEUP rather than mode */
		cmd = MXT_DIAGNOSTIC_PAGEUP;
	}

	return mxt_convert_debug_pages(data, outbuf);
}

static int mxt_queue_setup(struct vb2_queue *q,
		       unsigned int *nbuffers, unsigned int *nplanes,
		       unsigned int sizes[], struct device *alloc_devs[])
{
	struct mxt_data *data = q->drv_priv;
	size_t size = data->dbg.t37_nodes * sizeof(u16);

	if (*nplanes)
		return sizes[0] < size ? -EINVAL : 0;

	*nplanes = 1;
	sizes[0] = size;

	return 0;
}

static void mxt_buffer_queue(struct vb2_buffer *vb)
{
	struct mxt_data *data = vb2_get_drv_priv(vb->vb2_queue);
	u16 *ptr;
	int ret;
	u8 mode;

	ptr = vb2_plane_vaddr(vb, 0);
	if (!ptr) {
		dev_err(&data->client->dev, "Error acquiring frame ptr\n");
		goto fault;
	}

	switch (data->dbg.input) {
	case MXT_V4L_INPUT_DELTAS:
	default:
		mode = MXT_DIAGNOSTIC_DELTAS;
		break;

	case MXT_V4L_INPUT_REFS:
		mode = MXT_DIAGNOSTIC_REFS;
		break;
	}

	ret = mxt_read_diagnostic_debug(data, mode, ptr);
	if (ret)
		goto fault;

	vb2_set_plane_payload(vb, 0, data->dbg.t37_nodes * sizeof(u16));
	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
	return;

fault:
	vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
}

/* V4L2 structures */
static const struct vb2_ops mxt_queue_ops = {
	.queue_setup		= mxt_queue_setup,
	.buf_queue		= mxt_buffer_queue,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
};

static const struct vb2_queue mxt_queue = {
	.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	.io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ,
	.buf_struct_size = sizeof(struct mxt_vb2_buffer),
	.ops = &mxt_queue_ops,
	.mem_ops = &vb2_vmalloc_memops,
	.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC,
	.min_buffers_needed = 1,
};

static int mxt_vidioc_querycap(struct file *file, void *priv,
				 struct v4l2_capability *cap)
{
	struct mxt_data *data = video_drvdata(file);

	strlcpy(cap->driver, "atmel_mxt_ts", sizeof(cap->driver));
	strlcpy(cap->card, "atmel_mxt_ts touch", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "I2C:%s", dev_name(&data->client->dev));
	return 0;
}

static int mxt_vidioc_enum_input(struct file *file, void *priv,
				   struct v4l2_input *i)
{
	if (i->index >= MXT_V4L_INPUT_MAX)
		return -EINVAL;

	i->type = V4L2_INPUT_TYPE_TOUCH;

	switch (i->index) {
	case MXT_V4L_INPUT_REFS:
		strlcpy(i->name, "Mutual Capacitance References",
			sizeof(i->name));
		break;
	case MXT_V4L_INPUT_DELTAS:
		strlcpy(i->name, "Mutual Capacitance Deltas", sizeof(i->name));
		break;
	}

	return 0;
}

static int mxt_set_input(struct mxt_data *data, unsigned int i)
{
	struct v4l2_pix_format *f = &data->dbg.format;

	if (i >= MXT_V4L_INPUT_MAX)
		return -EINVAL;

	if (i == MXT_V4L_INPUT_DELTAS)
		f->pixelformat = V4L2_TCH_FMT_DELTA_TD16;
	else
		f->pixelformat = V4L2_TCH_FMT_TU16;

	f->width = data->xy_switch ? data->ysize : data->xsize;
	f->height = data->xy_switch ? data->xsize : data->ysize;
	f->field = V4L2_FIELD_NONE;
	f->colorspace = V4L2_COLORSPACE_RAW;
	f->bytesperline = f->width * sizeof(u16);
	f->sizeimage = f->width * f->height * sizeof(u16);

	data->dbg.input = i;

	return 0;
}

static int mxt_vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	return mxt_set_input(video_drvdata(file), i);
}

static int mxt_vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct mxt_data *data = video_drvdata(file);

	*i = data->dbg.input;

	return 0;
}

static int mxt_vidioc_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct mxt_data *data = video_drvdata(file);

	f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	f->fmt.pix = data->dbg.format;

	return 0;
}

static int mxt_vidioc_enum_fmt(struct file *file, void *priv,
				 struct v4l2_fmtdesc *fmt)
{
	if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	switch (fmt->index) {
	case 0:
		fmt->pixelformat = V4L2_TCH_FMT_TU16;
		break;

	case 1:
		fmt->pixelformat = V4L2_TCH_FMT_DELTA_TD16;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int mxt_vidioc_g_parm(struct file *file, void *fh,
			     struct v4l2_streamparm *a)
{
	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	a->parm.capture.readbuffers = 1;
	a->parm.capture.timeperframe.numerator = 1;
	a->parm.capture.timeperframe.denominator = 10;
	return 0;
}

static const struct v4l2_ioctl_ops mxt_video_ioctl_ops = {
	.vidioc_querycap        = mxt_vidioc_querycap,

	.vidioc_enum_fmt_vid_cap = mxt_vidioc_enum_fmt,
	.vidioc_s_fmt_vid_cap   = mxt_vidioc_fmt,
	.vidioc_g_fmt_vid_cap   = mxt_vidioc_fmt,
	.vidioc_try_fmt_vid_cap	= mxt_vidioc_fmt,
	.vidioc_g_parm		= mxt_vidioc_g_parm,

	.vidioc_enum_input      = mxt_vidioc_enum_input,
	.vidioc_g_input         = mxt_vidioc_g_input,
	.vidioc_s_input         = mxt_vidioc_s_input,

	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	.vidioc_create_bufs     = vb2_ioctl_create_bufs,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	.vidioc_expbuf          = vb2_ioctl_expbuf,

	.vidioc_streamon        = vb2_ioctl_streamon,
	.vidioc_streamoff       = vb2_ioctl_streamoff,
};

static const struct video_device mxt_video_device = {
	.name = "Atmel maxTouch",
	.fops = &mxt_video_fops,
	.ioctl_ops = &mxt_video_ioctl_ops,
	.release = video_device_release_empty,
	.device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_TOUCH |
		       V4L2_CAP_READWRITE | V4L2_CAP_STREAMING,
};

static void mxt_debug_init(struct mxt_data *data)
{
	struct mxt_info *info = data->info;
	struct mxt_dbg *dbg = &data->dbg;
	struct mxt_object *object;
	int error;

	object = mxt_get_object(data, MXT_GEN_COMMAND_T6);
	if (!object)
		goto error;

	dbg->diag_cmd_address = object->start_address + MXT_COMMAND_DIAGNOSTIC;

	object = mxt_get_object(data, MXT_DEBUG_DIAGNOSTIC_T37);
	if (!object)
		goto error;

	if (mxt_obj_size(object) != sizeof(struct t37_debug)) {
		dev_warn(&data->client->dev, "Bad T37 size");
		goto error;
	}

	dbg->t37_address = object->start_address;

	/* Calculate size of data and allocate buffer */
	dbg->t37_nodes = data->xsize * data->ysize;

	if (info->family_id == MXT_FAMILY_1386)
		dbg->t37_pages = MXT1386_COLUMNS * MXT1386_PAGES_PER_COLUMN;
	else
		dbg->t37_pages = DIV_ROUND_UP(data->xsize *
					      info->matrix_ysize *
					      sizeof(u16),
					      sizeof(dbg->t37_buf->data));

	dbg->t37_buf = devm_kmalloc_array(&data->client->dev, dbg->t37_pages,
					  sizeof(struct t37_debug), GFP_KERNEL);
	if (!dbg->t37_buf)
		goto error;

	/* init channel to zero */
	mxt_set_input(data, 0);

	/* register video device */
	snprintf(dbg->v4l2.name, sizeof(dbg->v4l2.name), "%s", "atmel_mxt_ts");
	error = v4l2_device_register(&data->client->dev, &dbg->v4l2);
	if (error)
		goto error;

	/* initialize the queue */
	mutex_init(&dbg->lock);
	dbg->queue = mxt_queue;
	dbg->queue.drv_priv = data;
	dbg->queue.lock = &dbg->lock;
	dbg->queue.dev = &data->client->dev;

	error = vb2_queue_init(&dbg->queue);
	if (error)
		goto error_unreg_v4l2;

	dbg->vdev = mxt_video_device;
	dbg->vdev.v4l2_dev = &dbg->v4l2;
	dbg->vdev.lock = &dbg->lock;
	dbg->vdev.vfl_dir = VFL_DIR_RX;
	dbg->vdev.queue = &dbg->queue;
	video_set_drvdata(&dbg->vdev, data);

	error = video_register_device(&dbg->vdev, VFL_TYPE_TOUCH, -1);
	if (error)
		goto error_unreg_v4l2;

	return;

error_unreg_v4l2:
	v4l2_device_unregister(&dbg->v4l2);
error:
	dev_warn(&data->client->dev, "Error initializing T37\n");
}

static void mxt_debug_deinit(struct mxt_data *data)
{
	#error "Not implemented yet!"
}
#else
static void mxt_debug_init(struct mxt_data *data)
{
}
/*
static void mxt_debug_deinit(struct mxt_data *data)
{
}
*/
#endif

static void
mxt_ts_prepare_debugfs(struct mxt_data *data, const char *debugfs_name) {

	data->debug_dir = debugfs_create_dir(debugfs_name, NULL);
	if (!data->debug_dir) {
		return;
	}

	debugfs_create_x8("tx_seq_num", S_IRUGO | S_IWUSR, data->debug_dir, &data->msg_num.txseq_num);
	debugfs_create_atomic_t("debug_irq", S_IRUGO | S_IWUSR, data->debug_dir, &data->irq_processing);
	debugfs_create_bool("crc_enabled", S_IRUGO, data->debug_dir, &data->crc_enabled);
}

static void
mxt_ts_teardown_debugfs(struct mxt_data *data)
{
	if (data->debug_dir) {
		debugfs_remove_recursive(data->debug_dir);
		data->debug_dir = NULL;
	}
}


static void mxt_free_input_device(struct mxt_data *data);
static void mxt_free_second_input_device(struct mxt_data *data);
static int mxt_configure_objects(struct mxt_data *data,
				 const struct firmware *cfg)
{
	struct device *dev = &data->client->dev;
	int error;

	if (cfg) {
		error = mxt_update_cfg(data, cfg);
		if (error < 0) {
			dev_warn(dev, "Error %d updating config\n", error);
		} else if (error == 0) {
			dev_info(dev, "Skip update config file\n");
		} else {
			dev_info(dev, "Config file updated, release the input device\n");
			mxt_free_input_device(data);
			mxt_free_second_input_device(data);
		}
	}

	if (!data->input_dev) {	// Check the major device only
		if (data->multitouch) {
			dev_info(dev, "mxt_config: Registering devices\n");
			data->input_dev = mxt_initialize_input_device(data, true);
			if (!data->input_dev) {
				dev_warn(dev, "Error to Register primary device\n");
				return -EEXIST;
			}
			
			if (data->T100_instances > 1) {
			    data->input_dev_sec = mxt_initialize_input_device(data, false);
			    if (!data->input_dev_sec) {
				    dev_warn(dev, "Error ot Register secondary device\n");
				}
			}
		} else {
			dev_warn(dev, "No touch object detected\n");
		}

		// FIXME: when should call mxt_debug_deinit()
		mxt_debug_init(data);
	}

	/* T7 config may have changed */
	mxt_init_t7_power_cfg(data);

	/* check T18 retrigen bit with irqflags */
	error = mxt_check_retrigen(data);
	if (error) {
		dev_warn(dev, "RETRIGEN Not Enabled or unavailable\n");
	}

	return 0;
}

/* Configuration crc check sum is returned as hex xxxxxx */
static ssize_t mxt_config_crc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%06x\n", data->config_crc);
}

/* Firmware Version is returned as Major.Minor.Build */
ssize_t mxt_fw_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_info *info = data->info;
	return scnprintf(buf, PAGE_SIZE, "%u.%u.%02X\n",
			 info->version >> 4, info->version & 0xf, info->build);
}

static ssize_t mxt_tx_seq_number_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret;

	if (kstrtou8(buf, 0, &i) == 0) {
		dev_dbg(dev, "TX seq_num = %d\n", i);
		if (i <= 255) {
			mxt_update_seq_num_lock(data, true, i);
		} else {
			mxt_resync_comm(data);
		}
		ret = count;
	} else {
		dev_dbg(dev, "Tx seq number write error\n");
		ret = -EINVAL;
	}

	return ret;
}

/* Returns current tx_seq number */
static ssize_t mxt_tx_seq_number_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "%x\n", data->msg_num.txseq_num);
}

/* Hardware Version is returned as FamilyID.VariantID */
static ssize_t mxt_hw_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_info *info = data->info;
	return scnprintf(buf, PAGE_SIZE, "%u.%u\n",
			 info->family_id, info->variant_id);
}

static ssize_t mxt_cfg_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 val[16];
	int i;
	int ret, offset = 0;
	
	if (!data->T38_address)
		return -EEXIST;

	ret = mxt_read_reg_auto(data->client,
			data->T38_address, sizeof(val), val, data);
	if (ret) {
		dev_err(dev, "Read T38 data failed %d\n", ret);
		return ret;
	}

	print_hex_dump(KERN_DEBUG, "T38 data: ", DUMP_PREFIX_NONE, 16, 1,
			val, sizeof(val), false);

	for (i = 0; i < sizeof(val); i++) {
		offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%02hhx ", val[i]);
	}

	buf[offset] = '\n';
	offset++;
	
	return offset;
}

static ssize_t mxt_show_instance(char *buf, int count,
				 struct mxt_object *object, int instance,
				 const u8 *val)
{
	int i;

	if (mxt_obj_instances(object) > 1)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "Inst: %u\n", instance);

	for (i = 0; i < mxt_obj_size(object); i++)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				"%02x ",val[i]);

	count += scnprintf(buf + count, PAGE_SIZE - count, "\n");

	return count;
}

static ssize_t mxt_object_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_object *object;
	int count = 0;
	int i, j;
	int error;
	u8 *obuf;

	/* Pre-allocate buffer large enough to hold max sized object. */
	obuf = kmalloc(256, GFP_KERNEL);
	if (!obuf)
		return -ENOMEM;

	error = 0;
	for (i = 0; i < data->info->object_num; i++) {

		object = data->object_table + i;

		if (!mxt_object_readable(object->type))
			continue;

		count += scnprintf(buf + count, PAGE_SIZE - count,
				"T%u:\n", object->type);

		for (j = 0; j < mxt_obj_instances(object); j++) {
			u16 size = mxt_obj_size(object);
			u16 addr = object->start_address + j * size;
			
			error = mxt_read_reg_auto(data->client, addr, size, obuf, data);
			if (error)
				goto done;

			count = mxt_show_instance(buf, count, object, j, obuf);
		}
	}

done:
	kfree(obuf);
	return error ?: count;
}

static int mxt_check_firmware_format(struct device *dev,
				     const struct firmware *fw)
{
	unsigned int pos = 0;
	char c;

	while (pos < fw->size) {
		c = *(fw->data + pos);

		if (c < '0' || (c > '9' && c < 'A') || c > 'F')
			return 0;

		pos++;
	}

	/*
	 * To convert file try:
	 * xxd -r -p mXTXXX__APP_VX-X-XX.enc > maxtouch.fw
	 */
	dev_err(dev, "Aborting: firmware file must be in binary format\n");

	return -EINVAL;
}

static int mxt_load_fw(struct device *dev, const char *fn)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	unsigned int retry = 0;
	unsigned int frame = 0;
	int ret;

	ret = request_firmware(&fw, fn, dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		return ret;
	} else {
		dev_info(dev, "Opened firmware file: %s\n", fn);
	}

	/* Check for incorrect enc file */
	ret = mxt_check_firmware_format(dev, fw);

	if (ret) {
		goto release_firmware;
	} else {
		dev_info(dev, "File format is okay\n");		
	}

	if (!data->in_bootloader) {
		/* Change to the bootloader mode */
		data->in_bootloader = true;

		ret = mxt_t6_command(data, MXT_COMMAND_RESET,
				     MXT_BOOT_VALUE, false);
		if (ret) {
			goto release_firmware;
		} else {
			dev_info(dev, "Sent bootloader command.\n");
		}

		// Reset command will make the device seq number to ZERO
		// Note: this mostly won't be a thread sync issue because if reset finished, it enterred bootloader mode without seq num needed
		// We don't use lock around the reset command because it's in bootloader now
		mxt_update_seq_num_lock(data, true, 0x00);

		msleep(MXT_RESET_TIME);

		/* Do not need to scan since we know family ID */
		ret = mxt_lookup_bootloader_address(data, 0);
		if (ret) {
			goto release_firmware;
		} else {
			dev_info(dev, "Found bootloader I2C address\n");
		}	
	} else {
		mxt_acquire_irq(data);	//Need this for firmware flashing
	}

	INIT_COMPLETION(data->bl_completion);

	ret = mxt_check_bootloader(data, MXT_WAITING_BOOTLOAD_CMD, false);
	if (ret) {
		/* Bootloader may still be unlocked from previous attempt */
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA, false);
		if (ret)
			goto disable_irq;
	} else {
		dev_info(dev, "Unlocking bootloader\n");

		/* Unlock bootloader */
		ret = mxt_send_bootloader_cmd(data, true);
		if (ret)
			goto disable_irq;
	}

	while (pos < fw->size) {
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA, true);
		if (ret)
			goto disable_irq;

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* Take account of CRC bytes */
		frame_size += 2;

		/* Write one frame to device */
		ret = mxt_bootloader_write(data, &fw->data[pos], frame_size);
		if (ret)
			goto disable_irq;

		ret = mxt_check_bootloader(data, MXT_FRAME_CRC_PASS, true);
		if (ret) {
			retry++;

			/* Back off by 20ms per retry */
			msleep(retry * 20);

			if (retry > 20) {
				dev_err(dev, "Retry count exceeded\n");
				goto disable_irq;
			}
		} else {
			retry = 0;
			pos += frame_size;
			frame++;
		}

		if (pos >= fw->size) {
			dev_info(dev, "Sent %u frames, %zu bytes\n",
				frame, fw->size);
		}
		else if (frame % 50 == 0) {
			dev_info(dev, "Sent %u frames, %d/%zu bytes\n",
				frame, pos, fw->size);
		}
	}

	dev_dbg(dev, "Sent %d frames, %d bytes\n", frame, pos);

	/*
	 * Wait for device to reset. Some bootloader versions do not assert
	 * the CHG line after bootloading has finished, so ignore potential
	 * errors.
	 */
	INIT_COMPLETION(data->bl_completion);

	msleep(MXT_BOOTLOADER_WAIT);	/* Wait for chip to leave bootloader*/
	
	ret = mxt_wait_for_completion(data, &data->bl_completion,
				      MXT_BOOTLOADER_WAIT);
	if (ret) {
		dev_err(dev, "Wait for Firmware update timeout\n");
		goto disable_irq;
	}

disable_irq:
	mxt_disable_irq(data);
release_firmware:
	release_firmware(fw);

	if (!ret) { 
		// We move here that after firmware finished, the interrupt may be working to processing message,
		// So we should keep irq disable before assert the exit of bootloader
		data->in_bootloader = false;
	}

	return ret;
}

static int mxt_update_file_name(struct device *dev, char **file_name,
				const char *buf, size_t count)
{
	char *file_name_tmp;

	/* Simple sanity check */
	if (count > 64) {
		dev_warn(dev, "File name too long\n");
		return -EINVAL;
	}

	file_name_tmp = krealloc(*file_name, count + 1, GFP_KERNEL);
	if (!file_name_tmp)
		return -ENOMEM;

	*file_name = file_name_tmp;
	memcpy(*file_name, buf, count);

	/* Echo into the sysfs entry may append newline at the end of buf */
	if (buf[count - 1] == '\n')
		(*file_name)[count - 1] = '\0';
	else
		(*file_name)[count] = '\0';

	return 0;
}

static ssize_t mxt_update_fw_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char *file_name = NULL;
	int error = 0;

	mutex_lock(&data->update_lock);

	error = mxt_update_file_name(dev, &file_name, buf, count);
	if (error) {
		dev_err(dev, "Failed get file name: %s\n", buf);
		goto out;
	}

	// Clear config
	error = mxt_clear_cfg(data);
	if (error) {
		dev_err(dev, "Skip to clear configuration before Firmware update\n");
	}

	error = mxt_load_fw(dev, file_name);
	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n. IRQ disabled.", error);
		mxt_disable_irq(data);
		
		dev_err(dev, "Executing hardware reset");
		// Not the `irq` should be disabled to call __mxt_reset() for the Seq num synchronization
		count = error;
	} else {
		dev_info(dev, "The firmware update succeeded, Reset\n");
		msleep(MXT_FW_FLASH_TIME);	
	}

	kfree(file_name);

	__mxt_reset(data, F_RST_ANY);	// Any reset

	/* Read infoblock to determine device type */
	error = mxt_read_info_block(data);
	if (error) {
		dev_err(dev, "Re-Read Info Block failed(%d)\n", error);
		goto out;
	}

	// Request IRQ
	error = mxt_acquire_irq(data);
	if (error) {
		dev_err(dev, "Re-Request IRQ failed(%d)\n", error);
		goto out;
	}

	error = request_firmware_nowait(THIS_MODULE, true, MXT_CFG_NAME,
					dev, GFP_KERNEL, data,
					mxt_config_cb);

	if (error) {
		dev_warn(dev, "Failed to invoke firmware loader: %d\n",
			error);
		goto out;
	}

out:
	mutex_unlock(&data->update_lock);
	if (error) {
		return error;
	} else {
		return count;
	}
}

static ssize_t mxt_update_cfg_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char *file_name = NULL;
	const struct firmware *cfg;
	int ret = 0, error;

	mutex_lock(&data->update_lock);

	error = mxt_update_file_name(dev, &file_name, buf, count);
	if (error) {
		dev_err(dev, "Failed get file name: %s\n", buf);
		goto out;
	}

	ret = request_firmware(&cfg, file_name, dev);
	if (ret < 0) {
		dev_err(dev, "Failure to request config file %s\n",
			file_name);
		ret = -ENOENT;
		goto out;
	} else {
		dev_info(dev, "Found configuration file: %s\n",
			file_name);
	}

	//Captures messages in buffer left over from clear_cfg()
	error = mxt_process_messages_until_invalid(data);
	if (error) {
		dev_err(dev, "Failed process configuration\n");
	}

	ret = mxt_configure_objects(data, cfg);
	if (ret)
		goto release;

	ret = count;

release:
	kfree(file_name);
	release_firmware(cfg);
out:
	mutex_unlock(&data->update_lock);

	return ret;
}

typedef struct {
	u8 id;
	char *text;
} msg_type_text_t;

ssize_t _extract_t25_message(const u8 *msg, char *output, ssize_t output_buffer_size)
{
	const u8 *text;
	u8 i,status, seq;
	ssize_t offset = 0;

	const msg_type_text_t status_message[] = {
		{ MXT_SELFTEST_T25_MESSAGE_TEST_PASS, "PASS" },
		{ MXT_SELFTEST_T25_MESSAGE_TEST_UNSUPPORT, "Un-Supported: " },
		{ MXT_SELFTEST_T25_MESSAGE_TEST_POWER_SUPPLY, "Power Supply is not present: " },
		{ MXT_SELFTEST_T25_MESSAGE_TEST_PIN_FAULT, "Pin fault: " },
		{ MXT_SELFTEST_T25_MESSAGE_TEST_OPEN_PIN_FAULT, "Open Pin fault: " },
		{ MXT_SELFTEST_T25_MESSAGE_TEST_SIGNAL_LIMIT, "Signal limit fault: " }
	};

	const msg_type_text_t pinfualt_sequence_message[] = {
		{ 1, "Driven Ground" },
		{ 2, "Driven High" },
		{ 3, "Walk 1" },
		{ 4, "Walk 0" },
		{ 7, "Initial High Voltage"}
	};

	/* <1> Test Status */
	status = msg[1];

	for ( text = "Unknown", i = 0; i < ARRAY_SIZE(status_message); i++ ) {
		if (status == status_message[i].id) {
			text = status_message[i].text;
			break;
		}
	}
	offset += scnprintf(output + offset, output_buffer_size - offset, "%s", text);

	// <2> Test details
	if (status == MXT_SELFTEST_T25_MESSAGE_TEST_INIT ||
		status == MXT_SELFTEST_T25_MESSAGE_TEST_PASS) {
		/* <2.1> Pass or not tested*/
	} else if (status == MXT_SELFTEST_T25_MESSAGE_TEST_UNSUPPORT) {
		/* <2.2> Not support */
	} else if (status == MXT_SELFTEST_T25_MESSAGE_TEST_POWER_SUPPLY ) {
		/* <2.3> Power supply */
	} else if (status == MXT_SELFTEST_T25_MESSAGE_TEST_PIN_FAULT ||
		status == MXT_SELFTEST_T25_MESSAGE_TEST_OPEN_PIN_FAULT) {
		/* <2.4> Pin fault */
		seq = msg[2];
		for ( text = "Unknown", i = 0; i < ARRAY_SIZE(pinfualt_sequence_message); i++ ) {
			if (seq == pinfualt_sequence_message[i].id) {
				text = pinfualt_sequence_message[i].text;
				break;
			}
		}
		/* Pin fault sequence and index */ 
		offset += scnprintf(output + offset, output_buffer_size - offset, "%s, X_PIN %d Y_PIN %d (raw)",
			text, msg[3], msg[4]);
	} else if (status == MXT_SELFTEST_T25_MESSAGE_TEST_SIGNAL_LIMIT) {
		/* <2.5> Signal Limit */
		offset += scnprintf(output + offset, output_buffer_size - offset, "T%d instance %d is out of range",
			msg[2], msg[3]);
	} else {
		/* <2.6> Unknown */
		offset += scnprintf(output + offset, output_buffer_size - offset, "Infos[2-6] %02x %02x %02x %02x %02x",
			msg[2], msg[3], msg[4], msg[5], msg[6]);
	}

	return offset;
}

ssize_t extract_t25_caches(struct mxt_data *data, char *output, ssize_t output_buffer_size)
{
	u8 i;
	ssize_t offset = 0;

	const msg_type_text_t type_message[] = {
		{ MESSAGE_CACHE_TEST_POST, "POST" },
		{ MESSAGE_CACHE_TEST_ON_DEMAND, "OnDemand" },
		{ MESSAGE_CACHE_TEST_BIST, "BIST" },
	};

	if (!data->msg_cache) {
		dev_err(&data->client->dev, "Message Cache is not allocated\n");
		return -ENXIO;
	}

	for ( i = MESSAGE_CACHE_TEST_START; i <= MESSAGE_CACHE_TEST_ON_DEMAND; i++ ) {
		// Type
		offset += scnprintf(output + offset, output_buffer_size - offset, "%s ",
			type_message[i - MESSAGE_CACHE_TEST_START].text);

		// Extract data
		offset += _extract_t25_message(
			MESSAGE_CACHE(data->msg_cache, data->T5_msg_size, i), 
			output + offset, output_buffer_size - offset);

		// Next line
		offset += scnprintf(output + offset, output_buffer_size - offset, "\n");
	}

	return offset;
}

ssize_t _extract_t10_message(const u8 *msg, char *output, ssize_t output_buffer_size)
{
	const u8 *text;
	u8 i, status, code, seq;
	ssize_t offset = 0;

	const msg_type_text_t status_message[] = {
		{ MXT_SELFTEST_T10_MESSAGE_POST_PASS, "PASS" },
		{ 0x12, "Failed: " },

		{ MXT_SELFTEST_T10_MESSAGE_BIST_PASS, "PASS" },
		{ 0x22, "Failed: " },
		{ 0x23, "Overrun: " },

		{ MXT_SELFTEST_T10_MESSAGE_ON_DEMAND_PASS, "PASS" },
		{ 0x32, "Failed: " },
		{ 0x33, "Unavailable: " },
		{ 0x3F, "Invalid: " },
	};

	const msg_type_text_t failure_group_code_message[] = {
		{ MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_CLOCK, "Clock related" },
		{ MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_FLASH, "Flash memory" },
		{ MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_RAM, "RAM" },
		{ MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_CTE, "CTE" },
		{ MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_SIGNAL_LIMIT, "Signal Limit" },
		{ MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_POWER, "Power" },
		{ MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_PIN_FAULT, "Pin fault" },
	};

	const msg_type_text_t pinfualt_sequence_message[] = {
		{ 1, "Driven Ground" },
		{ 2, "Driven High" },
		{ 3, "Walk 1" },
		{ 4, "Walk 0" },
		{ 5, "Walk 0 (Low Emmissions)"}
	};

	/* <1> Test Status */
	status = msg[1];

	for ( text = "Unknown", i = 0; i < ARRAY_SIZE(status_message); i++ ) {
		if (status == status_message[i].id) {
			text = status_message[i].text;
			break;
		}
	}
	offset += scnprintf(output + offset, output_buffer_size - offset, "%s", text);

	/* <2> Group code */
	if (status == MXT_SELFTEST_T10_MESSAGE_INIT
		||status == MXT_SELFTEST_T10_MESSAGE_POST_PASS
		|| status == MXT_SELFTEST_T10_MESSAGE_BIST_PASS
		|| status == MXT_SELFTEST_T10_MESSAGE_ON_DEMAND_PASS) {
		/* <2.1> Pass or Not tested */
	} else if (status == MXT_SELFTEST_T10_MESSAGE_POST_FAILED 
		|| status == MXT_SELFTEST_T10_MESSAGE_BIST_FAILED
		|| status == MXT_SELFTEST_T10_MESSAGE_ON_DEMAND_FAILED) {
		/* Failed */
		
		/* <2.2> Failure Group code */
		code = msg[2];

		for ( text = "Unknown", i = 0; i < ARRAY_SIZE(failure_group_code_message); i++ ) {
			if (code == failure_group_code_message[i].id) {
				text = failure_group_code_message[i].text;
				break;
			}
		}
		offset += scnprintf(output + offset, output_buffer_size - offset, "%s, ", text);

		/* <3> Failure details */
		if (code == MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_CLOCK
			|| code == MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_FLASH
			|| code == MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_RAM
			|| code == MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_CTE
			|| code == MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_POWER) {
			/* <3.1> Clock, Flash, Ram, CTE, Power Failed */
		} else if (code == MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_SIGNAL_LIMIT) {
			/* <3.2> Signal limit */
			offset += scnprintf(output + offset, output_buffer_size - offset, "T%d instance %d is out of range",
				msg[3], msg[4]);
		} else if (code == MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_PIN_FAULT_LEGACY) {
			/* <3.3> Pin fault(Legacy) */
			seq = msg[3];
			for ( text = "Unknown", i = 0; i < ARRAY_SIZE(pinfualt_sequence_message); i++ ) {
				if (seq == pinfualt_sequence_message[i].id) {
					text = pinfualt_sequence_message[i].text;
					break;
				}
			}
			/* Pin fault(legacy) sequence and X/Y/DS index */ 
			offset += scnprintf(output + offset, output_buffer_size - offset, "%s XPin %d, YPin %d, DSPin %d",
				text, msg[4], msg[5], msg[6]);
		} else if (code == MXT_SELFTEST_T10_MESSAGE_TEST_GROUP_PIN_FAULT) {
			/* <3.4> Pin fault */
			seq = msg[3];
			for ( text = "Unknown", i = 0; i < ARRAY_SIZE(pinfualt_sequence_message); i++ ) {
				if (seq == pinfualt_sequence_message[i].id) {
					text = pinfualt_sequence_message[i].text;
					break;
				}
			}
			/* Pin fault sequence and index */ 
			offset += scnprintf(output + offset, output_buffer_size - offset, "%s Pin %d",
				text, msg[4]);
		} else {
			/* <3.5> Unknown code */
			offset += scnprintf(output + offset, output_buffer_size - offset, "Infos[3-6]: %02x %02x %02x %02x",
				msg[3], msg[4], msg[5], msg[6]);
		}
	} else {
		/* <2.3> Others */
		offset += scnprintf(output + offset, output_buffer_size - offset, "Infos[2-6]: %02x %02x %02x %02x %02x",
			msg[2], msg[3], msg[4], msg[5], msg[6]);
	}

	return offset;
}

ssize_t extract_t10_caches(struct mxt_data *data, char *output, ssize_t output_buffer_size)
{
	u8 i;
	ssize_t offset = 0;

	const msg_type_text_t type_message[] = {
		{ MESSAGE_CACHE_TEST_POST, "POST" },
		{ MESSAGE_CACHE_TEST_ON_DEMAND, "OnDemand" },
		{ MESSAGE_CACHE_TEST_BIST, "BIST" },
	};

	if (!data->msg_cache) {
		dev_err(&data->client->dev, "Message Cache is not allocated\n");
		return -ENXIO;
	}

	for ( i = MESSAGE_CACHE_TEST_START; i <= MESSAGE_CACHE_TEST_BIST; i++ ) {
		// Type
		offset += scnprintf(output + offset, output_buffer_size - offset, "%s ",
			type_message[i - MESSAGE_CACHE_TEST_START].text);

		// Extract data
		offset += _extract_t10_message(
			MESSAGE_CACHE(data->msg_cache, data->T5_msg_size, i), 
			output + offset, output_buffer_size - offset);

		// Next line
		offset += scnprintf(output + offset, output_buffer_size - offset, "\n");
	}

	return offset;
}

static ssize_t mxt_selftest_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	ssize_t offset = 0;

	if (data->T25_reportid_min) {
		offset += extract_t25_caches(data, buf, PAGE_SIZE);
	} else if (data->T10_reportid_min) {
		offset += extract_t10_caches(data, buf, PAGE_SIZE);
	} else {
		dev_err(&data->client->dev, "Selftest Object is not exist\n");
	}

	return offset;
}

static ssize_t mxt_selftest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 cmd;
	int ret;

	/*
		For `Test All` command:
			<1> T10: issue cmd 0x3E, response with 0x31 if all pass, 0x32 if failed
			<2> T25: issue cmd 0xFE, response with 0xFE if all pass, other failed
	*/

	if (kstrtou8(buf, 0, &cmd) == 0) {
		ret = mxt_set_selftest(data, cmd, true);
		if (ret != 0) {
			dev_err(dev, "Set Selftest cmd %x failed\n", cmd);
			return ret;
		}
	} else {
		dev_err(dev, "mxt_selftest_store buf %s error\n", buf);
		return -EINVAL;
	}

	return count;
}

static ssize_t mxt_reset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char c;

	c = data->mxt_reset_state? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_reset_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret = 0;

	if (kstrtou8(buf, 0, &i) == 0) {
	
		data->mxt_reset_state = true;

		if (i == F_R_RSV) {
			i = F_RST_ANY;
		}

		ret = mxt_reset(data, i);

		data->mxt_reset_state = false;
		
	} else {
		data->mxt_reset_state = false;
	}

	return count;
}

static ssize_t mxt_crc_enabled_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char c;

	c = data->crc_enabled ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_debug_irq_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int val = -1;

	if (data->chg_gpio) {
		val = 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
			gpiod_get_value(data->chg_gpio)
#else
			gpio_get_value(data->chg_gpio)
#endif
	}

	return scnprintf(buf, PAGE_SIZE, "irq %d chg %d\n", atomic_read(&data->irq_processing), val);
}

static ssize_t mxt_debug_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char c;

	c = data->debug_enabled ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_debug_notify_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0\n");
}

static ssize_t mxt_debug_v2_enable_show(struct device *dev,	
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char c;	
	
	c = data-> debug_v2_enabled ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_debug_v2_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret;

	if (kstrtou8(buf, 0, &i) == 0 && i < 2) {
		if (i == 1)
			mxt_debug_msg_enable(data);
		else
			mxt_debug_msg_disable(data);

		ret = count;
	} else {
		dev_dbg(dev, "debug_enabled write error\n");
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t mxt_debug_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret;

	if (kstrtou8(buf, 0, &i) == 0 && i < 2) {
		data->debug_enabled = (i == 1);

		dev_dbg(dev, "%s\n", i ? "debug enabled" : "debug disabled");
		ret = count;
	} else {
		dev_dbg(dev, "debug_enabled write error\n");
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t mxt_debug_irq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	s8 i;
	ssize_t ret;

	if (kstrtos8(buf, 0, &i) == 0 && i < 2) {
		/*
			This interface will be call by upper level app(e.g: the bridge client). For the un-intended called by app, we limited below rules:
			i > 1: enable the irq
			i == 1: enable irq when irq_rpocessing is less than or equal 0(means irq disabled currently)
			i == 0: diable irq when irq_rpocessing is more than 1(means irq enabled currently)
			i < 0: diable irq
		*/

		if (i > 0) {
			if (i > 1 || atomic_read(&data->irq_processing) <= 0) {
				mxt_acquire_irq(data);
			}
		} else {
			if (i < 0 || atomic_read(&data->irq_processing) > 0) {
				mxt_disable_irq(data);
			}
		}

		dev_info(dev, "%s(%d)\n", i ? "Debug IRQ enabled" : "Debug IRQ disabled", i);
		ret = count;
	} else {
		dev_dbg(dev, "debug_irq write error\n");
		ret = -EINVAL;
	}

	return ret;
}

static int mxt_check_mem_access_params(struct mxt_data *data, loff_t off,
				       size_t *count)
{
	if (off >= data->mem_size)
		return -EIO;

	if (off + *count > data->mem_size)
		*count = data->mem_size - off;

	if (*count > MXT_MAX_BLOCK_WRITE)
		*count = MXT_MAX_BLOCK_WRITE;

	return 0;
}

static ssize_t mxt_mem_access_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = mxt_check_mem_access_params(data, off, &count);
	if (ret < 0)
		return ret;

	if (count > 0) {
		ret = mxt_read_reg_auto(data->client, off, count, buf, data);
	}

	return ret == 0 ? count : ret;
}

static ssize_t mxt_mem_access_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off,
	size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = mxt_check_mem_access_params(data, off, &count);
	if (ret < 0)
		return ret;

	if (count > 0){
		ret = mxt_write_reg_auto(data->client, off, count, buf, data);
	}

	return ret == 0 ? count : ret;
}

static DEVICE_ATTR(fw_version, S_IRUGO, mxt_fw_version_show, NULL);
static DEVICE_ATTR(hw_version, S_IRUGO, mxt_hw_version_show, NULL);
static DEVICE_ATTR(tx_seq_num, S_IWUSR | S_IRUSR, mxt_tx_seq_number_show,
		   mxt_tx_seq_number_store);
static DEVICE_ATTR(object, S_IRUGO, mxt_object_show, NULL);
static DEVICE_ATTR(update_cfg, S_IWUSR, NULL, mxt_update_cfg_store);
static DEVICE_ATTR(config_crc, S_IRUGO, mxt_config_crc_show, NULL);
static DEVICE_ATTR(config_ver, S_IRUGO, mxt_cfg_version_show, NULL);
static DEVICE_ATTR(update_fw, S_IWUSR, NULL, mxt_update_fw_store);
static DEVICE_ATTR(debug_enable, S_IWUSR | S_IRUSR, mxt_debug_enable_show,
		   mxt_debug_enable_store);
static DEVICE_ATTR(debug_v2_enable, S_IWUSR | S_IRUSR, mxt_debug_v2_enable_show,
		   mxt_debug_v2_enable_store);
static DEVICE_ATTR(debug_notify, S_IRUGO, mxt_debug_notify_show, NULL);
static DEVICE_ATTR(debug_irq, S_IWUSR | S_IRUSR, mxt_debug_irq_show,
		   mxt_debug_irq_store);
static DEVICE_ATTR(crc_enabled, S_IRUGO, mxt_crc_enabled_show, NULL);
static DEVICE_ATTR(mxt_reset, S_IWUSR | S_IRUSR, mxt_reset_show, mxt_reset_store);
static DEVICE_ATTR(selftest, S_IWUSR | S_IRUSR, mxt_selftest_show, mxt_selftest_store);

static struct attribute *mxt_attrs[] = {
	&dev_attr_fw_version.attr,
	&dev_attr_hw_version.attr,
	&dev_attr_tx_seq_num.attr,
	&dev_attr_debug_irq.attr,
	&dev_attr_crc_enabled.attr,
	&dev_attr_object.attr,
	&dev_attr_update_cfg.attr,
	&dev_attr_config_ver.attr,
	&dev_attr_config_crc.attr,
	&dev_attr_update_fw.attr,
	&dev_attr_debug_enable.attr,
	&dev_attr_debug_v2_enable.attr,
	&dev_attr_debug_notify.attr,
	&dev_attr_mxt_reset.attr,
	&dev_attr_selftest.attr,
	NULL
};

static const struct attribute_group mxt_attr_group = {
	.attrs = mxt_attrs,
};

static int mxt_sysfs_init(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;

	error = sysfs_create_group(&client->dev.kobj, &mxt_attr_group);
	if (error) {
		dev_err(&client->dev, "Failure %d creating sysfs group\n",
			error);
		return error;
	}

	sysfs_bin_attr_init(&data->mem_access_attr);
	data->mem_access_attr.attr.name = "mem_access";
	data->mem_access_attr.attr.mode = S_IRUGO | S_IWUSR;
	data->mem_access_attr.read = mxt_mem_access_read;
	data->mem_access_attr.write = mxt_mem_access_write;
	data->mem_access_attr.size = data->mem_size;	// Fixme, this is some issue when firmware updated

	error = sysfs_create_bin_file(&client->dev.kobj,
				  &data->mem_access_attr);
	if (error) {
		dev_err(&client->dev, "Failed to create %s\n",
			data->mem_access_attr.attr.name);
		goto err_remove_sysfs_group;
	}

	return 0;

err_remove_sysfs_group:
	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
	data->mem_access_attr.attr.name = NULL;
	return error;
}

static void mxt_sysfs_remove(struct mxt_data *data)
{
	struct i2c_client *client = data->client;

	if (data->mem_access_attr.attr.name) {
		sysfs_remove_bin_file(&client->dev.kobj,
				      &data->mem_access_attr);
		sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
		
		data->mem_access_attr.attr.name = NULL;
	}
}

static void mxt_reset_slots(struct mxt_data *data)
{
	struct input_dev *input_dev = data->input_dev;
	int id;

	if (!input_dev)
		return;

	for (id = 0; id < data->num_touchids; id++) {
		input_mt_slot(input_dev, id);
		input_mt_report_slot_state(input_dev, 0, 0);
	}

	mxt_input_sync(data);
}

static void mxt_start(struct mxt_data *data)
{
	struct i2c_client *client = data->client;

	dev_info (&client->dev, "mxt_start:  Starting . . .\n");

	switch (data->suspend_mode) {
	case MXT_SUSPEND_T9_CTRL:		
		mxt_reset(data, F_RST_ANY);

		/* Touch enable */
		/* 0x83 = SCANEN | RPTEN | ENABLE */
		mxt_write_object(data,
				MXT_TOUCH_MULTI_T9, MXT_T9_CTRL, 0x83);

		break;

	case MXT_SUSPEND_DEEP_SLEEP:
	default:

		mxt_set_t7_power_cfg(data, MXT_POWER_CFG_RUN);

		/* Recalibrate since chip has been in deep sleep */
		mxt_t6_command(data, MXT_COMMAND_CALIBRATE, 1, false);

		msleep(100); 	//Wait for calibration command

		break;
	}
}

static void mxt_stop(struct mxt_data *data)
{

	struct i2c_client *client = data->client;

	dev_info (&client->dev, "mxt_stop:  Stopping . . .\n");

	switch (data->suspend_mode) {
	case MXT_SUSPEND_T9_CTRL:
		/* Touch disable */
		mxt_write_object(data,
			MXT_TOUCH_MULTI_T9, MXT_T9_CTRL, 0);
		break;

	case MXT_SUSPEND_DEEP_SLEEP:

	default:
		mxt_set_t7_power_cfg(data, MXT_POWER_CFG_DEEPSLEEP);
		msleep(100); 	//Wait for calibration command

		mxt_reset_slots(data);
		break;
	}
}

#ifdef CONFIG_ACPI
static int mxt_input_open(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	mxt_start(data);

	return 0;
}

static void mxt_input_close(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	mxt_stop(data);
}
#endif

static int __mxt_parse_device_properties(struct mxt_data *data, const char keymap_property[], u32 **op_keymap, unsigned int *op_numkeys)
{
	struct device *dev = &data->client->dev;
	u32 *keymap;
	int n_keys;
	int error;

	if (device_property_present(dev, keymap_property)) {
		n_keys = device_property_read_u32_array(dev, keymap_property,
							NULL, 0);
		if (n_keys <= 0) {
			error = n_keys < 0 ? n_keys : -EINVAL;
			dev_err(dev, "invalid/malformed '%s' property: %d\n",
				keymap_property, error);
			return error;
		}

		keymap = devm_kmalloc_array(dev, n_keys, sizeof(*keymap),
					    GFP_KERNEL);
		if (!keymap) {
			dev_err(dev, "Failed alloc array memory %d * %d", n_keys, (int)sizeof(*keymap));
			return -ENOMEM;
		}
		error = device_property_read_u32_array(dev, keymap_property,
						       keymap, n_keys);
		if (error) {
			dev_err(dev, "failed to parse '%s' property: %d\n",
				keymap_property, error);
			devm_kfree(dev, keymap);
			return error;
		}

		dev_info(dev, "<DTS> parse device properties and got %d key values\n", n_keys);

		if (op_keymap) {
			*op_keymap = keymap;
		} else {
			devm_kfree(dev, keymap);
		}

		if (op_numkeys) {
			*op_numkeys = n_keys;
		}
	}

	return 0;
}

static int mxt_parse_device_properties(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int error;

	error = __mxt_parse_device_properties(data, "gpio-keymap", &data->t19_keymap, &data->t19_num_keys);
	if (error) {
		dev_err(dev, "failed to parse gpio keymap\n");
		return error;
	}

	
	error = __mxt_parse_device_properties(data, "t15-keymap", &data->t15_keymap, &data->t15_num_keys);
	if (error) {
		dev_err(dev, "failed to parse t15 keymap\n");
		return error;
	}

	return 0;
}

static void mxt_free_device_properties(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;

	if (data->t19_keymap) {
		devm_kfree(dev, data->t19_keymap);
		data->t19_keymap = NULL;
	}

	if (data->t15_keymap) {
		devm_kfree(dev, data->t15_keymap);
		data->t15_keymap = NULL;
	}
}

static int mxt_parse_gpio_properties(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	unsigned int  irq;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	// Using gpiod_xxx interface

	// `reset-gpios`
	data->reset_gpio = devm_gpiod_get_optional(dev,
						   "reset", GPIOD_OUT_LOW);
	if (IS_ERR_OR_NULL(data->reset_gpio)) {
		if (data->reset_gpio == NULL) {
			dev_warn(dev, "Warning: reset-gpios not found or undefined\n");
		} else {
			dev_err(dev, "Failed to get reset_gpios\n");
		}
		return -EPERM;
	}
	else {	
		gpiod_direction_output(data->reset_gpio, 0);	/* GPIO set output as inactive level */
	}

	// `chg-gpios`
	data->chg_gpio = devm_gpiod_get_optional(dev,
						   "chg", GPIOD_IN);
	if (IS_ERR_OR_NULL(data->chg_gpio)) {
		if (data->chg_gpio == NULL) {
			dev_dbg(dev, "Warning: chg-gpios not found or undefined\n");
		} else {
			dev_dbg(dev, "Failed to get chg_gpios\n");
		}
	}
	else {
		//FIXME: consider set pullup in DTS, or by external pull-up resistor ?
		gpiod_direction_input(data->chg_gpio);	/* GPIO set input */
		irq = gpiod_to_irq(data->chg_gpio);
		if (irq && irq != data->client->irq) {
			dev_warn(dev, "Using gpiod to irq mapping(%d) to overrite the client's irq(%d)\n", 
				irq, data->client->irq);
			data->client->irq = irq;
		}
	}
#else
	struct device_node *np = data->client->dev.of_node;
	
	// Using legacy gpio_xxx interface

	// `reset-gpios`
	data->reset_gpio = of_get_named_gpio_flags(np, "reset-gpios",
						    0, NULL);
	if (!gpio_is_valid(data->reset_gpio)) {
		if (data->reset_gpio == 0) {
			dev_warn(dev, "Warning: reset-gpios not found or undefined\n");
		} else {
			dev_err(dev, "Failed to get reset_gpios\n");
		}
		
		return -EPERM;
	} else {
		gpio_direction_output(data->reset_gpio, 1);	/* GPIO set output High */
	}

	// `chg-gpios`
	data->chg_gpio = of_get_named_gpio_flags(np, "chg-gpios",
						    0, NULL);
	if (!gpio_is_valid(data->chg_gpio)) {
		if (data->chg_gpio == 0) {
			dev_dbg(dev, "Warning: chg-gpios not found or undefined\n");
		} else {
			dev_dbg(dev, "Failed to get chg_gpios\n");
		}
	} else {
		//FIXME: consider set pullup in DTS, or by external pull-up resistor ?
		gpio_direction_input(data->chg_gpio);	/* GPIO set input */
		irq = gpio_to_irq(data->chg_gpio);
		if (irq && irq != data->client->irq) {
			dev_warn(dev, "Using gpio to irq mapping(%d) to overrite the client's irq(%d)\n", 
					irq, data->client->irq);
			data->client->irq = irq;
		}
	}
#endif

	return 0;
}

static const struct dmi_system_id chromebook_T9_suspend_dmi[] = {
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "GOOGLE"),
			DMI_MATCH(DMI_PRODUCT_NAME, "Link"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "Peppy"),
		},
	},
	{ }
};

static void mxt_remove(struct i2c_client *client);

static int mxt_probe(struct i2c_client *client)
{
	struct mxt_data *data;
	int error;

	dev_info(&client->dev, "MICROCHIP MaXTouch Driver version %s\n", DRIVER_VERSION_NUMBER);

	/*
	 * Ignore devices that do not have device properties attached to
	 * them, as we need help determining whether we are dealing with
	 * touch screen or touchpad.
	 *
	 * So far on x86 the only users of Atmel touch controllers are
	 * Chromebooks, and chromeos_laptop driver will ensure that
	 * necessary properties are provided (if firmware does not do that).
	 */
	if (!device_property_present(&client->dev, "compatible")) {
		return -ENXIO;
	}

	/*
	 * Ignore ACPI devices representing bootloader mode.
	 *
	 * This is a bit of a hack: Google Chromebook BIOS creates ACPI
	 * devices for both application and bootloader modes, but we are
	 * interested in application mode only (if device is in bootloader
	 * mode we'll end up switching into application anyway). So far
	 * application mode addresses were all above 0x40, so we'll use it
	 * as a threshold.
	 */
	if (ACPI_COMPANION(&client->dev) && client->addr < 0x40) {
		return -ENXIO;
	}

	data = devm_kzalloc(&client->dev, sizeof(struct mxt_data), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	snprintf(data->phys, sizeof(data->phys), "i2c-%u-%04x/input0",
		 client->adapter->nr, client->addr);

	data->client = client;
	i2c_set_clientdata(client, data);

	init_completion(&data->bl_completion);
	init_completion(&data->reset_completion);
	init_completion(&data->crc_completion);
	mutex_init(&data->i2c_lock);
	mutex_init(&data->update_lock);

	data->suspend_mode = dmi_check_system(chromebook_T9_suspend_dmi) ?
		MXT_SUSPEND_T9_CTRL : MXT_SUSPEND_DEEP_SLEEP;

	error = mxt_parse_device_properties(data);
	if (error) {
		dev_err(&client->dev, "Parse device properties failed %d\n", error);
		goto failed;
	}

	error = mxt_parse_gpio_properties(data);
	if (error) {
		dev_warn(&data->client->dev, "Skipped to use hardware reset\n");
	} else {
		__mxt_reset(data, F_RST_HARD);
	}

	error = mxt_initialize(data);
	if (error) {
		dev_err(&client->dev, "mxt initialize failed %d\n", error);
		goto failed;
	}

	/* Enable debugfs */
	mxt_ts_prepare_debugfs(data, dev_driver_string(&client->dev));

	/* Removed the mxt_sys_init and mxt_debug_msg_init */
	/* out of mxt_initialize to avoid duplicate inits */

	error = mxt_sysfs_init(data);
	if (error) {
		dev_err(&client->dev, "sysfs init failed %d\n", error);
		goto failed;
	}

	error = mxt_debug_msg_init(data);
	if (error) {
		dev_err(&client->dev, "debug msg init failed %d\n", error);
		goto failed;
	}

	mutex_init(&data->debug_msg_lock);

	return 0;
failed:
	mxt_remove(client);
	return error;
}

static void mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

	dev_info(&client->dev, "MICROCHIP MaXTouch Driver Removed\n");

	if (data) {
		mxt_debug_msg_remove(data);	
		mxt_sysfs_remove(data);

		mxt_free_irq(data);
		mxt_ts_teardown_debugfs(data);
	
		mxt_free_input_device(data);
		mxt_free_second_input_device(data);
		mxt_free_object_table(data);
		mxt_free_device_properties(data);

		devm_kfree(&client->dev, data);
		i2c_set_clientdata(client, NULL);
	}
}

static int __maybe_unused mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;

	if (!input_dev)
		return 0;

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		mxt_stop(data);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int __maybe_unused mxt_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;

	if (!input_dev)
		return 0;

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		mxt_start(data);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static SIMPLE_DEV_PM_OPS(mxt_pm_ops, mxt_suspend, mxt_resume);

static const struct of_device_id mxt_of_match[] = {
	{ .compatible = "atmel,maxtouch", },
	/* Compatibles listed below are deprecated */
	{ .compatible = "atmel,qt602240_ts", },
	{ .compatible = "atmel,atmel_mxt_ts", },
	{ .compatible = "atmel,atmel_mxt_tp", },
	{ .compatible = "atmel,mXT224", },
	{ .compatible = "mchp,mptt", },
	{},
};
MODULE_DEVICE_TABLE(of, mxt_of_match);

#ifdef CONFIG_ACPI
static const struct acpi_device_id mxt_acpi_id[] = {
	{ "ATML0000", 0 },	/* Touchpad */
	{ "ATML0001", 0 },	/* Touchscreen */
	{ }
};
MODULE_DEVICE_TABLE(acpi, mxt_acpi_id);
#endif

static const struct i2c_device_id mxt_id[] = {
	{ "qt602240_ts", 0 },
	{ "atmel_mxt_ts", 0 },
	{ "atmel_mxt_tp", 0 },
	{ "maxtouch", 0 },
	{ "mXT224", 0 },
	{ "mptt", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mxt_id);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "atmel_mxt_ts",
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(mxt_of_match),
#endif
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(mxt_acpi_id),
#endif
		.pm	= &mxt_pm_ops,
	},
	.probe		= mxt_probe,
	.remove		= mxt_remove,
	.id_table	= mxt_id,
};

module_i2c_driver(mxt_driver);

/* Module information */
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("Microchip maXTouch Touchscreen driver");
MODULE_LICENSE("GPL");
