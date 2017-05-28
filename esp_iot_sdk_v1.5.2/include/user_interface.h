/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __USER_INTERFACE_H__
#define __USER_INTERFACE_H__

#include "os_type.h"
#ifdef LWIP_OPEN_SRC
#include "lwip/ip_addr.h"
#else
#include "ip_addr.h"
#endif

#include "queue.h"
#include "user_config.h"
#include "spi_flash.h"

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

enum rst_reason {
	REASON_DEFAULT_RST = 0,         /**< normal startup by power on */
	REASON_WDT_RST,             /**< hardware watch dog reset */
	REASON_EXCEPTION_RST,       /**< exception reset, GPIO status won't change */
	REASON_SOFT_WDT_RST,        /**< software watch dog reset, GPIO status won't change */
	REASON_SOFT_RESTART,        /**< software restart ,system_restart , GPIO status won't change */
	REASON_DEEP_SLEEP_AWAKE,    /**< wake up from deep-sleep */
	REASON_EXT_SYS_RST          /**< external system reset */
};

struct rst_info{
	uint32 reason;  /**< enum rst_reason */
	uint32 exccause;
	uint32 epc1;
	uint32 epc2;
	uint32 epc3;
	uint32 excvaddr;
	uint32 depc;
};

/**
  * @brief  Get the reason of restart.
  *
  * @param  null
  *
  * @return struct rst_info* : information of the system restart
  */
struct rst_info* system_get_rst_info(void);

#define UPGRADE_FW_BIN1         0x00
#define UPGRADE_FW_BIN2         0x01

/**
  * @brief  Reset to default settings.
  *
  *         Reset to default settings of the following APIs : wifi_station_set_auto_connect,
  *         wifi_set_phy_mode, wifi_softap_set_config related, wifi_station_set_config
  *         related, and wifi_set_opmode.
  *
  * @param  null
  *
  * @return null
  */
void system_restore(void);

/**
  * @brief  Restart system.
  *
  * @param  null
  *
  * @return null
  */
void system_restart(void);

/**
  * @brief  Call this API before system_deep_sleep to set the activity after the
  *         next deep-sleep wakeup.
  *
  *         If this API is not called, default to be system_deep_sleep_set_option(1).
  *
  * @param  uint8 option :
  * @param  0 : Radio calibration after the deep-sleep wakeup is decided by byte
  *             108 of esp_init_data_default.bin (0~127byte).
  * @param  1 : Radio calibration will be done after the deep-sleep wakeup. This
  *             will lead to stronger current.
  * @param  2 : Radio calibration will not be done after the deep-sleep wakeup.
  *             This will lead to weaker current.
  * @param  4 : Disable radio calibration after the deep-sleep wakeup (the same
  *             as modem-sleep). This will lead to the weakest current, but the device
  *             can't receive or transmit data after waking up.
  *
  * @return true  : succeed
  * @return false : fail
  */
bool system_deep_sleep_set_option(uint8 option);

/**
  * @brief     Set the chip to deep-sleep mode.
  *
  *            The device will automatically wake up after the deep-sleep time set
  *            by the users. Upon waking up, the device boots up from user_init.
  *
  * @attention 1. XPD_DCDC should be connected to EXT_RSTB through 0 ohm resistor
  *               in order to support deep-sleep wakeup.
  * @attention 2. system_deep_sleep(0): there is no wake up timer; in order to wake
  *               up, connect a GPIO to pin RST, the chip will wake up by a falling-edge
  *               on pin RST
  *
  * @param     uint32 time_in_us : deep-sleep time, unit: microsecond
  *
  * @return    null
  */
void system_deep_sleep(uint32 time_in_us);

uint8 system_upgrade_userbin_check(void);
void system_upgrade_reboot(void);
uint8 system_upgrade_flag_check();
void system_upgrade_flag_set(uint8 flag);

void system_timer_reinit(void);

/**
  * @brief  Get system time, unit: microsecond.
  *
  * @param  null
  *
  * @return System time, unit: microsecond.
  */
uint32 system_get_time(void);

/* user task's prio must be 0/1/2 !!!*/
enum {
	USER_TASK_PRIO_0 = 0,
	USER_TASK_PRIO_1,
	USER_TASK_PRIO_2,
	USER_TASK_PRIO_MAX
};

bool system_os_task(os_task_t task, uint8 prio, os_event_t *queue, uint8 qlen);
bool system_os_post(uint8 prio, os_signal_t sig, os_param_t par);

/**
  * @brief  Print the system memory distribution, including data/rodata/bss/heap.
  *
  * @param  null
  *
  * @return null
  */
void system_print_meminfo(void);

/**
  * @brief  Get the size of available heap.
  *
  * @param  null
  *
  * @return Available heap size.
  */
uint32 system_get_free_heap_size(void);

void system_set_os_print(uint8 onoff);
uint8 system_get_os_print();

uint64 system_mktime(uint32 year, uint32 mon, uint32 day, uint32 hour, uint32 min, uint32 sec);

uint32 system_get_chip_id(void);

typedef void (* init_done_cb_t)(void);

void system_init_done_cb(init_done_cb_t cb);

/**
  * @brief     Get the RTC clock cycle.
  *
  * @attention 1. The RTC clock cycle has decimal part.
  * @attention 2. The RTC clock cycle will change according to the temperature,
  *               so RTC timer is not very precise.
  *
  * @param     null
  *
  * @return    RTC clock period (unit: microsecond), bit11~ bit0 are decimal.
  */
uint32 system_rtc_clock_cali_proc(void);

/**
  * @brief     Get RTC time, unit: RTC clock cycle.
  *
  * Example:
  *            If system_get_rtc_time returns 10 (it means 10 RTC cycles), and
  *            system_rtc_clock_cali_proc returns 5.75 (it means 5.75 microseconds per RTC clock cycle),
  *            (then the actual time is 10 x 5.75 = 57.5 microseconds.
  *
  * @attention System time will return to zero because of system_restart, but the
  *            RTC time still goes on. If the chip is reset by pin EXT_RST or pin
  *            CHIP_EN (including the deep-sleep wakeup), situations are shown as below:
  * @attention 1. reset by pin EXT_RST : RTC memory won't change, RTC timer returns to zero
  * @attention 2. watchdog reset : RTC memory won't change, RTC timer won't change
  * @attention 3. system_restart : RTC memory won't change, RTC timer won't change
  * @attention 4. power on : RTC memory is random value, RTC timer starts from zero
  * @attention 5. reset by pin CHIP_EN : RTC memory is random value, RTC timer starts from zero
  *
  * @param     null
  *
  * @return    RTC time.
  */
uint32 system_get_rtc_time(void);

/**
  * @brief     Read user data from the RTC memory.
  *
  *            The user data segment (512 bytes, as shown below) is used to store user data.
  *
  *             |<---- system data(256 bytes) ---->|<----------- user data(512 bytes) --------->|
  *
  * @attention Read and write unit for data stored in the RTC memory is 4 bytes.
  * @attention src_addr is the block number (4 bytes per block). So when reading data
  *            at the beginning of the user data segment, src_addr will be 256/4 = 64,
  *            n will be data length.
  *
  * @param     uint8 src : source address of rtc memory, src_addr >= 64
  * @param     void *dst : data pointer
  * @param     uint16 n : data length, unit: byte
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool system_rtc_mem_read(uint8 src_addr, void *des_addr, uint16 load_size);

/**
  * @brief     Write user data to  the RTC memory.
  *
  *            During deep-sleep, only RTC is working. So users can store their data
  *            in RTC memory if it is needed. The user data segment below (512 bytes)
  *            is used to store the user data.
  *
  *            |<---- system data(256 bytes) ---->|<----------- user data(512 bytes) --------->|
  *
  * @attention Read and write unit for data stored in the RTC memory is 4 bytes.
  * @attention src_addr is the block number (4 bytes per block). So when storing data
  *            at the beginning of the user data segment, src_addr will be 256/4 = 64,
  *            n will be data length.
  *
  * @param     uint8 src : source address of rtc memory, src_addr >= 64
  * @param     void *dst : data pointer
  * @param     uint16 n : data length, unit: byte
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool system_rtc_mem_write(uint8 des_addr, const void *src_addr, uint16 save_size);

/**
  * @brief  UART0 swap.
  *
  *         Use MTCK as UART0 RX, MTDO as UART0 TX, so ROM log will not output from
  *         this new UART0. We also need to use MTDO (U0CTS) and MTCK (U0RTS) as UART0 in hardware.
  *
  * @param  null
  *
  * @return null
  */
