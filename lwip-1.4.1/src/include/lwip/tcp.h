/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __LWIP_TCP_H__
#define __LWIP_TCP_H__

#include "lwip/opt.h"

#if LWIP_TCP /* don't build if not configured for use in lwipopts.h */

#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/ip.h"
#include "lwip/icmp.h"
#include "lwip/err.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tcp_pcb;

/** Function prototype for tcp accept callback functions. Called when a new
 * connection can be accepted on a listening pcb.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param newpcb The new connection pcb
 * @param err An error code if there has been an error accepting.
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
typedef err_t (*tcp_accept_fn)(void *arg, struct tcp_pcb *newpcb, err_t err);

/** Function prototype for tcp receive callback functions. Called when data has
 * been received.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
typedef err_t (*tcp_recv_fn)(void *arg, struct tcp_pcb *tpcb,
                             struct pbuf *p, err_t err);

/** Function prototype for tcp sent callback functions. Called when sent data has
 * been acknowledged by the remote side. Use it to free corresponding resources.
 * This also means that the pcb has now space available to send new data.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb for which data has been acknowledged
 * @param len The amount of bytes acknowledged
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
typedef err_t (*tcp_sent_fn)(void *arg, struct tcp_pcb *tpcb,
                              u16_t len);

/** Function prototype for tcp poll callback functions. Called periodically as
 * specified by @see tcp_poll.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb tcp pcb
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
typedef err_t (*tcp_poll_fn)(void *arg, struct tcp_pcb *tpcb);

/** Function prototype for tcp error callback functions. Called when the pcb
 * receives a RST or is unexpectedly closed for any other reason.
 *
 * @note The corresponding pcb is already freed when this callback is called!
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param err Error code to indicate why the pcb has been closed
 *            ERR_ABRT: aborted through tcp_abort or by a TCP timer
 *            ERR_RST: the connection was reset by the remote host
 */
typedef void  (*tcp_err_fn)(void *arg, err_t err);

/** Function prototype for tcp connected callback functions. Called when a pcb
 * is connected to the remote side after initiating a connection attempt by
 * calling tcp_connect().
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which is connected
 * @param err An unused error code, always ERR_OK currently ;-) TODO!
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 *
 * @note When a connection attempt fails, the error callback is currently called!
 */
typedef err_t (*tcp_connected_fn)(void *arg, struct tcp_pcb *tpcb, err_t err);

enum tcp_state {
  CLOSED      = 0,        //没有连接
  LISTEN      = 1,        //进入侦听，等待客户端的连接请求                (服务器)
  SYN_SENT    = 2,        //发送连接请求(发送SYS=1的报文)                 (客户端)
  SYN_RCVD    = 3,        //已收到对方的连接请求(接收到SYS=1的报文)       (服务器)
  ESTABLISHED = 4,        //连接已建立(三次握手成功)
  FIN_WAIT_1  = 5,        //关闭当前的连接(发送FIN=1的报文)               (客户端)
  FIN_WAIT_2  = 6,        //另一端已接受关闭该链接(发送FIN，接收到ACK)    (客户端)
  CLOSE_WAIT  = 7,        //等待程序关闭连接(接收到FIN=1的报文)           (服务器)
  CLOSING     = 8,        //两端同时收到对方的关闭请求
  LAST_ACK    = 9,        //服务器等待对方接收关闭操作(发送fin报文，等待客户端返回ack,收到ack进入CLOSED状态)  (服务器)
  TIME_WAIT   = 10        //关闭成功，等待网路中可能出现的剩余数据 2msl  msl=报文最大生存时间  lwip中为60s
};       //接收到服务器FIN=1的报文     (客户端)

#if LWIP_CALLBACK_API
  /* Function to call when a listener has been connected.
   * @param arg user-supplied argument (tcp_pcb.callback_arg)
   * @param pcb a new tcp_pcb that now is connected
   * @param err an error argument (TODO: that is current always ERR_OK?)
   * @return ERR_OK: accept the new connection,
   *                 any other err_t abortsthe new connection
   */
