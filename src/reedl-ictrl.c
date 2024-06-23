#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/syslog.h>

#define DBG_LOG_EN 1
#define DBG_LVL LOG_DEBUG
#include "reedl-generic.h"

#include "reedl-utils.h"
#include "reedl-ictrl.h"

int g_terminate = 0;

reedl_ictrl_t g_ictrl = {
    .cfg = {.port = "/dev/ttyACM1" }
};

#define SERIAL_IDLE_TIMER_INTERVAL_MS 3000

static void ictrl_serial_upstream_on_rx_dbgl(ictrl_serial_upstream_t *stream);
static ssize_t ictrl_serial_upstream_get_len_dbgl(uint8_t *rx_buff);

static void ictrl_serial_upstream_on_rx_crsp(ictrl_serial_upstream_t *stream);
static ssize_t ictrl_serial_upstream_get_len_crsp(uint8_t *rx_buff);

#define DBG_LOG_HEADER_SIZE 2

static char *dbgl2str(dbgl_t *dbgl)
{
    return dbgl->descr;
}

static int ictrl_serial_set_tty_params(int fd, speed_t brate)
{
    int rc = 0;
    int ret;
    struct termios tio;

    ret = tcgetattr(fd, &tio);
    if (ret) {
        DBG_ERR("ERROR during tcgetattr(): %d", errno);
        rc = -1;
    }
    DBG_LOG("Baudrate 1: %u-%X", cfgetispeed(&tio), tio.c_cflag);

    tio.c_cflag = (CS8 | CLOCAL | CREAD);
    cfsetispeed(&tio, brate);
    cfsetospeed(&tio, brate);

    // set input mode (non-canonical, no echo,...)
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME] = 0;    // inter-character timer unused
    tio.c_cc[VMIN] = 0;     // AV

    DBG_LOG("Baudrate 2: %u-%X", cfgetispeed(&tio), tio.c_cflag);
    ret = tcsetattr(fd, TCSANOW, &tio);
    if (ret) {
        DBG_ERR("Baudrate 3: ERROR during tcsetattr(): %d", errno);
        rc = -1;
    }

    return rc;
}

static void ictrl_serial_tty_close(reedl_ictrl_serial_t *ictrl_serial)
{
    if (-1 == ictrl_serial->fd_tty)
        return;

    DBG_INFO("Closing TTY port %s", ictrl_serial->cfg.port);

    close(ictrl_serial->fd_tty);
    ictrl_serial->fd_tty = -1;
}


static void ictrl_serial_close(reedl_ictrl_serial_t *ictrl_serial)
{
    if (ictrl_serial->fd_idle_timer > 0) {
        close(ictrl_serial->fd_idle_timer);
        ictrl_serial->fd_idle_timer = -1;
    }

    ictrl_serial_tty_close(ictrl_serial);
}