void system_uart_swap(void);

/**
  * @brief  Disable UART0 swap.
  *
  *         Use the original UART0, not MTCK and MTDO.
  *
  * @param  null
  *
  * @return null
  */
void system_uart_de_swap(void);

/**
  * @brief     Measure the input voltage of TOUT pin 6, unit : 1/1024 V.
  *
  * @attention 1. system_adc_read can only be called when the TOUT pin is connected
  *               to the external circuitry, and the TOUT pin input voltage should
  *               be limited to 0~1.0V.
  * @attention 2. When the TOUT pin is connected to the external circuitry, the 107th
  *               byte (vdd33_const) of esp_init_data_default.bin(0~127byte) should be
  *               set as the real power voltage of VDD3P3 pin 3 and 4.
  * @attention 3. The unit of vdd33_const is 0.1V, the effective value range is [18, 36];
  *               if vdd33_const is in [0, 18) or (36, 255), 3.3V is used to optimize RF by default.
  *
  * @param     null
  *
  * @return    Input voltage of TOUT pin 6, unit : 1/1024 V
  */
uint16 system_adc_read(void);

/**
  * @brief     Measure the power voltage of VDD3P3 pin 3 and 4, unit : 1/1024 V.
  *
  * @attention 1. system_get_vdd33 can only be called when TOUT pin is suspended.
  * @attention 2. The 107th byte in esp_init_data_default.bin (0~127byte) is named
  *               as "vdd33_const", when TOUT pin is suspended vdd33_const must be
  *               set as 0xFF, that is 255.
  *
  * @param     null
  *
  * @return    Power voltage of VDD33, unit : 1/1024 V
  */
uint16 system_get_vdd33(void);


/**
  * @brief  Get information of the SDK version.
  *
  * @param  null
  *
  * @return Information of the SDK version.
  */
const char *system_get_sdk_version(void);

#define SYS_BOOT_ENHANCE_MODE	0
#define SYS_BOOT_NORMAL_MODE	1

#define SYS_BOOT_NORMAL_BIN		0
#define SYS_BOOT_TEST_BIN		1

/**
  * @brief     Get information of the boot version.
  *
  * @attention If boot version >= 1.3 , users can enable the enhanced boot mode
  *            (refer to system_restart_enhance).
  *
  * @param     null
  *
  * @return    Information of the boot version.
  */
uint8 system_get_boot_version(void);

/**
  * @brief  Get the address of the current running user bin (user1.bin or user2.bin).
  *
  * @param  null
  *
  * @return The address of the current running user bin.
  */
uint32 system_get_userbin_addr(void);

/**
  * @brief  Get the boot mode.
  *
  * @param  null
  *
  * @return #define SYS_BOOT_ENHANCE_MODE   0
  * @return #define SYS_BOOT_NORMAL_MODE    1
  */
uint8 system_get_boot_mode(void);

/**
  * @brief     Restarts the system, and enters the enhanced boot mode.
  *
  * @attention SYS_BOOT_TEST_BIN is used for factory test during production; users
  *            can apply for the test bin from Espressif Systems.
  *
  * @param     uint8 bin_type : type of bin
  *    - #define SYS_BOOT_NORMAL_BIN      0   // user1.bin or user2.bin
  *    - #define SYS_BOOT_TEST_BIN        1   // can only be Espressif test bin
  * @param     uint32 bin_addr : starting address of the bin file
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool system_restart_enhance(uint8 bin_type, uint32 bin_addr);

#define SYS_CPU_80MHZ	80
#define SYS_CPU_160MHZ	160

/**
  * @brief  Set CPU frequency. Default is 80MHz.
  *
  *         System bus frequency is 80MHz, will not be affected by CPU frequency.
  *         The frequency of UART, SPI, or other peripheral devices, are divided
  *         from system bus frequency, so they will not be affected by CPU frequency either.
  *
  * @param  uint8 freq : CPU frequency, 80 or 160.
  *
  * @return true  : succeed
  * @return false : fail
  */
bool system_update_cpu_freq(uint8 freq);

/**
  * @brief  Get CPU frequency.
  *
  * @param  null
  *
  * @return CPU frequency, unit : MHz.
  */
uint8 system_get_cpu_freq(void);


typedef enum {
	FLASH_SIZE_4M_MAP_256_256 = 0,  /**<  Flash size : 4Mbits. Map : 256KBytes + 256KBytes */
	FLASH_SIZE_2M,                  /**<  Flash size : 2Mbits. Map : 256KBytes */
	FLASH_SIZE_8M_MAP_512_512,      /**<  Flash size : 8Mbits. Map : 512KBytes + 512KBytes */
	FLASH_SIZE_16M_MAP_512_512,     /**<  Flash size : 16Mbits. Map : 512KBytes + 512KBytes */
	FLASH_SIZE_32M_MAP_512_512,     /**<  Flash size : 32Mbits. Map : 512KBytes + 512KBytes */
	FLASH_SIZE_16M_MAP_1024_1024,   /**<  Flash size : 16Mbits. Map : 1024KBytes + 1024KBytes */
	FLASH_SIZE_32M_MAP_1024_1024    /**<  Flash size : 32Mbits. Map : 1024KBytes + 1024KBytes */
} flash_size_map;

/**
  * @brief  Get the current Flash size and Flash map.
  *
  *         Flash map depends on the selection when compiling, more details in document
  *         "2A-ESP8266__IOT_SDK_User_Manual"
  *
  * @param  null
  *
  * @return enum flash_size_map
  */
flash_size_map system_get_flash_size_map(void);

/**
  * @brief  Set the maximum value of RF TX Power, unit : 0.25dBm.
  *
  * @param  uint8 max_tpw : the maximum value of RF Tx Power, unit : 0.25dBm, range [0, 82].
  *                         It can be set refer to the 34th byte (target_power_qdb_0)
  *                         of esp_init_data_default.bin(0~127byte)
  *
  * @return null
  */
void system_phy_set_max_tpw(uint8 max_tpw);

/**
  * @brief     Adjust the RF TX Power according to VDD33, unit : 1/1024 V.
  *
  * @attention 1. When TOUT pin is suspended, VDD33 can be measured by system_get_vdd33.
  * @attention 2. When TOUT pin is connected to the external circuitry, system_get_vdd33
  *               can not be used to measure VDD33.
  *
  * @param     uint16 vdd33 : VDD33, unit : 1/1024V, range [1900, 3300]
  *
  * @return    null
  */
void system_phy_set_tpw_via_vdd33(uint16 vdd33);

/**
  * @brief     Enable RF or not when wakeup from deep-sleep.
  *
  * @attention 1. This API can only be called in user_rf_pre_init.
  * @attention 2. Function of this API is similar to system_deep_sleep_set_option,
  *               if they are both called, it will disregard system_deep_sleep_set_option
  *               which is called before deep-sleep, and refer to system_phy_set_rfoption
  *               which is called when deep-sleep wake up.
  * @attention 3. Before calling this API, system_deep_sleep_set_option should be called
  *               once at least.
  *
  * @param     uint8 option :
  *    - 0 : Radio calibration after deep-sleep wake up depends on esp_init_data_default.bin (0~127byte) byte 108.
  *    - 1 : Radio calibration is done after deep-sleep wake up; this increases the
  *          current consumption.
  *    - 2 : No radio calibration after deep-sleep wake up; this reduces the current consumption.
  *    - 4 : Disable RF after deep-sleep wake up, just like modem sleep; this has the
  *          least current consumption; the device is not able to transmit or receive
  *          data after wake up.
  *
  * @return    null
  */
void system_phy_set_rfoption(uint8 option);

void system_phy_set_powerup_option(uint8 option);


/**
  * @brief  Write data into flash with protection.
  *
  *         Flash read/write has to be 4-bytes aligned.
  *
  *         Protection of flash read/write :
  *             use 3 sectors (4KBytes per sector) to save  4KB data with protect,
  *             sector 0 and sector 1 are data sectors, back up each other,
  *             save data alternately, sector 2 is flag sector, point out which sector
  *             is keeping the latest data, sector 0 or sector 1.
  *
  * @param  uint16 start_sec : start sector (sector 0) of the 3 sectors which are
  *                            used for flash read/write protection.
  *    - For example, in IOT_Demo we can use the 3 sectors (3 * 4KB) starting from flash
  *      0x3D000 for flash read/write protection, so the parameter start_sec should be 0x3D
  * @param  void *param : pointer of the data to be written
  * @param  uint16 len : data length, should be less than a sector, which is 4 * 1024
  *
  * @return true  : succeed
  * @return false : fail
  */
