#pragma once 

#include "reedl-ictrl.h"

#define FLYN384

#ifdef FLYN384
#pragma pack(push, 1)


    typedef struct reedl_ictrl_crsp_imon {
        uint16_t vref;
        uint16_t temp_degc;
    } reedl_ictrl_crsp_imon_t;

    typedef struct reedl_ictrl_crsp_signature {
        uint8_t ver_hw;
        uint8_t ver_fw;
        uint16_t ver_hash;
        char signature[0];
    } reedl_ictrl_crsp_signature_t;

    typedef struct reedl_ictrl_crsp_sspiem_init {
        uint8_t rc;
    } reedl_ictrl_crsp_sspiem_init_t;

#endif

extern int reedl_ictr_sspiem_init(int en);
extern int reedl_ictrl_imon();
extern int reedl_ictrl_sign(const reedl_ictrl_crsp_signature_t **crsp);