#define DEF_ACCEPT_CALLBACK  tcp_accept_fn accept;
#else /* LWIP_CALLBACK_API */
#define DEF_ACCEPT_CALLBACK
#endif /* LWIP_CALLBACK_API */

/**
 * members common to struct tcp_pcb and struct tcp_listen_pcb
 */
#define TCP_PCB_COMMON(type) \
  type *next; /* for the linked list */ \
  void *callback_arg; \
  /* the accept callback for listen- and normal pcbs, if LWIP_CALLBACK_API */ \
  DEF_ACCEPT_CALLBACK \      //回调函数，侦听到连接时会调用
  enum tcp_state state; /* TCP state */ \    //连接的状态
  u8_t prio; \       //优先级，可用于回收低优先级控制块
  /* ports are in host byte order */ \
  u16_t local_port       //本地端口


/* the TCP protocol control block */
struct tcp_pcb {
/** common PCB members */
  IP_PCB;
/** protocol specific PCB members */
  TCP_PCB_COMMON(struct tcp_pcb);

  /* ports are in host byte order */
  u16_t remote_port;    //远端端口号
  
  u8_t flags;     //控制块状态，标志字段
#define TF_ACK_DELAY   ((u8_t)0x01U)   /* Delayed ACK. */      /* 延迟确认*/  
#define TF_ACK_NOW     ((u8_t)0x02U)   /* Immediate ACK. */      /* 立即确认*/  
#define TF_INFR        ((u8_t)0x04U)   /* In fast recovery. */    //连接处于快速重传
#define TF_TIMESTAMP   ((u8_t)0x08U)   /* Timestamp option enabled */   //时间戳选项使能
#define TF_RXCLOSED    ((u8_t)0x10U)   /* rx closed by tcp_shutdown */
#define TF_FIN         ((u8_t)0x20U)   /* Connection was closed locally (FIN segment enqueued). */  //应用程序已关闭该连接
#define TF_NODELAY     ((u8_t)0x40U)   /* Disable Nagle algorithm */      /* 关闭Nagle算法*/  
#define TF_NAGLEMEMERR ((u8_t)0x80U)   /* nagle enabled, memerr, try to output to prevent delayed ACK to happen */

  /* the rest of the fields are in host byte order
     as we have to do some math with them */

  /* Timers */
  u8_t polltmr, pollinterval;   /*poll计时器*/  
  u8_t last_timer;
  u32_t tmr;     /*该连接上次有数据包到达的时间*/  
  
                      //当接收到数据后，数据会放在接收窗口中等待上层取用，窗口值会变小，当上层取走数据后，窗口值会变大,rcv_wnd值变化时，内核会计算一个合理的                   
  /* receiver variables */  //rcv_ann_wnd值，下一次报文发送时填入首部，rcv_ann_right_edge在报文发送后值更新。

	/**接收窗口相关变量*/  
  u32_t rcv_nxt;   /* next seqno expected */   //下一个期望接收的字节序列
  u16_t rcv_wnd;   /* receiver window available */     //当前接收窗口大小()
  u16_t rcv_ann_wnd; /* receiver window to announce */   //将向对方通告的窗口大小
  u32_t rcv_ann_right_edge; /* announced right edge of window */  //上一次窗口通告时窗口的右边界值

  /* Retransmission timer. */
  s16_t rtime;       /*重传计时器*/  

  u16_t mss;   /* maximum segment size */ /*最大分段大小，lwip中实际发送的数据包，都是以mss大小进行切分的*/  

  /* RTT (round trip time) estimation variables */  /**RTT (round trip time)相关变量*/  
  u32_t rttest; /* RTT estimate in 500ms ticks */       /*用于计算RTT的数据分片的发送时间（500ms为单位的整数）*/  
  u32_t rtseq;  /* sequence number being timed */    //正在进行往返时间估计的报文序列号
  s16_t sa, sv; /* @todo document this */

  s16_t rto;    /* retransmission time-out */     //重发超时时间
  u8_t nrtx;    /* number of retransmissions */    //重发次数

  /* fast retransmit/recovery */ 
  u8_t dupacks;            //最大序列号被重复接收的次数
  u32_t lastack; /* Highest acknowledged seqno. */   //记录被接收方确认的数据的最高序列号，当接收到接收方的ack后，值增加，