bool system_param_save_with_protect(uint16 start_sec, void *param, uint16 len);


/**
  * @brief  Read the data saved into flash with the read/write protection.
  *
  *         Flash read/write has to be 4-bytes aligned.
  *
  *         Read/write protection of flash:
  *             use 3 sectors (4KB per sector) to save  4KB data with protect, sector
  *             0 and sector 1 are data sectors, back up each other, save data alternately,
  *             sector 2 is flag sector, point out which sector is keeping the latest data,
  *             sector 0 or sector 1.
  *
  * @param  uint16 start_sec : start sector (sector 0) of the 3 sectors used for
  *                            flash read/write protection. It cannot be sector 1 or sector 2.
  *    - For example, in IOT_Demo, the 3 sectors (3 * 4KB) starting from flash 0x3D000
  *      can be used for flash read/write protection.
  *      The parameter start_sec is 0x3D, and it cannot be 0x3E or 0x3F.
  * @param  uint16 offset : offset of data saved in sector
  * @param  void *param : data pointer
  * @param  uint16 len : data length, offset + len =< 4 * 1024
  *
  * @return true  : succeed
  * @return false : fail
  */
bool system_param_load(uint16 start_sec, uint16 offset, void *param, uint16 len);


void system_soft_wdt_stop(void);
void system_soft_wdt_restart(void);
void system_soft_wdt_feed(void);

void system_show_malloc(void);

typedef enum {
	NULL_MODE = 0,      /**< null mode */
	STATION_MODE = 1,       /**< WiFi station mode */
	SOFTAP_MODE = 2,        /**< WiFi soft-AP mode */
	STATIONAP_MODE = 3,     /**< WiFi station + soft-AP mode */
	MAX_MODE
} WIFI_MODE;

typedef enum {
	AUTH_OPEN = 0,      /**< authenticate mode : open */
	AUTH_WEP = 1,           /**< authenticate mode : WEP */
	AUTH_WPA_PSK = 2,       /**< authenticate mode : WPA_PSK */
	AUTH_WPA2_PSK = 3,      /**< authenticate mode : WPA2_PSK */
	AUTH_WPA_WPA2_PSK = 4,  /**< authenticate mode : WPA_WPA2_PSK */
	AUTH_MAX
} AUTH_MODE;

/**
  * @brief  Get the current operating mode of the WiFi.
  *
  * @param  null
  *
  * @return WiFi operating modes:
  *    - 0x01: station mode;
  *    - 0x02: soft-AP mode
  *    - 0x03: station+soft-AP mode
  */
WIFI_MODE wifi_get_opmode(void);

/**
  * @brief  Get the operating mode of the WiFi saved in the Flash.
  *
  * @param  null
  *
  * @return WiFi operating modes:
  *    - 0x01: station mode;
  *    - 0x02: soft-AP mode
  *    - 0x03: station+soft-AP mode
  */
WIFI_MODE wifi_get_opmode_default(void);

/**
  * @brief     Set the WiFi operating mode, and save it to Flash.
  *
  *            Set the WiFi operating mode as station, soft-AP or station+soft-AP,
  *            and save it to Flash. The default mode is soft-AP mode.
  *
  * @attention This configuration will be saved in the Flash system parameter area if changed.
  *
  * @param     uint8 opmode : WiFi operating modes:
  *    - 0x01: station mode;
  *    - 0x02: soft-AP mode
  *    - 0x03: station+soft-AP mode
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_set_opmode(WIFI_MODE opmode);

/**
  * @brief  Set the WiFi operating mode, and will not save it to Flash.
  *
  *         Set the WiFi operating mode as station, soft-AP or station+soft-AP, and
  *         the mode won't be saved to the Flash.
  *
  * @param  uint8 opmode : WiFi operating modes:
  *    - 0x01: station mode;
  *    - 0x02: soft-AP mode
  *    - 0x03: station+soft-AP mode
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_set_opmode_current(WIFI_MODE opmode);

uint8 wifi_get_broadcast_if(void);
bool wifi_set_broadcast_if(uint8 interface);

struct bss_info {
	STAILQ_ENTRY(bss_info)     next;

	uint8 bssid[6];
	uint8 ssid[32];
	uint8 ssid_len;
	uint8 channel;
	sint8 rssi;
	AUTH_MODE authmode;
	uint8 is_hidden;
	sint16 freq_offset;
	sint16 freqcal_val;
	uint8 *esp_mesh_ie;
};

typedef struct _scaninfo {
	STAILQ_HEAD(, bss_info) *pbss;
	struct espconn *pespconn;
	uint8 totalpage;
	uint8 pagenum;
	uint8 page_sn;
	uint8 data_cnt;
} scaninfo;

/**
  * @brief  Callback function for wifi_station_scan.
  *
  * @param  void *arg : information of APs that are found; save them as linked list;
  *                     refer to struct bss_info
  * @param  STATUS status : status of scanning
  *
  * @return null
  */
typedef void (* scan_done_cb_t)(void *arg, STATUS status);

struct station_config {
	uint8 ssid[32];
	uint8 password[64];
	uint8 bssid_set;	// Note: If bssid_set is 1, station will just connect to the router
						// with both ssid[] and bssid[] matched. Please check about this.
	uint8 bssid[6];
};

/**
  * @brief  Get the current configuration of the ESP8266 WiFi station.
  *
  * @param  struct station_config *config : ESP8266 station configuration
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_station_get_config(struct station_config *config);

/**
  * @brief  Get the configuration parameters saved in the Flash of the ESP8266 WiFi station.
  *
  * @param  struct station_config *config : ESP8266 station configuration
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_station_get_config_default(struct station_config *config);

/**
  * @brief     Set the configuration of the ESP8266 station and save it to the Flash.
  *
  * @attention 1. This API can be called only when the ESP8266 station is enabled.
  * @attention 2. If wifi_station_set_config is called in user_init , there is no
  *               need to call wifi_station_connect.
  *               The ESP8266 station will automatically connect to the AP (router)
  *               after the system initialization. Otherwise, wifi_station_connect should be called.
  * @attention 3. Generally, station_config.bssid_set needs to be 0; and it needs
  *               to be 1 only when users need to check the MAC address of the AP.
  * @attention 4. This configuration will be saved in the Flash system parameter area if changed.
  *
  * @param     struct station_config *config : ESP8266 station configuration
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_station_set_config(struct station_config *config);

/**
  * @brief     Set the configuration of the ESP8266 station. And the configuration
  *            will not be saved to the Flash.
  *
  * @attention 1. This API can be called only when the ESP8266 station is enabled.
  * @attention 2. If wifi_station_set_config_current is called in user_init , there
  *               is no need to call wifi_station_connect.
  *               The ESP8266 station will automatically connect to the AP (router)
  *               after the system initialization. Otherwise, wifi_station_connect
  *               should be called.
  * @attention 3. Generally, station_config.bssid_set needs to be 0; and it needs
  *               to be 1 only when users need to check the MAC address of the AP.
  *
  * @param     struct station_config *config : ESP8266 station configuration
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_station_set_config_current(struct station_config *config);

/**
  * @brief     Connect the ESP8266 WiFi station to the AP.
  *
  * @attention 1. This API should be called when the ESP8266 station is enabled,
  *               and the system initialization is completed. Do not call this API in user_init.
  * @attention 2. If the ESP8266 is connected to an AP, call wifi_station_disconnect to disconnect.
  *
  * @param     null
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_station_connect(void);

/**
  * @brief     Disconnect the ESP8266 WiFi station from the AP.
  *
  * @attention This API should be called when the ESP8266 station is enabled,
  *            and the system initialization is completed. Do not call this API in user_init.
  *
  * @param     null
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_station_disconnect(void);

/**
  * @brief  Get rssi of the AP which ESP8266 station connected to.
  *
  * @param  null
  *
  * @return 31 : fail, invalid value.
  * @return others : succeed, value of rssi. In general, rssi value < 10
  */
sint8 wifi_station_get_rssi(void);

struct scan_config {
	uint8 *ssid;	// Note: ssid == NULL, don't filter ssid.
	uint8 *bssid;	// Note: bssid == NULL, don't filter bssid.
	uint8 channel;	// Note: channel == 0, scan all channels, otherwise scan set channel.
	uint8 show_hidden;	// Note: show_hidden == 1, can get hidden ssid routers' info.
};

