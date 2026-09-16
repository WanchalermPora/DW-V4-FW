/* =============================================================================
 * drv_netc.c — DynaWatch V4 NETC Ethernet switch driver (STUB)
 *
 * Phase 5 stub: all functions print identity and return success.
 * Phase 8 will replace this with NXP NETC SDK calls.
 *
 * Phase 8 implementation notes:
 *   SDK component: fsl_netc.h (NXP MCUXpresso NETC driver)
 *   Init sequence:
 *     1. CLOCK_EnableClock(kCLOCK_Netc)
 *     2. NETC_SwitchGetDefaultConfig(&sw_cfg)
 *     3. Configure port maps, VLAN, FDB entries for 61850-9-3 multicast
 *     4. NETC_SwitchInit(NETC_SW, &sw_cfg)
 *     5. PHY: MDIO scan, MDIO_Write(PHY_CTRL, RESET_BIT); wait READY
 *     6. ENETC0 (host port): NETC_EPGetDefaultConfig; NETC_EPInit
 *     7. Enable PTP: NETC_TimerGetDefaultConfig; NETC_TimerInit
 *
 *   RX path (Phase 8):
 *     NETC generates RX descriptor ring interrupts → ISR calls s_rx_callback
 *     with frame pointer and NETC PTP ingress timestamp.
 *
 *   Protocols:
 *     IEC 61850-9-3: GOOSE/SV multicast Ethertype 0x88BA/0x88B8
 *     IEEE C37.118.2: UDP port 4712 (default)
 *     IEEE 1588:      Ethertype 0x88F7
 * =============================================================================
 */

#include "drv_netc.h"
#include "fsl_debug_console.h"

/* ─── Module state ─────────────────────────────────────────────────────── */
static drv_netc_rx_callback_t s_rx_callback = NULL;

/* ─── Public API ───────────────────────────────────────────────────────── */

status_t drv_netc_init(void)
{
    PRINTF("[NETC] drv_netc_init: 3-port 1G switch stub\r\n");
    PRINTF("[NETC]   Port 0: PHY-A (SGMII), Port 1: PHY-B (SGMII), Port 2: CM33 host\r\n");
    PRINTF("[NETC]   Protocols: IEC 61850-9-3, IEEE C37.118.2, IEEE 1588\r\n");
    PRINTF("[NETC] Phase 8 will implement NXP NETC SDK init sequence.\r\n");
    return kStatus_Success;
}

status_t drv_netc_get_link_status(uint8_t port, netc_link_status_t *p_stat)
{
    if (port > DRV_NETC_PORT_HOST || p_stat == NULL)
    {
        return kStatus_InvalidArgument;
    }

    /* Phase 5 stub: report link down on external ports */
    p_stat->link_up     = false;
    p_stat->full_duplex = false;
    p_stat->speed       = kNetcLinkDown;

    PRINTF("[NETC] Port %u link status: DOWN (stub)\r\n", port);
    return kStatus_Success;
}

status_t drv_netc_tx_frame(const uint8_t *p_frame, uint16_t len)
{
    if (p_frame == NULL || len < 64U || len > 1518U)
    {
        return kStatus_InvalidArgument;
    }

    /* Phase 8: NETC_EP_TxQueueSubmit(ENETC0, p_frame, len) */
    PRINTF("[NETC] TX %u bytes (stub — dropped)\r\n", len);
    return kStatus_Success;
}

void drv_netc_register_rx_callback(drv_netc_rx_callback_t cb)
{
    s_rx_callback = cb;
}

uint64_t drv_netc_get_ptp_time_ns(void)
{
    /* Phase 7/8: NETC_TimerGetCurrentTime(NETC_TIMER, &ts); return ts_ns; */
    return 0ULL;
}
