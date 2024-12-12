#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/eventfd.h>
#include <sys/select.h>

#define DBG_LOG_EN 1
#define DBG_LVL LOG_DEBUG
#include "reedl-generic.h"

#include "reedl-ictrl.h"
#include "reedl-ictrl-cmd.h"


struct reedl_ictrl_crsp_signature;
struct reedl_ictrl_crsp_imon;

#define MAX_CRESP_LEN 512

typedef union reedl_ictrl_crsp {
    reedl_ictrl_crsp_signature_t        sign;
    reedl_ictrl_crsp_imon_t             imon;
    reedl_ictrl_crsp_fpga_sign_t        fpga_sign;
    reedl_ictrl_crsp_sspiem_read_t      sspiem_read;
    reedl_ictrl_crsp_sspiem_verify64_t  sspiem_verify64;
    reedl_ictrl_crsp_ret_code_t         ret_code;   // Common response for all commands that respond with return code only.
    uint8_t raw[MAX_CRESP_LEN];
} reedl_ictrl_crsp_t;

reedl_ictrl_crsp_t g_crsp;

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
    reedl_ictrl_crsp_t *crsp, size_t crsp_max_len,
    int timeout_ms, const char *comment)
{
    reedl_ictrl_serial_t* ictrl_serial = g_ictrl.serial;

    fd_set rd_fds;
    int fd_evt_crsp;
    int ictrl_err;
    struct timeval tv;
    int rc, res;

    fd_evt_crsp = eventfd(0, 0);       // AV: will fd number be reused?

    // Register response buffer and cb
    reedl_ictrl_serial_subscribe_crsp(ictrl_serial, crsp->raw, crsp_max_len, fd_evt_crsp);

    // Send command
    res = reedl_ictrl_serial_tx(ictrl_serial,
        (uint8_t*)cmd, cmd->len + sizeof(struct cmd_hdr), comment);

    if (res) {
        close(fd_evt_crsp);
        fd_evt_crsp = -1;
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
    fd_evt_crsp = -1;

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

    reedl_ictrl_crsp_signature_t* crsp_sign = &g_crsp.sign;

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 50, "sign");

    if (rc)
        goto reedl_ictrl_error_sign;
   
    DBG_LOG("Device signature:\n%s %s, %s\n",
        crsp_sign->signature, crsp_sign->ver, crsp_sign->hash);

    if (crsp) *crsp = crsp_sign;
    return 0;

reedl_ictrl_error_sign:

    DBG_ERR("command error : SIGN\n");
    return rc;
}

int reedl_ictrl_fpga_sign(const reedl_ictrl_crsp_fpga_sign_t **crsp)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_fpga_sign {
        struct cmd_hdr hdr;
        // No payload
    } cmd = { {0xD2, 0x48, 0x00} };
#pragma pack(pop)

    reedl_ictrl_crsp_fpga_sign_t* crsp_fpga_sign = &g_crsp.fpga_sign;

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 50, "fpga_sign");

    if (rc)
        goto reedl_ictrl_error_fpga_sign;
   
    DBG_LOG("\nFPGA signature:0x%04X v%d.%d (0x%04X), serial#:%d\n",
        *(uint16_t*)crsp_fpga_sign->sign,
        crsp_fpga_sign->ver[0], crsp_fpga_sign->ver[1],
        crsp_fpga_sign->hash, crsp_fpga_sign->serial);

    if (crsp) *crsp = crsp_fpga_sign;
    return 0;

reedl_ictrl_error_fpga_sign:

    DBG_ERR("command error : FPGA SIGN\n");
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

    reedl_ictrl_crsp_imon_t* reedl_imon = &g_crsp.imon;

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

int reedl_ictrl_sspiem_init(int en)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_init {
        struct cmd_hdr hdr;
        uint8_t en;
    } cmd = { { 0xD2, 0x50, sizeof(cmd.en)}, en };
#pragma pack(pop)

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 50, "sspiem_init");

    if (rc) {
        DBG_ERR("command error : SSPIEM_INIT\n");
        return rc;
    }

    DBG_LOG("SSPIEM INIT: %s (%d)\n",
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK", g_crsp.ret_code.rc);

    return 0;
}

int reedl_ictrl_sspiem_reset(int en)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_reset {
        struct cmd_hdr hdr;
        uint8_t en;
    } cmd = { { 0xD2, 0x51, sizeof(cmd.en)}, en };
#pragma pack(pop)

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 50, "sspiem_reset");   // AV timeout is to big - wait until RX timeout will be triggered

    if (rc) {
        DBG_ERR("command error : SSPIEM_RESET\n");
        return rc;
    }

    DBG_LOG("SSPIEM RESET: %s\n",
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK");

    return 0;
}

int reedl_ictrl_sspiem_cs(int state)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_reset {
        struct cmd_hdr hdr;
        uint8_t state;
    } cmd = { { 0xD2, 0x55, sizeof(cmd.state)}, state };    // 0 - CS is LOW (active)
#pragma pack(pop)

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 500, "sspiem_cs");

    if (rc) {
        DBG_ERR("command error : SSPIEM_CS\n");
        return rc;
    }

    DBG_LOG("SSPIEM CS: %s\n",
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK");

    return 0;
}