/**
  * @brief     Scan all available APs.
  *
  * @attention This API should be called when the ESP8266 station is enabled, and
  *            the system initialization is completed. Do not call this API in user_init.
  *
  * @param     struct scan_config *config : configuration of scanning
  * @param     struct scan_done_cb_t cb : callback of scanning
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_station_scan(struct scan_config *config, scan_done_cb_t cb);

/**
  * @brief  Check if the ESP8266 station will connect to the recorded AP automatically
  *         when the power is on.
  *
  * @param  null
  *
  * @return true  : connect to the AP automatically
  * @return false : not connect to the AP automatically
  */
bool wifi_station_get_auto_connect(void);

/**
  * @brief     Set whether the ESP8266 station will connect to the recorded AP
  *            automatically when the power is on. It will do so by default.
  *
  * @attention 1. If this API is called in user_init, it is effective immediately
  *               after the power is on. If it is called in other places, it will
  *               be effective the next time when the power is on.
  * @attention 2. This configuration will be saved in Flash system parameter area if changed.
  *
  * @param     bool set : If it will automatically connect to the AP when the power is on
  *    - true : it will connect automatically
  *    - false: it will not connect automatically
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_station_set_auto_connect(bool set);

/**
  * @brief  Check whether the ESP8266 station will reconnect to the AP after disconnection.
  *
  * @param  null
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_station_get_reconnect_policy(void);

typedef enum {
	STATION_IDLE = 0,        /**< ESP8266 station idle */
	STATION_CONNECTING,      /**< ESP8266 station is connecting to AP*/
	STATION_WRONG_PASSWORD,  /**< the password is wrong*/
	STATION_NO_AP_FOUND,     /**< ESP8266 station can not find the target AP*/
	STATION_CONNECT_FAIL,    /**< ESP8266 station fail to connect to AP*/
	STATION_GOT_IP           /**< ESP8266 station got IP address from AP*/
} STATION_STATUS;

enum dhcp_status {
	DHCP_STOPPED,
	DHCP_STARTED
};

/**
  * @brief  Get the connection status of the ESP8266 WiFi station.
  *
  * @param  null
  *
  * @return the status of connection
  */
STATION_STATUS wifi_station_get_connect_status(void);

/**
  * @brief  Get the information of APs (5 at most) recorded by ESP8266 station.
  *
  * @param  struct station_config config[] : information of the APs, the array size should be 5.
  *
  * @return The number of APs recorded.
  */
uint8 wifi_station_get_current_ap_id(void);

/**
  * @brief  Switch the ESP8266 station connection to a recorded AP.
  *
  * @param  uint8 new_ap_id : AP's record id, start counting from 0.
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_station_ap_change(uint8 current_ap_id);

/**
  * @brief     Set the number of APs that can be recorded in the ESP8266 station.
  *            When the ESP8266 station is connected to an AP, the SSID and password
  *            of the AP will be recorded.
  *
  * @attention This configuration will be saved in the Flash system parameter area if changed.
  *
  * @param     uint8 ap_number : the number of APs that can be recorded (MAX: 5)
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_station_ap_number_set(uint8 ap_number);

/**
  * @brief  Get the information of APs (5 at most) recorded by ESP8266 station.
  *
  * Example:
  * <pre>
  *         struct station_config config[5];
  *         nt i = wifi_station_get_ap_info(config);
  * </pre>
  *
  * @param  struct station_config config[] : information of the APs, the array size should be 5.
  *
  * @return The number of APs recorded.
  */
uint8 wifi_station_get_ap_info(struct station_config config[]);

/**
  * @brief     Enable the ESP8266 station DHCP client.
  *
  * @attention 1. The DHCP is enabled by default.
  * @attention 2. The DHCP and the static IP API ((wifi_set_ip_info)) influence each other,
  *               and if the DHCP is enabled, the static IP will be disabled;
  *               if the static IP is enabled, the DHCP will be disabled.
  *               It depends on the latest configuration.
  *
  * @param  null
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_station_dhcpc_start(void);

/**
  * @brief     Disable the ESP8266 station DHCP client.
  *
  * @attention 1. The DHCP is enabled by default.
  * @attention 2. The DHCP and the static IP API ((wifi_set_ip_info)) influence each other,
  *               and if the DHCP is enabled, the static IP will be disabled;
  *               if the static IP is enabled, the DHCP will be disabled.
  *               It depends on the latest configuration.
  *
  * @param     null
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_station_dhcpc_stop(void);

/**
  * @brief  Get the ESP8266 station DHCP client status.
  *
  * @param  null
  *
  * @return enum dhcp_status
  */
enum dhcp_status wifi_station_dhcpc_status(void);
bool wifi_station_dhcpc_set_maxtry(uint8 num);

char* wifi_station_get_hostname(void);
bool wifi_station_set_hostname(char *name);

int wifi_station_set_cert_key(uint8 *client_cert, int client_cert_len,
	uint8 *private_key, int private_key_len,
	uint8 *private_key_passwd, int private_key_passwd_len);
void wifi_station_clear_cert_key(void);


/** \defgroup SoftAP_APIs SoftAP APIs
  * @brief ESP8266 Soft-AP APIs
  * @attention To call APIs related to ESP8266 soft-AP has to enable soft-AP mode first (wifi_set_opmode)
  */

struct softap_config {
	uint8 ssid[32];         /**< SSID of ESP8266 soft-AP */
	uint8 password[64];     /**< Password of ESP8266 soft-AP */
	uint8 ssid_len;         /**< Length of SSID. If softap_config.ssid_len==0, check the SSID until there is a termination character; otherwise, set the SSID length according to softap_config.ssid_len. */
	uint8 channel;          /**< Channel of ESP8266 soft-AP */
	AUTH_MODE authmode;     /**< Auth mode of ESP8266 soft-AP. Do not support AUTH_WEP in soft-AP mode */
	uint8 ssid_hidden;      /**< Broadcast SSID or not, default 0, broadcast the SSID */
	uint8 max_connection;   /**< Max number of stations allowed to connect in, default 4, max 4 */
	uint16 beacon_interval; /**< Beacon interval, 100 ~ 60000 ms, default 100 */
};

/**
  * @brief  Get the current configuration of the ESP8266 WiFi soft-AP
  *
  * @param  struct softap_config *config : ESP8266 soft-AP configuration
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_softap_get_config(struct softap_config *config);

/**
  * @brief  Get the configuration of the ESP8266 WiFi soft-AP saved in the flash
  *
  * @param  struct softap_config *config : ESP8266 soft-AP configuration
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_softap_get_config_default(struct softap_config *config);

/**
  * @brief     Set the configuration of the WiFi soft-AP and save it to the Flash.
  *
  * @attention 1. This configuration will be saved in flash system parameter area if changed
  * @attention 2. The ESP8266 is limited to only one channel, so when in the soft-AP+station mode,
  *               the soft-AP will adjust its channel automatically to be the same as
  *               the channel of the ESP8266 station.
  *
  * @param     struct softap_config *config : ESP8266 soft-AP configuration
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_softap_set_config(struct softap_config *config);

/**
  * @brief     Set the configuration of the WiFi soft-AP; the configuration will
  *            not be saved to the Flash.
  *
  * @attention The ESP8266 is limited to only one channel, so when in the soft-AP+station mode,
  *            the soft-AP will adjust its channel automatically to be the same as
  *            the channel of the ESP8266 station.
  *
  * @param     struct softap_config *config : ESP8266 soft-AP configuration
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_softap_set_config_current(struct softap_config *config);

struct station_info {
	STAILQ_ENTRY(station_info)	next;

	uint8 bssid[6];
	struct ip_addr ip;
};

struct dhcps_lease {
	bool enable;
	struct ip_addr start_ip;
	struct ip_addr end_ip;
};

enum dhcps_offer_option{
	OFFER_START = 0x00,
	OFFER_ROUTER = 0x01,
	OFFER_END
};

/**
  * @brief     Get the number of stations connected to the ESP8266 soft-AP.
  *
  * @attention The ESP8266 is limited to only one channel, so when in the soft-AP+station mode,
  *            the soft-AP will adjust its channel automatically to be the same as
  *            the channel of the ESP8266 station.
  *
  * @param     null
  *
  * @return    the number of stations connected to the ESP8266 soft-AP
  */
uint8 wifi_softap_get_station_num(void);

