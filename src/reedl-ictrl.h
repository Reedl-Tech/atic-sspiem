#pragma once

#include <stdint.h>
#include <sys/select.h>
#include <sys/types.h>

#define DBG_BUFF_IDX_DBGL 0
#define DBG_BUFF_IDX_CRSP 1
#define DBG_BUFF_IDX_IMUD 2
#define DBG_BUFF_IDX_LAST 3
#define DBG_BUFF_CNT DBG_BUFF_IDX_LAST

#define ICTRL_SERIAL_UPSTREAM_HDR_SIZE 1

#pragma pack(push, 1)
typedef struct {
    uint8_t mark;
    uint8_t act;
    uint8_t size;
} crsp_hdr_t;
#pragma pack(pop)

typedef struct {
    const char *port;
} reedl_ictrl_serial_cfg_t;

typedef struct {
    union {
        uint8_t* raw;
        crsp_hdr_t* crsp;
    } data;                 // Pointer provided by a command sender 
                            // (aka subscriber) to a buffer with size max_len.
    ssize_t max_len;        // 
    int fd_evt;             // Notification event fd.
    int *err;               // Pointer to error code. Provided by sender,
                            // updated by ictrl_serial. Not is use so far.
} reedl_crsp_t;

typedef struct ictrl_serial_upstream_s {
    int idx;
    size_t hdr_size;
    size_t stream_len;      // Number of bytes expected in stream
    size_t wr_idx;          // Number of already received bytes.
    uint8_t rx_buff[1024];  // Buffer to receive from the device
    void (*on_rx)(struct ictrl_serial_upstream_s *stream);
    ssize_t (*get_len)(uint8_t *rx_buff);

    reedl_crsp_t resp;

} ictrl_serial_upstream_t;

typedef struct {
    ictrl_serial_upstream_t *curr_stream;
    ictrl_serial_upstream_t streams[DBG_BUFF_CNT];
} ictrl_serial_rcv_t;

typedef struct reedl_ictrl_serial_s {
    char signature[6];
    reedl_ictrl_serial_cfg_t cfg;

    int fd_idle_timer;

    int fd_tty;
    uint8_t tty_rx_buf[256];    // for SSPIEm maximum transaction size is 12bytes.

    ictrl_serial_rcv_t receiver;

} reedl_ictrl_serial_t;

typedef struct {
    pthread_t                   thread;
    reedl_ictrl_serial_t*       serial;
    reedl_ictrl_serial_cfg_t    cfg;
} reedl_ictrl_t;

#if 0
/* IO command response internal format. Is used internaly by reedl-ictrl-serial */
typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint8_t *data;
} ictrl_serial_crsp_t;
#endif

#pragma pack(push, 1)
typedef struct {
    uint8_t mark;
    uint8_t size;
} dbgl_hdr_t;

typedef struct {
    dbgl_hdr_t  hdr;        // keep it 1st
//    uint8_t   fid;
//    uint16_t  line;
    char        descr[0];   // keep it last
} dbgl_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint8_t type;
    uint8_t size;
} imud_hdr_t;

typedef struct {
    uint16_t accl_x;
    uint16_t accl_y;
    uint16_t accl_z;
    uint16_t temp;
    uint16_t gyro_x;
    uint16_t gyro_y;
    uint16_t gyro_z;
} imu_fifo_entry_t;

typedef struct imu_dump_data_s {
    uint32_t timestamp;         // FIFO read moment
    uint8_t num;                // Sequence number of 1st entry in the packet

    imu_fifo_entry_t data[4];   // Must be last
} imu_dump_data_t;

typedef struct {
    imud_hdr_t hdr;
    imu_dump_data_t imu_dump;
} imu_dump_pck_t;
#pragma pack(pop)


reedl_ictrl_serial_t *reedl_ictrl_serial_init(reedl_ictrl_serial_cfg_t *cfg);

void reedl_ictrl_serial_check_fds(reedl_ictrl_serial_t *ictrl_serial,
                                     fd_set *rd_fds, fd_set *wr_fds);

int reedl_ictrl_serial_set_fds(reedl_ictrl_serial_t *ictrl_serial, int fd_max,
                                  fd_set *rd_fds, fd_set *wr_fds);

void reedl_ictrl_serial_subscribe_crsp(reedl_ictrl_serial_t* ictrl_serial,
    uint8_t* data, uint32_t max_len, int fd_evt);

int reedl_ictrl_serial_tx(reedl_ictrl_serial_t *ictrl_serial,
                             uint8_t *data, uint32_t bytes_to_write,
                             const char *comment);

int reedl_ictrl_serial_open(reedl_ictrl_serial_t* ictrl_serial);

int reedl_ictrl_init(const char *port_name);
