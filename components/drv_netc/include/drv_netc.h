/* =============================================================================
 * drv_netc.h — DynaWatch V4 NETC 3-port Ethernet switch driver
 *
 * NXP i.MX RT1186 embeds the Network Controller (NETC) block which contains:
 *   - Integrated 3-port 1G Ethernet switch
 *   - PTP/IEEE 1588 hardware timestamping engine (nanosecond resolution)
 *   - IEC 62439-3 HSR/PRP acceleration (future)
 *   - Credit-based shaper (IEEE 802.1Qav) per port
 *
 * DynaWatch port map:
 *   Port 0: External RJ-45 PHY A (SGMII / 1000Base-T)
 *   Port 1: External RJ-45 PHY B (SGMII / 1000Base-T)
 *   Port 2: Internal (host interface — CM33 ENETC endpoint)
 *
 * Protocol stack:
 *   IEC 61850-9-3 (Sampled Values over Ethernet, multicast)
 *   IEEE C37.118.2 (synchrophasor data, unicast UDP/IP)
 *   IEEE 1588-2019 / IEC 62439-3 (PTP boundary clock)
 *
 * Phase 5 stub: init / link-up API declared; implementation deferred.
 * Phase 8 will implement the full NETC driver using NXP NETC SDK.
 *
 * Called from CM33 (NETC is a CM33 peripheral in RT1186 power domain).
 * =============================================================================
 */

#ifndef DRV_NETC_H_
#define DRV_NETC_H_

#include "fsl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Port indices ─────────────────────────────────────────────────────── */
#define DRV_NETC_PORT_A     0U  /*!< External PHY A (RJ-45)        */
#define DRV_NETC_PORT_B     1U  /*!< External PHY B (RJ-45)        */
#define DRV_NETC_PORT_HOST  2U  /*!< CM33 internal (ENETC0)        */

/* ─── Link status ──────────────────────────────────────────────────────── */
typedef enum {
    kNetcLinkDown = 0U,
    kNetcLink10M  = 1U,
    kNetcLink100M = 2U,
    kNetcLink1G   = 3U,
} netc_link_speed_t;

typedef struct {
    netc_link_speed_t speed;
    bool              full_duplex;
    bool              link_up;
} netc_link_status_t;

/* ─── Public API ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise NETC switch hardware.
 *
 * Phase 8 will:
 *   - Enable NETC clocks (CLOCK_EnableClock(kCLOCK_Netc))
 *   - Configure MDIO for PHY access
 *   - Reset PHY A and PHY B, verify device ID
 *   - Configure switch table entries for IEC 61850-9-3 multicast
 *   - Enable PTP timestamping engine
 *
 * Phase 5 stub: prints identity, returns kStatus_Success.
 *
 * @return kStatus_Success.
 */
status_t drv_netc_init(void);

/**
 * @brief Poll link status on the specified port.
 *
 * @param port    Port index (DRV_NETC_PORT_A, _PORT_B).
 * @param p_stat  Out: link status.
 * @return kStatus_Success or kStatus_InvalidArgument.
 */
status_t drv_netc_get_link_status(uint8_t port, netc_link_status_t *p_stat);

/**
 * @brief Transmit an Ethernet frame on the host port (Phase 5 stub).
 *
 * @param p_frame  Pointer to frame data (Ethernet II, including header).
 * @param len      Frame length in bytes (64..1518).
 * @return kStatus_Success (stub always succeeds).
 */
status_t drv_netc_tx_frame(const uint8_t *p_frame, uint16_t len);

/**
 * @brief Register a receive callback for the host port (Phase 8 hook).
 *
 * The callback is invoked from the NETC RX ISR with the received Ethernet
 * frame. The buffer is valid only for the duration of the callback; the
 * application must copy any data it needs to retain.
 *
 * @param cb  Callback: cb(uint8_t *frame, uint16_t len, uint64_t ts_ns)
 *            ts_ns: NETC PTP ingress timestamp in nanoseconds from epoch.
 */
typedef void (*drv_netc_rx_callback_t)(const uint8_t *frame, uint16_t len,
                                       uint64_t ts_ns);
void drv_netc_register_rx_callback(drv_netc_rx_callback_t cb);

/**
 * @brief Read the NETC PTP real-time clock in nanoseconds (Phase 7/8).
 *
 * Phase 5 stub: returns 0.
 *
 * @return PTP time in nanoseconds since TAI epoch.
 */
uint64_t drv_netc_get_ptp_time_ns(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_NETC_H_ */