/**
  * @brief     Get the information of stations connected to the ESP8266 soft-AP,
  *            including MAC and IP.
  *
  * @attention wifi_softap_get_station_info can not get the static IP, it can only
  *            be used when DHCP is enabled.
  *
  * @param     null
  *
  * @return    struct station_info* : station information structure
  */
struct station_info *wifi_softap_get_station_info(void);

/**
  * @brief     Free the space occupied by station_info when wifi_softap_get_station_info is called.
  *
  * @attention The ESP8266 is limited to only one channel, so when in the soft-AP+station mode,
  *            the soft-AP will adjust its channel automatically to be the same as
  *            the channel of the ESP8266 station.
  *
  * @param     null
  *
  * @return    null
  */
void wifi_softap_free_station_info(void);

/**
  * @brief     Enable the ESP8266 soft-AP DHCP server.
  *
  * @attention 1. The DHCP is enabled by default.
  * @attention 2. The DHCP and the static IP related API (wifi_set_ip_info) influence
  *               each other, if the DHCP is enabled, the static IP will be disabled;
  *               if the static IP is enabled, the DHCP will be disabled.
  *               It depends on the latest configuration.
  *
  * @param     null
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_softap_dhcps_start(void);

/**
  * @brief  Disable the ESP8266 soft-AP DHCP server. The DHCP is enabled by default.
  *
  * @param  null
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_softap_dhcps_stop(void);

/**
  * @brief  Get the ESP8266 soft-AP DHCP server status.
  *
  * @param  null
  *
  * @return enum dhcp_status
  */
enum dhcp_status wifi_softap_dhcps_status(void);

/**
  * @brief     Query the IP range that can be got from the ESP8266 soft-AP DHCP server.
  *
  * @attention This API can only be called during ESP8266 soft-AP DHCP server enabled.
  *
  * @param     struct dhcps_lease *please : IP range of the ESP8266 soft-AP DHCP server.
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_softap_get_dhcps_lease(struct dhcps_lease *please);

/**
  * @brief     Set the IP range of the ESP8266 soft-AP DHCP server.
  *
  * @attention 1. The IP range should be in the same sub-net with the ESP8266
  *               soft-AP IP address.
  * @attention 2. This API should only be called when the DHCP server is disabled
  *               (wifi_softap_dhcps_stop).
  * @attention 3. This configuration will only take effect the next time when the
  *               DHCP server is enabled (wifi_softap_dhcps_start).
  *    - If the DHCP server is disabled again, this API should be called to set the IP range.
  *    - Otherwise, when the DHCP server is enabled later, the default IP range will be used.
  *
  * @param     struct dhcps_lease *please : IP range of the ESP8266 soft-AP DHCP server.
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_softap_set_dhcps_lease(struct dhcps_lease *please);

/**
  * @brief     Get ESP8266 soft-AP DHCP server lease time.
  *
  * @attention This API can only be called during ESP8266 soft-AP DHCP server enabled.
  *
  * @param     null
  *
  * @return    lease time, uint: minute.
  */
uint32 wifi_softap_get_dhcps_lease_time(void);

/**
  * @brief     Set ESP8266 soft-AP DHCP server lease time, default is 120 minutes.
  *
  * @attention This API can only be called during ESP8266 soft-AP DHCP server enabled.
  *
  * @param     uint32 minute : lease time, uint: minute, range:[1, 2880].
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_softap_set_dhcps_lease_time(uint32 minute);

/**
  * @brief     Reset ESP8266 soft-AP DHCP server lease time which is 120 minutes by default.
  *
  * @attention This API can only be called during ESP8266 soft-AP DHCP server enabled.
  *
  * @param     null
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_softap_reset_dhcps_lease_time(void);

/**
  * @brief  Set the ESP8266 soft-AP DHCP server option.
  *
  * Example:
  * <pre>
  *         uint8 mode = 0;
  *         wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);
  * </pre>
  *
  * @param  uint8 level : OFFER_ROUTER, set the router option.
  * @param  void* optarg :
  *    - bit0, 0 disable the router information;
  *    - bit0, 1 enable the router information.
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_softap_set_dhcps_offer_option(uint8 level, void *optarg);


typedef enum {
	STATION_IF = 0, /**< ESP8266 station interface */
	SOFTAP_IF,      /**< ESP8266 soft-AP interface */
	MAX_IF
} WIFI_INTERFACE;

/**
  * @brief     Get the IP address of the ESP8266 WiFi station or the soft-AP interface.
  *
  * @attention Users need to enable the target interface (station or soft-AP) by wifi_set_opmode first.
  *
  * @param     WIFI_INTERFACE if_index : get the IP address of the station or the soft-AP interface,
  *                                      0x00 for STATION_IF, 0x01 for SOFTAP_IF.
  * @param     struct ip_info *info : the IP information obtained.
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_get_ip_info(WIFI_INTERFACE if_index, struct ip_info *info);

/**
  * @brief     Set the IP address of the ESP8266 WiFi station or the soft-AP interface.
  *
  * @attention 1. Users need to enable the target interface (station or soft-AP) by
  *               wifi_set_opmode first.
  * @attention 2. To set static IP, users need to disable DHCP first (wifi_station_dhcpc_stop
  *               or wifi_softap_dhcps_stop):
  *    - If the DHCP is enabled, the static IP will be disabled; if the static IP is enabled,
  *      the DHCP will be disabled. It depends on the latest configuration.
  *
  * @param  WIFI_INTERFACE if_index : get the IP address of the station or the soft-AP interface,
  *                                   0x00 for STATION_IF, 0x01 for SOFTAP_IF.
  * @param  struct ip_info *info : the IP information obtained.
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_set_ip_info(WIFI_INTERFACE if_index, struct ip_info *info);

/**
  * @brief  Get MAC address of the ESP8266 WiFi station or the soft-AP interface.
  *
  * @param  WIFI_INTERFACE if_index : get the IP address of the station or the soft-AP interface,
  *                                   0x00 for STATION_IF, 0x01 for SOFTAP_IF.
  * @param  uint8 *macaddr : the MAC address.
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_get_macaddr(WIFI_INTERFACE if_index, uint8 *macaddr);

/**
  * @brief     Set MAC address of the ESP8266 WiFi station or the soft-AP interface.
  *
  * @attention 1. This API can only be called in user_init.
  * @attention 2. Users need to enable the target interface (station or soft-AP) by wifi_set_opmode first.
  * @attention 3. ESP8266 soft-AP and station have different MAC addresses, do not set them to be the same.
  *    - The bit0 of the first byte of ESP8266 MAC address can not be 1. For example, the MAC address
  *      can set to be "1a:XX:XX:XX:XX:XX", but can not be "15:XX:XX:XX:XX:XX".
  *
  * @param     WIFI_INTERFACE if_index : get the IP address of the station or the soft-AP interface,
  *                                   0x00 for STATION_IF, 0x01 for SOFTAP_IF.
  * @param     uint8 *macaddr : the MAC address.
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_set_macaddr(WIFI_INTERFACE if_index, uint8 *macaddr);


/**
  * @brief  Get the channel number for sniffer functions.
  *
  * @param  null
  *
  * @return channel number
  */
uint8 wifi_get_channel(void);

/**
  * @brief  Set the channel number for sniffer functions.
  *
  * @param  uint8 channel :  channel number
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_set_channel(uint8 channel);

/**
  * @brief  Install the WiFi status LED.
  *
  * @param  uint8 gpio_id : GPIO ID
  * @param  uint8 gpio_name : GPIO mux name
  * @param  uint8 gpio_func : GPIO function
  *
  * @return null
  */
void wifi_status_led_install(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func);

/**
  * @brief  Uninstall the WiFi status LED.
  *
  * @param null
  *
  * @return null
  */
void wifi_status_led_uninstall();

/** Get the absolute difference between 2 u32_t values (correcting overflows)
 * 'a' is expected to be 'higher' (without overflow) than 'b'. */
#define ESP_U32_DIFF(a, b) (((a) >= (b)) ? ((a) - (b)) : (((a) + ((b) ^ 0xFFFFFFFF) + 1)))

/**
  * @brief     Enable the promiscuous mode.
  *
  * @attention 1. The promiscuous mode can only be enabled in the ESP8266 station mode.
  * @attention 2. When in the promiscuous mode, the ESP8266 station and soft-AP are disabled.
  * @attention 3. Call wifi_station_disconnect to disconnect before enabling the promiscuous mode.
  * @attention 4. Don't call any other APIs when in the promiscuous mode. Call
  *               wifi_promiscuous_enable(0) to quit sniffer before calling other APIs.
  *
  * @param     uint8 promiscuous :
  *    - 0: to disable the promiscuous mode
  *    - 1: to enable the promiscuous mode
  *
  * @return    null
  */