	/**拥塞控制相关变量*/  
  /* congestion avoidance/control variables */
  u16_t cwnd;       /*拥塞避免窗口*/  
  u16_t ssthresh;    /*慢启动门限*/  
  
	    /**发送窗口相关变量*/  
  /* sender variables */
  u32_t snd_nxt;   /* next new seqno to be sent */   //自己下一个将要发送的数据的序号
  u32_t snd_wl1, snd_wl2; /* Sequence and acknowledgement numbers of last    //上次窗口更新时接收到的数据序号和确认序号
                             window update. */
  u32_t snd_lbb;       /* Sequence number of next byte to be buffered. */   //记录下一个被应用缓存的数据的起始编号。
  u16_t snd_wnd;   /* sender window */       //发送窗口大小，一般被设置为首部通告的接收窗口值。
  u16_t snd_wnd_max; /* the maximum sender window announced by the remote host */

  u16_t acked;

  u16_t snd_buf;   /* Available buffer space for sending (in bytes). */
#define TCP_SNDQUEUELEN_OVERFLOW (0xffffU-3)
  u16_t snd_queuelen; /* Available buffer space for sending (in tcp_segs). */

#if TCP_OVERSIZE
  /* Extra bytes available at the end of the last pbuf in unsent. */
  u16_t unsent_oversize;
#endif /* TCP_OVERSIZE */ 

  /* These are ordered by sequence number: */        //三个缓冲队列，分别为队列的首指针
  struct tcp_seg *unsent;   /* Unsent (queued) segments. */         //连接还未发送出去的报文段
  struct tcp_seg *unacked;  /* Sent but unacknowledged segments. */     //连接已经发送出去，但还未被确认的报文段
#if TCP_QUEUE_OOSEQ  
  struct tcp_seg *ooseq;    /* Received out of sequence segments. */     //连接收到的无序报文段
#endif /* TCP_QUEUE_OOSEQ */

  struct pbuf *refused_data; /* Data previously received but not yet taken by upper layer */

#if LWIP_CALLBACK_API
  /* Function to be called when more send buffer space is available. */
  tcp_sent_fn sent;                    //数据发送成功后被调用
  /* Function to be called when (in-sequence) data has arrived. */
  tcp_recv_fn recv;                     //接收到数据后被调用
  /* Function to be called when a connection has been set up. */
  tcp_connected_fn connected;             //连接建立后被调用
  /* Function which is called periodically. */
  tcp_poll_fn poll;               //被内核周期性调用
  /* Function to be called whenever a fatal error occurs. */
  tcp_err_fn errf;                             //连接发生错误时调用
#endif /* LWIP_CALLBACK_API */

/**时间戳选项相关变量*/    
#if LWIP_TCP_TIMESTAMPS   
  u32_t ts_lastacksent;
  u32_t ts_recent;
#endif /* LWIP_TCP_TIMESTAMPS */

	/**保活定时器相关变量*/  
  /* idle time before KEEPALIVE is sent */
  u32_t keep_idle; /*间隔多长时间开始发送 keep alive消息*/  
#if LWIP_TCP_KEEPALIVE
  u32_t keep_intvl; /*TCP选项keepa live中的值,发送keepalive时间间隔*/  
  u32_t keep_cnt;  /*TCP选项keep live 最多发送多少次，最后这个连接就被rst了*/  
#endif /* LWIP_TCP_KEEPALIVE */
  
  /* Persist timer counter */
  u8_t persist_cnt;  /*坚持定时器退避时间*/  
  /* Persist timer back-off */
  u8_t persist_backoff; /*坚持计时器指数退避参数*/  

  /* KEEPALIVE counter */
  u8_t keep_cnt_sent;/*已经发送的保活消息次数*/  
};

struct tcp_pcb_listen {  
/* Common members of all PCB types */
  IP_PCB;
/* Protocol specific PCB members */
  TCP_PCB_COMMON(struct tcp_pcb_listen);

#if TCP_LISTEN_BACKLOG
  u8_t backlog;
  u8_t accepts_pending;
#endif /* TCP_LISTEN_BACKLOG */
};