int reedl_ictrl_sspiem_runclk()
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_runclk {
        struct cmd_hdr hdr;
    } cmd = { { 0xD2, 0x54, 0x00 } };
#pragma pack(pop)

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 500, "sspiem_runclk");

    if (rc) {
        DBG_ERR("command error : SSPIEM_RUNCLK\n");
        return rc;
    }

    DBG_LOG("SSPIEM RUNCLK: %s\n",
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK");

    return 0;
}

int reedl_ictrl_sspiem_write(const uint8_t *data, int data_len)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_write {
        struct cmd_hdr hdr;
        uint8_t data[16];
    } cmd = { { 0xD2, 0x53, data_len} };
#pragma pack(pop)

    if (data_len > sizeof(cmd.data)) {
        return -1;
    }

    memcpy(cmd.data, data, data_len);

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 500, "sspiem_write");

    if (rc) {
        DBG_ERR("command error : SSPIEM_WRITE\n");
        return rc;
    }

    DBG_LOG("SSPIEM WRITE: %d bytes %s (%d)\n", data_len,
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK", g_crsp.ret_code.rc);

    return 0;
}

int reedl_ictrl_sspiem_read(uint8_t* data, int data_len)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_read {
        struct cmd_hdr hdr;
        uint8_t bytes_to_read;
    } cmd = { { 0xD2, 0x52, sizeof(cmd.bytes_to_read)}, (uint8_t)data_len };
#pragma pack(pop)

    reedl_ictrl_crsp_sspiem_read_t* crsp = &g_crsp.sspiem_read;

    if (sizeof(crsp->data) < data_len) {
        DBG_LOG("SSPIEM READ: Request is too big (%d)\n", data_len);
        return -1;
    }
    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 500, "sspiem_read");

    if (rc) {
        DBG_ERR("command error : SSPIEM_READ\n");
        return rc;
    }

    if ( crsp->hdr.size != sizeof(crsp->rc) + data_len) {
        DBG_LOG("SSPIEM READ: Failed to read %d bytes\n", data_len);
        return -1;
    }

    memcpy(data, crsp->data, data_len);

    DBG_LOG("SSPIEM READ: %d bytes %s (%d) \n", data_len,
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK", g_crsp.ret_code.rc);

    return 0;
}

int reedl_ictrl_sspiem_prog64(const uint8_t* data, int data_len)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_prog64 {
        struct cmd_hdr hdr;
        uint8_t data[256];
    } cmd = { { 0xD2, 0x56, data_len} };
#pragma pack(pop)

    assert(sizeof(cmd.data) >= data_len);
    memcpy(cmd.data, data, data_len);

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 500, "sspiem_prog64");

    if (rc) {
        DBG_ERR("command error : SSPIEM_PROG64\n");
        return rc;
    }

    DBG_LOG("SSPIEM PROG64: %d bytes %s (%d)\n", data_len,
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK", g_crsp.ret_code.rc);

    return 0;
}

int reedl_ictrl_sspiem_progZeros(int data_len)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_progZeros {
        struct cmd_hdr hdr;
        uint8_t zeros_len;
    } cmd = { { 0xD2, 0x57, sizeof(cmd.zeros_len)}, .zeros_len = data_len };
#pragma pack(pop)

    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, sizeof(g_crsp), 500, "sspiem_progZeros");

    if (rc) {
        DBG_ERR("command error : SSPIEM_PROGZEROS\n");
        return rc;
    }

    DBG_LOG("SSPIEM PROGZEROS: %d bytes %s (%d)\n", data_len,
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK", g_crsp.ret_code.rc);

    return 0;
}

int reedl_ictrl_sspiem_verify64(uint8_t* data, int data_len)
{
    int rc = 0;

#pragma pack(push, 1)
    struct reedl_ictrl_cmd_sspiem_verify64 {
        struct cmd_hdr hdr;
        uint8_t bytes_to_read;
    } cmd = { { 0xD2, 0x58, sizeof(cmd.bytes_to_read)}, (uint8_t)data_len };
#pragma pack(pop)

    reedl_ictrl_crsp_sspiem_verify64_t* crsp = &g_crsp.sspiem_verify64;

    if (sizeof(crsp->data) < data_len) {
        DBG_LOG("SSPIEM READ: Request is too big (%d)\n", data_len);
        return -1;
    }
    size_t btw = sizeof(g_crsp) - sizeof(crsp->data) + data_len;
    rc = reedl_ictrl_cmd(&cmd.hdr, &g_crsp, btw, 500, "sspiem_verify64");

    if (rc) {
        DBG_ERR("command error : SSPIEM_VERIFY64\n");
        return rc;
    }

    if (crsp->hdr.size != sizeof(crsp->rc) + data_len) {
        DBG_LOG("SSPIEM VERIFY64: Failed to read %d bytes\n", data_len);
        return -1;
    }

    memcpy(data, crsp->data, data_len);

    DBG_LOG("SSPIEM VERIFY64: %d bytes %s (%d) \n", data_len,
        (g_crsp.ret_code.rc == 0) ? "OK" : "NOK", g_crsp.ret_code.rc);

    return 0;
}