void wifi_promiscuous_enable(uint8 promiscuous);

/**
  * @brief The RX callback function in the promiscuous mode.
  *
  *        Each time a packet is received, the callback function will be called.
  *
  * @param uint8 *buf : the data received
  * @param uint16 len : data length
  *
  * @return null
  */
typedef void (* wifi_promiscuous_cb_t)(uint8 *buf, uint16 len);

/**
  * @brief Register the RX callback function in the promiscuous mode.
  *
  *        Each time a packet is received, the registered callback function will be called.
  *
  * @param wifi_promiscuous_cb_t cb : callback
  *
  * @return null
  */
void wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb_t cb);

/**
  * @brief     Set the MAC address filter for the sniffer mode.
  *
  * @attention This filter works only for the current sniffer mode.
  *            If users disable and then enable the sniffer mode, and then enable
  *            sniffer, they need to set the MAC address filter again.
  *
  * @param     const uint8_t *address : MAC address
  *
  * @return    true  : succeed
  * @return    false : fail
  */
void wifi_promiscuous_set_mac(const uint8_t *address);

typedef enum {
	PHY_MODE_11B    = 1,    /**<  802.11b */
	PHY_MODE_11G    = 2,    /**<  802.11g */
	PHY_MODE_11N    = 3     /**<  802.11n */
} WIFI_PHY_MODE;

/**
  * @brief  Get the ESP8266 physical mode (802.11b/g/n).
  *
  * @param  null
  *
  * @return enum WIFI_PHY_MODE
  */
WIFI_PHY_MODE wifi_get_phy_mode(void);

/**
  * @brief     Set the ESP8266 physical mode (802.11b/g/n).
  *
  * @attention The ESP8266 soft-AP only supports bg.
  *
  * @param     WIFI_PHY_MODE mode : physical mode
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_set_phy_mode(WIFI_PHY_MODE mode);

typedef enum {
	NONE_SLEEP_T    = 0,
	LIGHT_SLEEP_T,
	MODEM_SLEEP_T
} sleep_type;

/**
  * @brief     Sets sleep type.
  *
  *            Set NONE_SLEEP_T to disable sleep. Default to be Modem sleep.
  *
  * @attention Sleep function only takes effect in station-only mode.
  *
  * @param     sleep_type type : sleep type
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_set_sleep_type(sleep_type type);

/**
  * @brief  Gets sleep type.
  *
  * @param  null
  *
  * @return sleep type
  */
sleep_type wifi_get_sleep_type(void);

/**
  * @brief     Enable force sleep function.
  *
  * @attention Force sleep function is disabled by default.
  *
  * @param     null
  *
  * @return    null
  */
void wifi_fpm_open(void);

/**
  * @brief  Disable force sleep function.
  *
  * @param  null
  *
  * @return null
  */
void wifi_fpm_close(void);

/**
  * @brief     Wake ESP8266 up from MODEM_SLEEP_T force sleep.
  *
  * @attention This API can only be called when MODEM_SLEEP_T force sleep function
  *            is enabled, after calling wifi_fpm_open.
  *            This API can not be called after calling wifi_fpm_close.
  *
  * @param     null
  *
  * @return    null
  */
void wifi_fpm_do_wakeup(void);

/**
  * @brief     Force ESP8266 enter sleep mode, and it will wake up automatically when time out.
  *
  * @attention 1. This API can only be called when force sleep function is enabled, after
  *               calling wifi_fpm_open. This API can not be called after calling wifi_fpm_close.
  * @attention 2. If this API returned 0 means that the configuration is set successfully,
  *               but the ESP8266 will not enter sleep mode immediately, it is going to sleep
  *               in the system idle task. Please do not call other WiFi related function right
  *               after calling this API.
  *
  * @param     uint32 sleep_time_in_us : sleep time, ESP8266 will wake up automatically
  *                                      when time out. Unit: us. Range: 10000 ~ 268435455(0xFFFFFFF).
  *    - If sleep_time_in_us is 0xFFFFFFF, the ESP8266 will sleep till
  *    - if wifi_fpm_set_sleep_type is set to be LIGHT_SLEEP_T, ESP8266 can wake up by GPIO.
  *    - if wifi_fpm_set_sleep_type is set to be MODEM_SLEEP_T, ESP8266 can wake up by wifi_fpm_do_wakeup.
  *
  * @return   0, setting succeed;
  * @return  -1, fail to sleep, sleep status error;
  * @return  -2, fail to sleep, force sleep function is not enabled.
  */
sint8 wifi_fpm_do_sleep(uint32 sleep_time_in_us);

/**
  * @brief     Set sleep type for force sleep function.
  *
  * @attention This API can only be called before wifi_fpm_open.
  *
  * @param     sleep_type type : sleep type
  *
  * @return    null
  */
void wifi_fpm_set_sleep_type(sleep_type type);

/**
  * @brief  Get sleep type of force sleep function.
  *
  * @param  null
  *
  * @return sleep type
  */
sleep_type wifi_fpm_get_sleep_type(void);


enum {
	EVENT_STAMODE_CONNECTED = 0,
	EVENT_STAMODE_DISCONNECTED,
	EVENT_STAMODE_AUTHMODE_CHANGE,
	EVENT_STAMODE_GOT_IP,
	EVENT_STAMODE_DHCP_TIMEOUT,
	EVENT_SOFTAPMODE_STACONNECTED,
	EVENT_SOFTAPMODE_STADISCONNECTED,
	EVENT_SOFTAPMODE_PROBEREQRECVED,
	EVENT_MAX
};

enum {
	REASON_UNSPECIFIED              = 1,
	REASON_AUTH_EXPIRE              = 2,
	REASON_AUTH_LEAVE               = 3,
	REASON_ASSOC_EXPIRE             = 4,
	REASON_ASSOC_TOOMANY            = 5,
	REASON_NOT_AUTHED               = 6,
	REASON_NOT_ASSOCED              = 7,
	REASON_ASSOC_LEAVE              = 8,
	REASON_ASSOC_NOT_AUTHED         = 9,
	REASON_DISASSOC_PWRCAP_BAD      = 10,  /* 11h */
	REASON_DISASSOC_SUPCHAN_BAD     = 11,  /* 11h */
	REASON_IE_INVALID               = 13,  /* 11i */
	REASON_MIC_FAILURE              = 14,  /* 11i */
	REASON_4WAY_HANDSHAKE_TIMEOUT   = 15,  /* 11i */
	REASON_GROUP_KEY_UPDATE_TIMEOUT = 16,  /* 11i */
	REASON_IE_IN_4WAY_DIFFERS       = 17,  /* 11i */
	REASON_GROUP_CIPHER_INVALID     = 18,  /* 11i */
	REASON_PAIRWISE_CIPHER_INVALID  = 19,  /* 11i */
	REASON_AKMP_INVALID             = 20,  /* 11i */
	REASON_UNSUPP_RSN_IE_VERSION    = 21,  /* 11i */
	REASON_INVALID_RSN_IE_CAP       = 22,  /* 11i */
	REASON_802_1X_AUTH_FAILED       = 23,  /* 11i */
	REASON_CIPHER_SUITE_REJECTED    = 24,  /* 11i */

	REASON_BEACON_TIMEOUT           = 200,
	REASON_NO_AP_FOUND              = 201,
	REASON_AUTH_FAIL				= 202,
	REASON_ASSOC_FAIL				= 203,
	REASON_HANDSHAKE_TIMEOUT		= 204,
};

typedef struct {
	uint8 ssid[32];
	uint8 ssid_len;
	uint8 bssid[6];
	uint8 channel;
} Event_StaMode_Connected_t;

typedef struct {
	uint8 ssid[32];
	uint8 ssid_len;
	uint8 bssid[6];
	uint8 reason;
} Event_StaMode_Disconnected_t;

typedef struct {
	uint8 old_mode;
	uint8 new_mode;
} Event_StaMode_AuthMode_Change_t;

typedef struct {
	struct ip_addr ip;
	struct ip_addr mask;
	struct ip_addr gw;
} Event_StaMode_Got_IP_t;

typedef struct {
	uint8 mac[6];
	uint8 aid;
} Event_SoftAPMode_StaConnected_t;

typedef struct {
	uint8 mac[6];
	uint8 aid;
} Event_SoftAPMode_StaDisconnected_t;