int reedl_ictrl_serial_open(reedl_ictrl_serial_t *ictrl_serial)
{
    int int_param = 1;
    int rc;

    ictrl_serial->fd_tty =
            open(ictrl_serial->cfg.port, O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (ictrl_serial->fd_tty < 0) {
        DBG_ERR("Unable to open tty port (%s): %s", ictrl_serial->cfg.port,
                strerror(errno));
        ictrl_serial->fd_tty = -1;
        return -1;
    }

    rc = ioctl(ictrl_serial->fd_tty, FIONBIO, &int_param, sizeof(int_param));
    if (rc < 0) {
        DBG_ERR("Unable set FIONBIO on %s: %s", ictrl_serial->cfg.port,
                strerror(errno));
        goto on_error;
    }
    
    if (0 != ictrl_serial_set_tty_params(ictrl_serial->fd_tty, 115200)) {
        goto on_error;
    }

    return 0;

on_error:
    ictrl_serial_tty_close(ictrl_serial);
    return -1;
}

static void ictrl_serial_on_idle(reedl_ictrl_serial_t *ictrl_serial)
{
    // Try to connect to port if not connected
    if (-1 == ictrl_serial->fd_tty) {
        ictrl_serial_tty_open(ictrl_serial);
    }
}

#define DBG_BUFF_MARK_OVFL 0xDD
#define DBG_BUFF_MARK_RSP 0xD3
#define DBG_BUFF_MARK_LOG 0xD5
#define DBG_BUFF_MARK_LOG_ISR 0xD9

static ssize_t ictrl_serial_upstream_get_len_dbgl(uint8_t *rx_buff)
{
    dbgl_hdr_t *dbg_hdr = (dbgl_hdr_t *)rx_buff;
    if (dbg_hdr->mark != DBG_BUFF_MARK_OVFL &&
        dbg_hdr->mark != DBG_BUFF_MARK_LOG &&
        dbg_hdr->mark != DBG_BUFF_MARK_LOG_ISR) {
        return -1;
    }
    return ((dbgl_hdr_t *)rx_buff)->size + sizeof(dbgl_hdr_t);
}
    
static void ictrl_serial_upstream_on_rx_dbgl(ictrl_serial_upstream_t *stream)
{
    dbgl_t *dbgl = (dbgl_t *)stream->rx_buff;

    if ((dbgl->hdr.mark == DBG_BUFF_MARK_LOG) ||
        (dbgl->hdr.mark == DBG_BUFF_MARK_LOG_ISR)) {
        DBG_LOG("DBGL: %s", dbgl2str(dbgl));
        memset(stream->rx_buff, 0xCC, sizeof(stream->rx_buff));
    }

    return;
}

static ssize_t ictrl_serial_upstream_get_len_crsp(uint8_t *rx_buff)
{
    return ((crsp_hdr_t *)rx_buff)->size + sizeof(crsp_hdr_t);
}

 /*
 *  Process command response (CRSP)
 */
static void ictrl_serial_upstream_on_rx_crsp(ictrl_serial_upstream_t *stream)
{
    reedl_crsp_t* resp = &stream->resp;
    crsp_hdr_t *crsp_hdr = (crsp_hdr_t*)stream->rx_buff;

    int max_len = MAX(stream->get_len(stream->rx_buff), resp->max_len);

    if (crsp_hdr->mark != DBG_BUFF_MARK_RSP) {
        DBG_ERR("Stream error %s@%d", __func__, __LINE__);
        while (1);
    }

    // Everething is OK here proceed copy response and notify subscrber
    {
        uint64_t u = 1;
        memcpy(resp->data.crsp, crsp_hdr, max_len);
        *(resp->err) = 0;
        write(resp->fd_evt, &u, sizeof(uint64_t));
    }

    // Notification is a single-shot only. Flush after use.
    memset(resp, 0, sizeof(reedl_crsp_t));          // !!! fd cleared ?

    // Just for debug, clean RX buff
    memset(stream->rx_buff, 0xCC, sizeof(stream->rx_buff));

    return;
}

static ssize_t ictrl_serial_upstream_get_len_imud(uint8_t *rx_buff)
{
    return ((imud_hdr_t *)rx_buff)->size + sizeof(imud_hdr_t);
}

 /*
 *  Process IMU dump message (IMUD)
 */
static void ictrl_serial_upstream_on_rx_imud(ictrl_serial_upstream_t *stream)
{
    imu_dump_pck_t *imud_pck = (imu_dump_pck_t *)stream->rx_buff;
    
    if (imud_pck->hdr.type != 0xC1) {
        DBG_ERR("Stream error %s@%d", __func__, __LINE__);
        while (1);
    }

    // reedl_ictrl_cam_on_imud(imud_pck); // Not used in SSPIEM

    //memset(stream->rx_buff, 0xCC, sizeof(stream->rx_buff));
    return;
}

uint8_t *ictrl_serial_rx_proc_cont(reedl_ictrl_serial_t *ictrl_serial,
                           uint8_t *rx_buff, ssize_t *recv_len)
{
    ictrl_serial_upstream_t *stream;
    ictrl_serial_rcv_t *receiver = &ictrl_serial->receiver;
    
    int btr;

    // Is new stream started
    if (receiver->curr_stream == NULL) {
        uint8_t stream_hdr = *rx_buff;
        uint8_t stream_id = stream_hdr;

        DBG_LOG("*** New stream started ***");
        // Copy data to stream specific buffer
        // and execute corresponding handler when finished
        if (stream_id < COUNT_OF(receiver->streams)) {
            stream = &receiver->streams[stream_id];
            stream->wr_idx = 0;
            stream->stream_len = 0;
            receiver->curr_stream = stream;
        } else {
            return NULL;
        }

        // Stream header is always 1 byte long
        rx_buff++;
        *recv_len -= 1;
        if (*recv_len == 0)
            return rx_buff;
    }

    // Data received for already started stream
    stream = receiver->curr_stream;
    if (stream->stream_len == 0 ) {
        // Receive stream header 
        btr = stream->hdr_size - stream->wr_idx;
    } else {
        // Receive stream body
        btr = stream->stream_len - stream->wr_idx;
    }
    btr = MIN(btr, *recv_len);

    memcpy(&stream->rx_buff[stream->wr_idx], rx_buff, btr);
    stream->wr_idx += btr;
    rx_buff += btr;
    *recv_len -= btr;
    
    if (stream->wr_idx == stream->hdr_size) {
        // Header received

        ssize_t l = stream->get_len(stream->rx_buff);
        if (l > 0) {
            DBG_LOG("**** Stream Header received %lu", stream->stream_len);
            stream->stream_len = l;        
        } else {
            // Bad stream mark or zero length. Stream is broken
            *recv_len = 0;
        }

    } else if (stream->wr_idx == stream->stream_len) {
        // Call stream handler if number of requested bytes received
        // and initialize new stream reception

        DBG_LOG("**** Stream Body received");

        stream->on_rx(stream);
#if NAV
        ictrl_serial_upstream_on_rx_dbgl;
        ictrl_serial_upstream_on_rx_crsp;
        ictrl_serial_upstream_on_rx_imud;
#endif
        receiver->curr_stream = NULL;
    }

    return rx_buff;
}

void ictrl_serial_rx_proc(reedl_ictrl_serial_t *ictrl_serial, ssize_t recv_len)
{
    uint8_t *rx_buff = ictrl_serial->tty_rx_buf;

    do {
        rx_buff = ictrl_serial_rx_proc_cont(ictrl_serial, rx_buff, &recv_len);
        if (!rx_buff) {
            // Error
            return;
        }
    } while (recv_len);

}

reedl_ictrl_serial_t *reedl_ictrl_serial_init(reedl_ictrl_serial_cfg_t *cfg)
{
    int i;
    reedl_ictrl_serial_t *ictrl_serial = reedl_malloc(sizeof(reedl_ictrl_serial_t));
    memset(ictrl_serial, 0, sizeof(reedl_ictrl_serial_t));
    strncpy(ictrl_serial->signature, "ictrl", sizeof(ictrl_serial->signature)-1);

    ictrl_serial->cfg = *cfg;
    ictrl_serial->fd_tty = -1;
    ictrl_serial->fd_idle_timer = -1;

    if (0 != timer_create_set(&ictrl_serial->fd_idle_timer, 
        SERIAL_IDLE_TIMER_INTERVAL_MS, "IDLE")) {
        return NULL;
    }

    for (i = 0; i < COUNT_OF(ictrl_serial->receiver.streams); i++) {
        ictrl_serial_upstream_t *stream = &ictrl_serial->receiver.streams[i];
        stream->idx = i;
        stream->wr_idx = 0;
        memset(stream->rx_buff, 0, sizeof(stream->rx_buff));
    }

    /* Register streams */
    ictrl_serial->receiver.streams[DBG_BUFF_IDX_DBGL].hdr_size =
            sizeof(dbgl_hdr_t);
    ictrl_serial->receiver.streams[DBG_BUFF_IDX_DBGL].on_rx =
            ictrl_serial_upstream_on_rx_dbgl;
    ictrl_serial->receiver.streams[DBG_BUFF_IDX_DBGL].get_len =
            ictrl_serial_upstream_get_len_dbgl,

    ictrl_serial->receiver.streams[DBG_BUFF_IDX_CRSP].hdr_size =
            sizeof(crsp_hdr_t);
    ictrl_serial->receiver.streams[DBG_BUFF_IDX_CRSP].on_rx =
            ictrl_serial_upstream_on_rx_crsp;
    ictrl_serial->receiver.streams[DBG_BUFF_IDX_CRSP].get_len =
            ictrl_serial_upstream_get_len_crsp;

    ictrl_serial->receiver.streams[DBG_BUFF_IDX_IMUD].hdr_size =
            sizeof(imud_hdr_t);
    ictrl_serial->receiver.streams[DBG_BUFF_IDX_IMUD].on_rx =
            ictrl_serial_upstream_on_rx_imud;
    ictrl_serial->receiver.streams[DBG_BUFF_IDX_IMUD].get_len =
            ictrl_serial_upstream_get_len_imud;

    return ictrl_serial;
}

void reedl_ictrl_serial_check_fds (
    reedl_ictrl_serial_t *ictrl_serial, fd_set *rd_fds, fd_set *wr_fds)
{
    if (!ictrl_serial)
        return;

    if (FD_ISSET(ictrl_serial->fd_idle_timer, rd_fds)) {
        uint64_t is_trig;
        read(ictrl_serial->fd_idle_timer, &is_trig, sizeof(uint64_t));
        if (is_trig) {
            // ictrl_serial_on_idle(ictrl_serial);
        }
    }

    if ((-1 != ictrl_serial->fd_tty) && FD_ISSET(ictrl_serial->fd_tty, rd_fds)) {
        ssize_t recv_len = read(ictrl_serial->fd_tty, ictrl_serial->tty_rx_buf,
                                sizeof(ictrl_serial->tty_rx_buf));
        if (recv_len > 0) {
            ictrl_serial_rx_proc(ictrl_serial, recv_len);
        } else {
            int err = errno;
            if (err != EAGAIN) {
                DBG_ERR("TTY error - %s (%d)", strerror(err), err);
                ictrl_serial_tty_close(ictrl_serial);
            }
        }
    }
}

int reedl_ictrl_serial_set_fds(reedl_ictrl_serial_t *ictrl_serial, int fd_max,
                           fd_set *rd_fds, fd_set *wr_fds)
{
    if (-1 != ictrl_serial->fd_idle_timer) {
        FD_SET(ictrl_serial->fd_idle_timer, rd_fds);
        fd_max = MAX(ictrl_serial->fd_idle_timer, fd_max);
    }

    if (-1 != ictrl_serial->fd_tty) {
        FD_SET(ictrl_serial->fd_tty, rd_fds);
        fd_max = MAX(ictrl_serial->fd_tty, fd_max);
    }

    return fd_max;
}

int reedl_ictrl_serial_tx(reedl_ictrl_serial_t *ictrl_serial,
                      uint8_t *data, uint32_t bytes_to_write, char *comment)
{
    int rc = 0;
    ssize_t written;
    if (-1 == ictrl_serial->fd_tty) {
        DBG_ERR("ICTRL --> HOST: %ls ignored - not connected", comment);
        rc = -1;
    } 
    if (!rc) {
        written = write(ictrl_serial->fd_tty, data, bytes_to_write);
        if (written != bytes_to_write) {
            rc = -2;
            DBG_ERR("ICTRL --> HOST: error %d", errno);
        } else {
            DBG_LOG("ICTRL --> HOST: %ls", comment);
        }
    }

    return rc;
}


void reedl_ictrl_serial_subscribe_crsp(reedl_ictrl_serial_t* ictrl_serial,
    uint8_t* data, uint32_t max_len, int fd_evt)
{
    reedl_crsp_t *resp = &ictrl_serial->receiver.streams[DBG_BUFF_IDX_CRSP].resp;

    *(resp->err) = 0;
    resp->data.raw = data;
    resp->max_len = max_len;
    if (-1 != resp->fd_evt) {
        close(resp->fd_evt);
    }
    resp->fd_evt = fd_evt;
}
    
static void reedl_ictrl_serial_loop(reedl_ictrl_serial_t* ictrl_serial)
{
    fd_set rd_fds, wr_fds, err_fds;
    struct timeval tv;
    int rc;
    int fd_max = 0;

    while (!g_terminate) {
        FD_ZERO(&rd_fds);
        FD_ZERO(&wr_fds);
        FD_ZERO(&err_fds);

        fd_max = reedl_ictrl_serial_set_fds(ictrl_serial, fd_max,
            &rd_fds, &wr_fds);

        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms looping
        rc = select(fd_max + 1, &rd_fds, &wr_fds, &err_fds, &tv);

        if (rc < 0) {
            if (errno != EAGAIN) {
                // For ex. errno = EINTR  on SIGINT
                syslog(LOG_WARNING, "Select aborted: %s", strerror(errno));
                DBG_INFO("Select aborted: %s", strerror(errno));
                // TODO: execute close for all submodules from the idle loop check fds
                break;
            }
        }
        else if (rc > 0) {
            reedl_ictrl_serial_check_fds(ictrl_serial, &rd_fds, &wr_fds);
        }
    }
    syslog(LOG_INFO, "Select aborted: %s", strerror(errno));
    DBG_INFO("Select aborted: %s", strerror(errno));
}
static void* reedl_ictrl_serial_thread(void* arg)
{
    reedl_ictrl_t* reedl_ictrl = (reedl_ictrl_t*)arg;
    reedl_ictrl_serial_loop(reedl_ictrl->serial);
}

int reedl_ictrl_init(const char *port_name)
{
    int rc, ret;
    pthread_attr_t attr;

    reedl_ictrl_t* ictrl = &g_ictrl;

    if (port_name) {
        ictrl->cfg.port = port_name;
    }
    ictrl->serial = reedl_ictrl_serial_init(&ictrl->cfg);

    rc = reedl_ictrl_serial_open(ictrl->serial);
    if (rc) {
        return -1;
    }

    pthread_attr_init(&attr);
    ret = pthread_create(
        &ictrl->thread,
        &attr,
        reedl_ictrl_serial_thread,
        ictrl);

    ret = pthread_attr_destroy(&attr);


	// Sanity check
	//	- Check ICTRL compatibility. Read & check ATIC signature.
	//  - Check ATIC HW. Read & check IMON voltages
	// Put FPGA into default state
	//  - CDONE control
	//  - RST control
    

    return ret;
}
