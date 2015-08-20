/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __AE_H__
#define __AE_H__

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
/* 开启这个标志后 epoll 的 wait time 为 0 */
#define AE_DONT_WAIT 4

#define AE_NOMORE -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* File event structure */
typedef struct aeFileEvent {
	/* 事件类型 */
    int mask; /* one of AE_(READABLE|WRITABLE) */
	/* 回调函数 */
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;

/* Time event structure */
typedef struct aeTimeEvent {
    long long id; /* time event identifier. */
	/* 事件触发时间 */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
	/* 事件处理的回调函数, 在 processTimeEvents 中调用, 初始时有一个
	 * redis.c serverCron() 的 time event, 当返回值不为 AE_NOMORE(-1) 时, 则
	 * 该 time event 不从 time event list 中删除, 且下一次事件触发时间为
	 * (当前时间+返回值), 返回值单位为 毫秒; 当返回值为 AE_NOMORE 时, 则删除
	 * 该事件 */
    aeTimeProc *timeProc;
	/* 删除一个 time event 时调用, serverCron() 事件的这个函数为 NULL */
    aeEventFinalizerProc *finalizerProc;
	/* serverCron() 事件的这个域为 NULL */
    void *clientData;
	/* 链表结构, 下一个事件指针 */
    struct aeTimeEvent *next;
} aeTimeEvent;

/* A fired event */
typedef struct aeFiredEvent {
    int fd;
	/* 可读, 可写标记 */
    int mask;
} aeFiredEvent;

/* State of an event based program */
typedef struct aeEventLoop {
	/* 初始为 -1 */
    int maxfd;   /* highest file descriptor currently registered */
	/* 初始为 server.maxclients+REDIS_EVENTLOOP_FDSET_INCR */
    int setsize; /* max number of file descriptors tracked */
	/* 初始为 0 */
    long long timeEventNextId;
	/* 最后一次处理 time events 的时间 */
    time_t lastTime;     /* Used to detect system clock skew */
	/* 用文件描述符索引这个数组, 大小为 setsize */
    aeFileEvent *events; /* Registered events */
	/* 就绪的事件, 大小为 setsize */
    aeFiredEvent *fired; /* Fired events */
	/* 链表头结点(有效) */
    aeTimeEvent *timeEventHead;
	/* 控制 ae stop */
    int stop;
	/* 采用 epoll 时类型为 aeApiState, 采用不同的实现时这个类型不一样 */
    void *apidata; /* This is used for polling API specific data */
	/* 函数, 在 aeMain 中调用, 初始化设置为 redis.c beforeSleep() */
    aeBeforeSleepProc *beforesleep;
} aeEventLoop;

/* Prototypes */
aeEventLoop *aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);
int aeGetSetSize(aeEventLoop *eventLoop);
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);

#endif