typedef struct {
	int rssi;
	uint8 mac[6];
} Event_SoftAPMode_ProbeReqRecved_t;

typedef union {
	Event_StaMode_Connected_t			connected;
	Event_StaMode_Disconnected_t		disconnected;
	Event_StaMode_AuthMode_Change_t		auth_change;
	Event_StaMode_Got_IP_t				got_ip;
	Event_SoftAPMode_StaConnected_t		sta_connected;
	Event_SoftAPMode_StaDisconnected_t	sta_disconnected;
	Event_SoftAPMode_ProbeReqRecved_t   ap_probereqrecved;
} Event_Info_u;

typedef struct _esp_event {
	uint32 event;
	Event_Info_u event_info;
} System_Event_t;

/**
  * @brief      The Wi-Fi event handler.
  *
  * @attention  No complex operations are allowed in callback.
  *             If you want to execute any complex operations, please post message to another task instead.
  *
  * @param      System_Event_t *event : WiFi event
  *
  * @return     null
  */
typedef void (* wifi_event_handler_cb_t)(System_Event_t *event);

/**
  * @brief  Register the Wi-Fi event handler.
  *
  * @param  wifi_event_handler_cb_t cb : callback function
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_set_event_handler_cb(wifi_event_handler_cb_t cb);

/* WPS can only be used when ESP8266 station is enabled. */

typedef enum wps_type {
	WPS_TYPE_DISABLE = 0,
	WPS_TYPE_PBC, // Only this is supported
	WPS_TYPE_PIN,
	WPS_TYPE_DISPLAY,
	WPS_TYPE_MAX,
} WPS_TYPE_t;

enum wps_cb_status {
	WPS_CB_ST_SUCCESS = 0,     /**< WPS succeed */
	WPS_CB_ST_FAILED,          /**< WPS fail */
	WPS_CB_ST_TIMEOUT,         /**< WPS timeout, fail */
	WPS_CB_ST_WEP,             /**< WPS failed because that WEP is not supported */
	WPS_CB_ST_SCAN_ERR,        /**< can not find the target WPS AP */
};


/**
  * @brief     Enable Wi-Fi WPS function.
  *
  * @attention WPS can only be used when ESP8266 station is enabled.
  *
  * @param     WPS_TYPE_t wps_type : WPS type, so far only WPS_TYPE_PBC is supported
  *
  * @return    true  : succeed
  * @return    false : fail
  */
bool wifi_wps_enable(WPS_TYPE_t wps_type);

/**
  * @brief  Disable Wi-Fi WPS function and release resource it taken.
  *
  * @param  null
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_wps_disable(void);

/**
  * @brief     WPS starts to work.
  *
  * @attention WPS can only be used when ESP8266 station is enabled.
  *
  * @param     null
  *
  * @return    true  : WPS starts to work successfully, but does not mean WPS succeed.
  * @return    false : fail
  */
bool wifi_wps_start(void);

/**
  * @brief  WPS callback.
  *
  * @param  int status : status of WPS, enum wps_cb_status.
  *    -  If parameter status == WPS_CB_ST_SUCCESS in WPS callback, it means WPS got AP's
  *       information, user can call wifi_wps_disable to disable WPS and release resource,
  *       then call wifi_station_connect to connect to target AP.
  *    -  Otherwise, it means that WPS fail, user can create a timer to retry WPS by
  *       wifi_wps_start after a while, or call wifi_wps_disable to disable WPS and release resource.
  *
  * @return null
  */
typedef void (*wps_st_cb_t)(int status);

/**
  * @brief     Set WPS callback.
  *
  * @attention WPS can only be used when ESP8266 station is enabled.
  *
  * @param     wps_st_cb_t cb : callback.
  *
  * @return    true  : WPS starts to work successfully, but does not mean WPS succeed.
  * @return    false : fail
  */
bool wifi_set_wps_cb(wps_st_cb_t cb);


/**
  * @brief  Callback of sending user-define 802.11 packets.
  *
  * @param  uint8 status : 0, packet sending succeed; otherwise, fail.
  *
  * @return null
  */
typedef void (*freedom_outside_cb_t)(uint8 status);

/**
  * @brief      Register a callback for sending user-define 802.11 packets.
  *
  * @attention  Only after the previous packet was sent, entered the freedom_outside_cb_t,
  *             the next packet is allowed to send.
  *
  * @param      freedom_outside_cb_t cb : sent callback
  *
  * @return     0, succeed;
  * @return    -1, fail.
  */
sint32 wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);

/**
  * @brief  Unregister the callback for sending user-define 802.11 packets.
  *
  * @param  null
  *
  * @return null
  */
void wifi_unregister_send_pkt_freedom_cb(void);

/**
  * @brief     Send user-define 802.11 packets.
  *
  * @attention 1. Packet has to be the whole 802.11 packet, does not include the FCS.
  *               The length of the packet has to be longer than the minimum length
  *               of the header of 802.11 packet which is 24 bytes, and less than 1400 bytes.
  * @attention 2. Duration area is invalid for user, it will be filled in SDK.
  * @attention 3. The rate of sending packet is same as the management packet which
  *               is the same as the system rate of sending packets.
  * @attention 4. Only after the previous packet was sent, entered the sent callback,
  *               the next packet is allowed to send. Otherwise, wifi_send_pkt_freedom
  *               will return fail.
  *
  * @param     uint8 *buf : pointer of packet
  * @param     uint16 len : packet length
  * @param     bool sys_seq : follow the system's 802.11 packets sequence number or not,
  *                           if it is true, the sequence number will be increased 1 every
  *                           time a packet sent.
  *
  * @return    0, succeed;
  * @return   -1, fail.
  */
sint32 wifi_send_pkt_freedom(uint8 *buf, uint16 len, bool sys_seq);

/**
  * @brief  Enable RFID LOCP (Location Control Protocol) to receive WDS packets.
  *
  * @param  null
  *
  * @return 0, succeed;
  * @return otherwise, fail.
  */
sint32 wifi_rfid_locp_recv_open(void);

/**
  * @brief  Disable RFID LOCP (Location Control Protocol) .
  *
  * @param  null
  *
  * @return null
  */
void wifi_rfid_locp_recv_close(void);

/**
  * @brief  RFID LOCP (Location Control Protocol) receive callback .
  *
  * @param  uint8 *frm : point to the head of 802.11 packet
  * @param  int len : packet length
  * @param  int rssi : signal strength
  *
  * @return null
  */
typedef void (*rfid_locp_cb_t)(uint8 *frm, int len, sint8 rssi);

/**
  * @brief  Register a callback of receiving WDS packets.
  *
  *         Register a callback of receiving WDS packets. Only if the first MAC
  *         address of the WDS packet is a multicast address.
  *
  * @param  rfid_locp_cb_t cb : callback
  *
  * @return 0, succeed;
  * @return otherwise, fail.
  */
sint32 wifi_register_rfid_locp_recv_cb(rfid_locp_cb_t cb);

/**
  * @brief  Unregister the callback of receiving WDS packets.
  *
  * @param  null
  *
  * @return null
  */
void wifi_unregister_rfid_locp_recv_cb(void);


enum FIXED_RATE {
		PHY_RATE_48       = 0x8,
		PHY_RATE_24       = 0x9,
		PHY_RATE_12       = 0xA,
		PHY_RATE_6        = 0xB,
		PHY_RATE_54       = 0xC,
		PHY_RATE_36       = 0xD,
		PHY_RATE_18       = 0xE,
		PHY_RATE_9        = 0xF,
};

#define FIXED_RATE_MASK_NONE	0x00
#define FIXED_RATE_MASK_STA		0x01
#define FIXED_RATE_MASK_AP		0x02
#define FIXED_RATE_MASK_ALL		0x03

/**
  * @brief     Set the fixed rate and mask of sending data from ESP8266.
  *
  * @attention 1. Only if the corresponding bit in enable_mask is 1, ESP8266 station
  *               or soft-AP will send data in the fixed rate.
  * @attention 2. If the enable_mask is 0, both ESP8266 station and soft-AP will not
  *               send data in the fixed rate.
  * @attention 3. ESP8266 station and soft-AP share the same rate, they can not be
  *               set into the different rate.
  *
  * @param     uint8 enable_mask : 0x00 - disable the fixed rate
  *    -  0x01 - use the fixed rate on ESP8266 station
  *    -  0x02 - use the fixed rate on ESP8266 soft-AP
  *    -  0x03 - use the fixed rate on ESP8266 station and soft-AP
  * @param     uint8 rate  : value of the fixed rate
  *
  * @return    0         : succeed
  * @return    otherwise : fail
  */