#if LWIP_EVENT_API

enum lwip_event {
  LWIP_EVENT_ACCEPT,
  LWIP_EVENT_SENT,
  LWIP_EVENT_RECV,
  LWIP_EVENT_CONNECTED,
  LWIP_EVENT_POLL,
  LWIP_EVENT_ERR
};

err_t lwip_tcp_event(void *arg, struct tcp_pcb *pcb,
         enum lwip_event,
         struct pbuf *p,
         u16_t size,
         err_t err);

#endif /* LWIP_EVENT_API */

/* Application program's interface: */
struct tcp_pcb * tcp_new     (void);

void             tcp_arg     (struct tcp_pcb *pcb, void *arg);
void             tcp_accept  (struct tcp_pcb *pcb, tcp_accept_fn accept);
void             tcp_recv    (struct tcp_pcb *pcb, tcp_recv_fn recv);
void             tcp_sent    (struct tcp_pcb *pcb, tcp_sent_fn sent);
void             tcp_poll    (struct tcp_pcb *pcb, tcp_poll_fn poll, u8_t interval);
void             tcp_err     (struct tcp_pcb *pcb, tcp_err_fn err);

#define          tcp_mss(pcb)             (((pcb)->flags & TF_TIMESTAMP) ? ((pcb)->mss - 12)  : (pcb)->mss)
#define          tcp_sndbuf(pcb)          ((pcb)->snd_buf)
#define          tcp_sndqueuelen(pcb)     ((pcb)->snd_queuelen)
#define          tcp_nagle_disable(pcb)   ((pcb)->flags |= TF_NODELAY)
#define          tcp_nagle_enable(pcb)    ((pcb)->flags &= ~TF_NODELAY)
#define          tcp_nagle_disabled(pcb)  (((pcb)->flags & TF_NODELAY) != 0)

#if TCP_LISTEN_BACKLOG
#define          tcp_accepted(pcb) do { \
  LWIP_ASSERT("pcb->state == LISTEN (called for wrong pcb?)", pcb->state == LISTEN); \
  (((struct tcp_pcb_listen *)(pcb))->accepts_pending--); } while(0)
#else  /* TCP_LISTEN_BACKLOG */
#define          tcp_accepted(pcb) LWIP_ASSERT("pcb->state == LISTEN (called for wrong pcb?)", \
                                               (pcb)->state == LISTEN)
#endif /* TCP_LISTEN_BACKLOG */

void             tcp_recved  (struct tcp_pcb *pcb, u16_t len);
err_t            tcp_bind    (struct tcp_pcb *pcb, ip_addr_t *ipaddr,
                              u16_t port);
err_t            tcp_connect (struct tcp_pcb *pcb, ip_addr_t *ipaddr,
                              u16_t port, tcp_connected_fn connected);

struct tcp_pcb * tcp_listen_with_backlog(struct tcp_pcb *pcb, u8_t backlog);
#define          tcp_listen(pcb) tcp_listen_with_backlog(pcb, TCP_DEFAULT_LISTEN_BACKLOG)

void             tcp_abort (struct tcp_pcb *pcb);
err_t            tcp_close   (struct tcp_pcb *pcb);
err_t            tcp_shutdown(struct tcp_pcb *pcb, int shut_rx, int shut_tx);

/* Flags for "apiflags" parameter in tcp_write */
#define TCP_WRITE_FLAG_COPY 0x01
#define TCP_WRITE_FLAG_MORE 0x02

err_t            tcp_write   (struct tcp_pcb *pcb, const void *dataptr, u16_t len,
                              u8_t apiflags);

void             tcp_setprio (struct tcp_pcb *pcb, u8_t prio);

#define TCP_PRIO_MIN    1
#define TCP_PRIO_NORMAL 64
#define TCP_PRIO_MAX    127

err_t            tcp_output  (struct tcp_pcb *pcb);


const char* tcp_debug_state_str(enum tcp_state s);


#ifdef __cplusplus
}
#endif

#endif /* LWIP_TCP */

#endif /* __LWIP_TCP_H__ */

