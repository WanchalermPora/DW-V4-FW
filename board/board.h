/* =============================================================================
 * board.h — DynaWatch V4 board-level definitions
 *
 * Target: Custom DynaWatch V4 board — NXP i.MX RT1186-EVK derivative
 *         CM7 @ 600 MHz + CM33 @ 240 MHz
 * =============================================================================
 */

#ifndef BOARD_H_
#define BOARD_H_

#include "fsl_common.h"
#include "clock_config.h"
#include "pin_mux.h"

/* ─── Board identification ─────────────────────────────────────────────────── */
#define BOARD_NAME              "DynaWatch-V4"
#define BOARD_HW_REV            "1.0"

/* ─── Debug UART ───────────────────────────────────────────────────────────── */
#define BOARD_DEBUG_UART        LPUART1
#define BOARD_DEBUG_UART_CLK_FREQ  CLOCK_GetRootClockFreq(kCLOCK_Root_Lpuart1)
#define BOARD_DEBUG_UART_BAUDRATE  115200U

/* ─── Status LEDs — disabled; PRINTF over LPUART1 is the sole indicator.
 *     Uncomment when hardware bring-up requires visual feedback.
 * #define BOARD_LED_GREEN_GPIO    GPIO9
 * #define BOARD_LED_GREEN_PIN     3U
 * #define BOARD_LED_RED_GPIO      GPIO9
 * #define BOARD_LED_RED_PIN       4U
 * #define BOARD_LED_GPIO          BOARD_LED_GREEN_GPIO
 * #define BOARD_LED_GPIO_PIN      BOARD_LED_GREEN_PIN
 */

/* ─── User Button — disabled; no DEV_BYPASS trigger in current build.
 * #define BOARD_USER_BTN_GPIO     GPIO8
 * #define BOARD_USER_BTN_PIN      14U
 */

/* ─── ADS131M08-Q1 SPI (ADC for PoW acquisition) ──────────────────────────── */
#define BOARD_ADC_SPI           LPSPI1
#define BOARD_ADC_SPI_CLK_FREQ  CLOCK_GetRootClockFreq(kCLOCK_Root_Lpspi1)
#define BOARD_ADC_SPI_BAUDRATE  20000000U   /* 20 MHz */
#define BOARD_ADC_DRDY_GPIO     GPIO7
#define BOARD_ADC_DRDY_PIN      0U
#define BOARD_ADC_DRDY_IRQ      GPIO7_Combined_0_15_IRQn

/* ─── Ethernet (C37.118.2 / Modbus TCP / IEC 61850) ───────────────────────── */
#define BOARD_ENET_PHY_ADDR     0x02U       /* KSZ9131 PHY address */

/* ─── PTP / 1PPS ───────────────────────────────────────────────────────────── */
#define BOARD_1PPS_GPIO         GPIO6
#define BOARD_1PPS_PIN          10U
#define BOARD_1PPS_IRQ          GPIO6_Combined_0_15_IRQn

/* ─── OCRAM regions (linker-aligned) ──────────────────────────────────────── */
/* PoW ring and double buffer live in OCRAM1 (CM7 + CM33 accessible).        */
#define BOARD_POW_BUF_BASE      0x20240000UL    /* OCRAM1 start */
#define BOARD_POW_BUF_SIZE      (512U * 1024U)  /* 512 KB reserved */

/* ─── OTFAD / XiP flash context for LUT ───────────────────────────────────── */
#define BOARD_OTFAD_CTX1_BASE   0x08800000UL    /* FlexSPI1 AHB window, context 1 */
#define BOARD_OTFAD_CTX1_SIZE   (4U * 1024U * 1024U)  /* 4 MB */

/* ─── Function prototypes ──────────────────────────────────────────────────── */
void BOARD_InitHardware(void);
void BOARD_InitDebugConsole(void);

#endif /* BOARD_H_ */