sint32 wifi_set_user_fixed_rate(uint8 enable_mask, uint8 rate);

/**
  * @brief  Get the fixed rate and mask of ESP8266.
  *
  * @param  uint8 *enable_mask : pointer of the enable_mask
  * @param  uint8 *rate : pointer of the fixed rate
  *
  * @return 0  : succeed
  * @return otherwise : fail
  */
int wifi_get_user_fixed_rate(uint8 *enable_mask, uint8 *rate);

enum support_rate {
	RATE_11B5M	= 0,
	RATE_11B11M	= 1,
	RATE_11B1M	= 2,
	RATE_11B2M	= 3,
	RATE_11G6M	= 4,
	RATE_11G12M	= 5,
	RATE_11G24M	= 6,
	RATE_11G48M	= 7,
	RATE_11G54M	= 8,
	RATE_11G9M	= 9,
	RATE_11G18M	= 10,
	RATE_11G36M	= 11,
};

/**
  * @brief     Set the support rate of ESP8266.
  *
  *            Set the rate range in the IE of support rate in ESP8266's beacon,
  *            probe req/resp and other packets.
  *            Tell other devices about the rate range supported by ESP8266 to limit
  *            the rate of sending packets from other devices.
  *            Example : wifi_set_user_sup_rate(RATE_11G6M, RATE_11G24M);
  *
  * @attention This API can only support 802.11g now, but it will support 802.11b in next version.
  *
  * @param     uint8 min : the minimum value of the support rate, according to enum support_rate.
  * @param     uint8 max : the maximum value of the support rate, according to enum support_rate.
  *
  * @return    0         : succeed
  * @return    otherwise : fail
  */
sint32 wifi_set_user_sup_rate(uint8 min, uint8 max);

enum RATE_11B_ID {
	RATE_11B_B11M	= 0,
	RATE_11B_B5M	= 1,
	RATE_11B_B2M	= 2,
	RATE_11B_B1M	= 3,
};

enum RATE_11G_ID {
	RATE_11G_G54M	= 0,
	RATE_11G_G48M	= 1,
	RATE_11G_G36M	= 2,
	RATE_11G_G24M	= 3,
	RATE_11G_G18M	= 4,
	RATE_11G_G12M	= 5,
	RATE_11G_G9M	= 6,
	RATE_11G_G6M	= 7,
	RATE_11G_B5M	= 8,
	RATE_11G_B2M	= 9,
	RATE_11G_B1M	= 10
};

enum RATE_11N_ID {
	RATE_11N_MCS7S	= 0,
	RATE_11N_MCS7	= 1,
	RATE_11N_MCS6	= 2,
	RATE_11N_MCS5	= 3,
	RATE_11N_MCS4	= 4,
	RATE_11N_MCS3	= 5,
	RATE_11N_MCS2	= 6,
	RATE_11N_MCS1	= 7,
	RATE_11N_MCS0	= 8,
	RATE_11N_B5M	= 9,
	RATE_11N_B2M	= 10,
	RATE_11N_B1M	= 11
};

#define RC_LIMIT_11B		0
#define RC_LIMIT_11G		1
#define RC_LIMIT_11N		2
#define RC_LIMIT_P2P_11G	3
#define RC_LIMIT_P2P_11N	4
#define RC_LIMIT_NUM		5

#define LIMIT_RATE_MASK_NONE	0x00
#define LIMIT_RATE_MASK_STA		0x01
#define LIMIT_RATE_MASK_AP		0x02
#define LIMIT_RATE_MASK_ALL		0x03

/**
  * @brief     Limit the initial rate of sending data from ESP8266.
  *
  *            Example:
  *                wifi_set_user_rate_limit(RC_LIMIT_11G, 0, RATE_11G_G18M, RATE_11G_G6M);
  *
  * @attention The rate of retransmission is not limited by this API.
  *
  * @param     uint8 mode :  WiFi mode
  *    -  #define RC_LIMIT_11B  0
  *    -  #define RC_LIMIT_11G  1
  *    -  #define RC_LIMIT_11N  2
  * @param     uint8 ifidx : interface of ESP8266
  *    -  0x00 - ESP8266 station
  *    -  0x01 - ESP8266 soft-AP
  * @param     uint8 max : the maximum value of the rate, according to the enum rate
  *                        corresponding to the first parameter mode.
  * @param     uint8 min : the minimum value of the rate, according to the enum rate
  *                        corresponding to the first parameter mode.
  *
  * @return    0         : succeed
  * @return    otherwise : fail
  */
bool wifi_set_user_rate_limit(uint8 mode, uint8 ifidx, uint8 max, uint8 min);

/**
  * @brief  Get the interfaces of ESP8266 whose rate of sending data is limited by
  *         wifi_set_user_rate_limit.
  *
  * @param  null
  *
  * @return LIMIT_RATE_MASK_NONE - disable the limitation on both ESP8266 station and soft-AP
  * @return LIMIT_RATE_MASK_STA  - enable the limitation on ESP8266 station
  * @return LIMIT_RATE_MASK_AP   - enable the limitation on ESP8266 soft-AP
  * @return LIMIT_RATE_MASK_ALL  - enable the limitation on both ESP8266 station and soft-AP
  */
uint8 wifi_get_user_limit_rate_mask(void);

/**
  * @brief  Set the interfaces of ESP8266 whose rate of sending packets is limited by
  *         wifi_set_user_rate_limit.
  *
  * @param  uint8 enable_mask :
  *    -  LIMIT_RATE_MASK_NONE - disable the limitation on both ESP8266 station and soft-AP
  *    -  LIMIT_RATE_MASK_STA  - enable the limitation on ESP8266 station
  *    -  LIMIT_RATE_MASK_AP   - enable the limitation on ESP8266 soft-AP
  *    -  LIMIT_RATE_MASK_ALL  - enable the limitation on both ESP8266 station and soft-AP
  *
  * @return true  : succeed
  * @return false : fail
  */
bool wifi_set_user_limit_rate_mask(uint8 enable_mask);

typedef enum {
	USER_IE_BEACON = 0,
	USER_IE_PROBE_REQ,
	USER_IE_PROBE_RESP,
	USER_IE_ASSOC_REQ,
	USER_IE_ASSOC_RESP,
	USER_IE_MAX
} user_ie_type;

/**
  * @brief  User IE received callback.
  *
  * @param  user_ie_type type :  type of user IE.
  * @param  const uint8 sa[6] : source address of the packet.
  * @param  const uint8 m_oui[3] : factory tag.
  * @param  uint8 *user_ie : pointer of user IE.
  * @param  uint8 ie_len : length of user IE.
  * @param  sint32 rssi : signal strength.
  *
  * @return null
  */
typedef void (*user_ie_manufacturer_recv_cb_t)(user_ie_type type, const uint8 sa[6], const uint8 m_oui[3], uint8 *ie, uint8 ie_len, sint32 rssi);

/**
  * @brief  Set user IE of ESP8266.
  *
  *         The user IE will be added to the target packets of user_ie_type.
  *
  * @param  bool enable  :
  *    -  true, enable the corresponding user IE function, all parameters below have to be set.
  *    -  false, disable the corresponding user IE function and release the resource,
  *       only the parameter "type" below has to be set.
  * @param  uint8 *m_oui : factory tag, apply for it from Espressif System.
  * @param  user_ie_type type : IE type. If it is USER_IE_BEACON, please disable the
  *                             IE function and enable again to take the configuration
  *                             effect immediately .
  * @param  uint8 *user_ie : user-defined information elements, need not input the whole
  *                          802.11 IE, need only the user-define part.
  * @param  uint8 len : length of user IE, 247 bytes at most.
  *
  * @return true  : succeed
  * @return false : fail
  */

bool wifi_set_user_ie(bool enable, uint8 *m_oui, user_ie_type type, uint8 *user_ie, uint8 len);

/**
  * @brief   Register user IE received callback.
  *
  * @param   user_ie_manufacturer_recv_cb_t cb : callback
  *
  * @return  0 : succeed
  * @return -1 : fail
  */
sint32 wifi_register_user_ie_manufacturer_recv_cb(user_ie_manufacturer_recv_cb_t cb);

/**
  * @brief  Unregister user IE received callback.
  *
  * @param  null
  *
  * @return null
  */
void wifi_unregister_user_ie_manufacturer_recv_cb(void);

#endif
