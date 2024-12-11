#pragma once 

#include "reedl-ictrl.h"

#define FLYN384

#ifdef FLYN384
#pragma pack(push, 1)

    typedef struct {
        uint8_t mark;		// DBG_BUFF_MARK_RSP
        uint8_t act;
        uint8_t size;
    } reedl_ictrl_crsp_hdr_t;

    typedef struct reedl_ictrl_crsp_imon {
        reedl_ictrl_crsp_hdr_t hdr;
        uint16_t vref;
        uint16_t temp_degc;
    } reedl_ictrl_crsp_imon_t;

    typedef struct reedl_ictrl_crsp_fpga_sign {
        reedl_ictrl_crsp_hdr_t hdr;
    	uint8_t rc;
    	uint8_t sign[2];
    	uint8_t ver[2];
    	uint32_t hash;
    	uint16_t serial;
    } reedl_ictrl_crsp_fpga_sign_t;

    typedef struct reedl_ictrl_crsp_sspiem_read {
        reedl_ictrl_crsp_hdr_t hdr;
        uint8_t rc;
        uint8_t data[16];
    } reedl_ictrl_crsp_sspiem_read_t;

    typedef struct reedl_ictrl_crsp_signature {
        reedl_ictrl_crsp_hdr_t hdr;
        char signature[16];
        char ver[6];
        char hash[8];
    } reedl_ictrl_crsp_signature_t;

    typedef struct reedl_ictrl_crsp_ret_code {
        reedl_ictrl_crsp_hdr_t hdr;
        uint8_t rc;
    } reedl_ictrl_crsp_ret_code_t;

#endif

extern int reedl_ictrl_sspiem_init(int en);
extern int reedl_ictrl_sspiem_reset(int en);
extern int reedl_ictrl_sspiem_cs(int cs_state);
extern int reedl_ictrl_sspiem_runclk();
extern int reedl_ictrl_sspiem_write(const uint8_t* data, int data_len);
extern int reedl_ictrl_sspiem_read(uint8_t* data, int data_len);
extern int reedl_ictrl_sspiem_prog64(const uint8_t* data, int data_len);
extern int reedl_ictrl_imon();
extern int reedl_ictrl_sign(const reedl_ictrl_crsp_signature_t **crsp);
extern int reedl_ictrl_fpga_sign(const reedl_ictrl_crsp_fpga_sign_t **crsp);

