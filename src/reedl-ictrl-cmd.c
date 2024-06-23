#include <stdint.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/select.h>

#define DBG_LOG_EN 1
#define DBG_LVL LOG_DEBUG
#include "reedl-generic.h"

#include "reedl-ictrl.h"
#include "reedl-ictrl-cmd.h"


struct reedl_ictrl_crsp_signature;
struct reedl_ictrl_crsp_imon;

#define MAX_CRESP_LEN 256

union {
    struct reedl_ictrl_crsp_signature    sign;
    struct reedl_ictrl_crsp_imon         imon;
    data[MAX_CRESP_LEN];
} g_crsp;

extern reedl_ictrl_t g_ictrl;

#pragma pack(push, 1)
struct cmd_hdr {
    uint8_t    mark;
    uint8_t    cmd;
    int16_t    len;
};
#pragma pack(pop)

static int reedl_ictrl_cmd (
    struct cmd_hdr* cmd,
    uint8_t* crsp, size_t crsp_max_len,
    int timeout_ms, const char *comment)
{
    reedl_ictrl_serial_t* ictrl_serial = &g_ictrl.serial;

    fd_set rd_fds;
    int fd_evt_crsp;
    struct timeval tv;
    int rc, res;

    fd_evt_crsp = eventfd(0, 0);       // AV: will fd number be reused?

    // Register response buffer and cb
    reedl_ictrl_serial_subscribe_crsp(ictrl_serial, crsp, crsp_max_len, fd_evt_crsp);

    // Send command
    res = reedl_ictrl_serial_tx(ictrl_serial,
        cmd, cmd->len + sizeof(struct cmd_hdr), "cmd");

    if (res) {
        close(fd_evt_crsp);
        return -1;
    }

    // Wait for response
    FD_ZERO(&rd_fds);
    FD_SET(fd_evt_crsp, &rd_fds);

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    res = select(fd_evt_crsp + 1, &rd_fds, NULL, NULL, &tv);

    rc = -1;
    if (res < 0) {
        DBG_LOG("ICTRL CMD error %d", res);
    }
    else if (res == 0) {
        DBG_LOG("ICTRL CMD timeout");
    }
    else {
        if (FD_ISSET(fd_evt_crsp, &rd_fds)) {
            rc = 0;
        }
    }
    close(fd_evt_crsp);
    return rc;
}

int reedl_ictrl_sign(const reedl_ictrl_crsp_signature_t **crsp)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sign {
        struct cmd_hdr hdr;
        // No payload
    } cmd = { {0xD2, 0x11, 0x00} };
#pragma pack(pop)

    reedl_ictrl_crsp_signature_t* crsp_sign = &g_crsp;

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 50, "sign");

    if (rc)
        goto reedl_ictrl_error_sign;
   
    DBG_LOG("Device signature:\n%s. HW %d.%d, FW %d.%d\n",
        crsp_sign->signature,
        crsp_sign->ver_hw >> 4, crsp_sign->ver_hw & 0x0F,
        crsp_sign->ver_fw >> 4, crsp_sign->ver_fw & 0x0F);

    crsp = crsp_sign;
    return 0;

reedl_ictrl_error_sign:

    DBG_ERR("command error : SIGN\n");
    return rc;
}

int reedl_ictrl_imon()
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_imon {
        struct cmd_hdr hdr;
        // No payload
    } cmd = { { 0xD2, 0x31, 0x00 } };
#pragma pack(pop)

    reedl_ictrl_crsp_imon_t* reedl_imon = &g_crsp;

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 50, "imon");

    if (rc)
        goto reedl_ictrl_error_imon;

    DBG_LOG("Imon: Vref=%.1d Temp=%d degC\n",
        reedl_imon->vref, reedl_imon->temp_degc);

    // TODO: Check all other voltages. Return err + log in case of 
    //       any of voltages is out of range.

    return 0;

reedl_ictrl_error_imon:

    DBG_ERR("command error : IMON\n");
    return rc;

}


int reedl_ictr_sspiem_init(int en)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_init {
        struct cmd_hdr hdr;
        uint8_t en;
    } cmd = { { 0xD2, 0x31, 0x00 }, en };
#pragma pack(pop)

    reedl_ictrl_crsp_sspiem_init_t* sspiem_init = &g_crsp;

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 50, "sspiem_init");

    if (rc)
        goto reedl_ictrl_error_sspiem_init;

    DBG_LOG("SSPIEM init: %s\n",
        (sspiem_init->rc == 0) ? "OK" : "NOK");

    return 0;

reedl_ictrl_error_sspiem_init:

    DBG_ERR("command error : SSPIEM_INIT\n");
    return rc;

}

