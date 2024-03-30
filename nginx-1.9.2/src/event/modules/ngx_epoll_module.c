
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

#if (NGX_TEST_BUILD_EPOLL)

/* epoll declarations */
/*
epoll_event��events��ȡֵ����
�������������������ש�������������������������������������������������������������������������������
��    eventsȡֵ  ��    ����                                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLIN         ��                                                                              ��
��                ��  ��ʾ��Ӧ�������������ݿ��Զ�����TCP���ӵ�Զ�������ر����ӣ�Ҳ�൱�ڿɶ���   ��
��                ��������Ϊ��Ҫ����������FIN����                                               ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLOUT        ��  ��ʾ��Ӧ�������Ͽ���д�����ݷ��ͣ����������η����������������TCP���ӣ����� ��
��                �������ɹ����¼��൱�ڿ�д�¼���                                                ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLRDHUP      ��  ��ʾTCP���ӵ�Զ�˹رջ��ر�����                                           ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLPRI        ��  ��ʾ��Ӧ���������н���������Ҫ��                                            ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLERR        ��  ��ʾ��Ӧ�����ӷ�������                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLHUP        ��  ��ʾ��Ӧ�����ӱ�����                                                        ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLET         ��  ��ʾ��������ʽ��������Ե����(ET)��ϵͳĬ��Ϊˮƽ����(LT��)                  ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLONESHOT    ��  ��ʾ������¼�ֻ����һ�Σ��´���Ҫ����ʱ�����¼���epoll                     ��
�������������������ߩ�������������������������������������������������������������������������������

����Linux Epoll ETģʽEPOLLOUT��EPOLLIN����ʱ�� ETģʽ��Ϊ��Ե����ģʽ������˼�壬������Ե������ǲ��ᴥ���ġ�

Epoll ET -> EPOLLOUT�¼���
EPOLLOUT�¼�ֻ��������ʱ����һ�Σ���ʾ��д������ʱ����Ҫ����������Ҫ��׼��������������
1.ĳ��write��д���˷��ͻ����������ش�����ΪEAGAIN��
2.�Զ˶�ȡ��һЩ���ݣ������¿�д�ˣ���ʱ�ᴥ��EPOLLOUT��
�򵥵�˵��EPOLLOUT�¼�ֻ���ڲ���д����д��ת��ʱ�̣��Żᴥ��һ�Σ����Խб�Ե��������з�û��ģ�

��ʵ������������ǿ�ƴ���һ�Σ�Ҳ���а취�ģ�ֱ�ӵ���epoll_ctl��������һ��event�Ϳ����ˣ�event��ԭ��������һģһ�����У����������EPOLLOUT�����ؼ����������ã��ͻ����ϴ���һ��EPOLLOUT�¼���

Epoll ET -> EPOLLIN�¼���
EPOLLIN�¼���ֻ�е��Զ�������д��ʱ�Żᴥ�������Դ���һ�κ���Ҫ���϶�ȡ��������ֱ������EAGAINΪֹ������ʣ�µ�����ֻ�����´ζԶ���д��ʱ����һ��ȡ�����ˡ�


*/
#define EPOLLIN        0x001
#define EPOLLPRI       0x002
#define EPOLLOUT       0x004
#define EPOLLRDNORM    0x040
#define EPOLLRDBAND    0x080
#define EPOLLWRNORM    0x100
#define EPOLLWRBAND    0x200
#define EPOLLMSG       0x400
#define EPOLLERR       0x008 //EPOLLERR|EPOLLHUP����ʾ�����쳣���  fd�������������  //epoll EPOLLERR|EPOLLHUPʵ������ͨ��������д�¼����ж�д����recv write����������쳣
#define EPOLLHUP       0x010


#define EPOLLRDHUP     0x2000 //���Զ��Ѿ��رգ�����д���ݣ���������¼�

#define EPOLLET        0x80000000 //��ʾ��������ʽ��������Ե����(ET)��ϵͳĬ��Ϊˮƽ����(LT��)  
//���øñ�Ǻ��ȡ�����ݺ�������ڴ������ݹ����������µ����ݵ��������ᴥ��epoll_wait���أ��������ݴ�����Ϻ�����add epoll_ctl�������ο�<linux�����ܷ���������>9.3.4��
#define EPOLLONESHOT   0x40000000

#define EPOLL_CTL_ADD  1
#define EPOLL_CTL_DEL  2
#define EPOLL_CTL_MOD  3

typedef union epoll_data {
    void         *ptr; //��ngx_epoll_add_event�и�ֵΪngx_connection_t����
    int           fd;
    uint32_t      u32;
    uint64_t      u64;
} epoll_data_t;

struct epoll_event {
    uint32_t      events;
    epoll_data_t  data;
};


int epoll_create(int size);

int epoll_create(int size)
{
    return -1;
}


int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
//EPOLL_CTL_ADDһ�κ󣬾Ϳ���һֱͨ��epoll_wait����ȡ���¼������ǵ���EPOLL_CTL_DEL������ÿ�ζ��¼�����epoll_wait���غ�Ҫ�������EPOLL_CTL_ADD��
//֮ǰ�������еĵط�����ע���ˣ���עΪÿ�ζ��¼�������Ҫ����addһ�Σ�epoll_ctr addд�¼�һ��
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    return -1;
}


int epoll_wait(int epfd, struct epoll_event *events, int nevents, int timeout);

/*
//ngx_notify->ngx_epoll_notifyֻ�ᴥ��epoll_in������ͬʱ����epoll_out�������������¼�epoll_in,���ͬʱ����epoll_out
*/
int epoll_wait(int epfd, struct epoll_event *events, int nevents, int timeout) //timeoutΪ-1��ʾ���޵ȴ�
{
    return -1;
}

#if (NGX_HAVE_EVENTFD)
#define SYS_eventfd       323
#endif

#if (NGX_HAVE_FILE_AIO)

#define SYS_io_setup      245
#define SYS_io_destroy    246
#define SYS_io_getevents  247

typedef u_int  aio_context_t;

struct io_event {
    uint64_t  data;  /* the data field from the iocb */ //���ύ�¼�ʱ��Ӧ��iocb�ṹ���е�aio_data��һ�µ�
    uint64_t  obj;   /* what iocb this event came from */ //ָ���ύ�¼�ʱ��Ӧ��iocb�ṹ��
    int64_t   res;   /* result code for this event */
    int64_t   res2;  /* secondary result */
};


#endif
#endif /* NGX_TEST_BUILD_EPOLL */

//��Ա��Ϣ��ngx_epoll_commands
typedef struct {
    /*
    events�ǵ���epoll_wait����ʱ���˵ĵ�3������maxevents������2������events����Ĵ�СҲ�����������ģ����潫��ngx_epoll_init�����г�ʼ���������
     */
    ngx_uint_t  events; // "epoll_events"��������  Ĭ��512 ��ngx_epoll_init_conf
    ngx_uint_t  aio_requests; // "worker_aio_requests"��������  Ĭ��32 ��ngx_epoll_init_conf
} ngx_epoll_conf_t;


static ngx_int_t ngx_epoll_init(ngx_cycle_t *cycle, ngx_msec_t timer);
#if (NGX_HAVE_EVENTFD)
static ngx_int_t ngx_epoll_notify_init(ngx_log_t *log);
static void ngx_epoll_notify_handler(ngx_event_t *ev);
#endif
static void ngx_epoll_done(ngx_cycle_t *cycle);
static ngx_int_t ngx_epoll_add_event(ngx_event_t *ev, ngx_int_t event,
    ngx_uint_t flags);
static ngx_int_t ngx_epoll_del_event(ngx_event_t *ev, ngx_int_t event,
    ngx_uint_t flags);
static ngx_int_t ngx_epoll_add_connection(ngx_connection_t *c);
static ngx_int_t ngx_epoll_del_connection(ngx_connection_t *c,
    ngx_uint_t flags);
#if (NGX_HAVE_EVENTFD)
static ngx_int_t ngx_epoll_notify(ngx_event_handler_pt handler);
#endif
static ngx_int_t ngx_epoll_process_events(ngx_cycle_t *cycle, ngx_msec_t timer,
    ngx_uint_t flags);

#if (NGX_HAVE_FILE_AIO)
static void ngx_epoll_eventfd_handler(ngx_event_t *ev);
#endif

static void *ngx_epoll_create_conf(ngx_cycle_t *cycle);
static char *ngx_epoll_init_conf(ngx_cycle_t *cycle, void *conf);

static int                  ep = -1; //ngx_epoll_init -> epoll_create�ķ���ֵ
static struct epoll_event  *event_list; //epoll_events��sizeof(struct epoll_event) * nevents, ��ngx_epoll_init
static ngx_uint_t           nevents; //nerentsҲ��������epoll_events�Ĳ���

#if (NGX_HAVE_EVENTFD)
//ִ��ngx_epoll_notify���ͨ��epoll_wait����ִ�иú���ngx_epoll_notify_handler
//ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�
static int                  notify_fd = -1; //��ʼ����ngx_epoll_notify_init
static ngx_event_t          notify_event;
static ngx_connection_t     notify_conn;
#endif

#if (NGX_HAVE_FILE_AIO)
//����֪ͨ�첽I/O�¼���������������iocb�ṹ���е�aio_resfd��Ա��һ�µ�  ͨ����fd��ӵ�epoll�¼��У��Ӷ����Լ���첽io�¼�
int                         ngx_eventfd = -1;
//�첽I/O�������ģ�ȫ��Ψһ�����뾭��io_setup��ʼ������ʹ��
aio_context_t               ngx_aio_ctx = 0; //ngx_epoll_aio_init->io_setup����
//�첽I/O�¼���ɺ����֪ͨ����������Ҳ����ngx_eventfd����Ӧ��ngx_event_t�¼�
static ngx_event_t          ngx_eventfd_event; //���¼�
//�첽I/O�¼���ɺ����֪ͨ��������ngx_eventfd����Ӧ��ngx_connectiont����
static ngx_connection_t     ngx_eventfd_conn;

#endif

static ngx_str_t      epoll_name = ngx_string("epoll");

static ngx_command_t  ngx_epoll_commands[] = {
    /*
    �ڵ���epoll_waitʱ�����ɵ�2�͵�3����������Linux�ں�һ�����ɷ��ض��ٸ��¼�������������ʾ����һ��epoll_waitʱ���ɷ���
    ���¼�������Ȼ����Ҳ��Ԥ������ô��epoll_event�ṹ�����ڴ洢�¼�
     */
    { ngx_string("epoll_events"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      0,
      offsetof(ngx_epoll_conf_t, events),
      NULL },

    /*
    �ڿ����첽I/O��ʹ��io_setupϵͳ���ó�ʼ���첽I/O�����Ļ���ʱ����ʼ������첽I/O�¼�����
     */
    { ngx_string("worker_aio_requests"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      0,
      offsetof(ngx_epoll_conf_t, aio_requests),
      NULL },

      ngx_null_command
};


ngx_event_module_t  ngx_epoll_module_ctx = {
    &epoll_name,
    ngx_epoll_create_conf,               /* create configuration */
    ngx_epoll_init_conf,                 /* init configuration */

    {
        ngx_epoll_add_event,             /* add an event */  //ngx_add_event
        ngx_epoll_del_event,             /* delete an event */
        ngx_epoll_add_event,             /* enable an event */ //ngx_add_conn
        ngx_epoll_del_event,             /* disable an event */
        ngx_epoll_add_connection,        /* add an connection */
        ngx_epoll_del_connection,        /* delete an connection */
#if (NGX_HAVE_EVENTFD)
        ngx_epoll_notify,                /* trigger a notify */
#else
        NULL,                            /* trigger a notify */
#endif
        ngx_epoll_process_events,        /* process the events */
        ngx_epoll_init,                  /* init the events */ //�ڴ������ӽ�����ִ��
        ngx_epoll_done,                  /* done the events */
    }
};

ngx_module_t  ngx_epoll_module = {
    NGX_MODULE_V1,
    &ngx_epoll_module_ctx,               /* module context */
    ngx_epoll_commands,                  /* module directives */
    NGX_EVENT_MODULE,                    /* module type */
    NULL,                                /* init master */
    NULL,                                /* init module */
    NULL,                                /* init process */
    NULL,                                /* init thread */
    NULL,                                /* exit thread */
    NULL,                                /* exit process */
    NULL,                                /* exit master */
    NGX_MODULE_V1_PADDING
};


#if (NGX_HAVE_FILE_AIO)

/*
 * We call io_setup(), io_destroy() io_submit(), and io_getevents() directly
 * as syscalls instead of libaio usage, because the library header file
 * supports eventfd() since 0.3.107 version only.
 */
/*
 Linux�ں��ṩ��5��ϵͳ��������ļ��������첽I/O���ܣ�����9-7��
��9-7  Linux�ں��ṩ���ļ��첽1/0��������
�������������������������������������������ש������������������������������������ש�������������������������������
��    ������                              ��    ��������                        ��    ִ������                  ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                                        ��  nr events��ʾ��Ҫ��ʼ�����첽     ��  ��ʼ���ļ��첽I/O�������ģ� ��
��int io_setup(unsigned nr_events,        ��1/0�����Ŀ��Դ�����¼�����С��     ��ִ�гɹ���ctxp���Ƿ��������  ��
��aio context_t *ctxp)                    ������ctxp���ļ��첽1/0������������   ����������������첽1/0������   ��
��                                        ��                                    �������ٿ��Դ���nr- events����  ��
��                                        ����ָ��                              ��                              ��
��                                        ��                                    ����������0��ʾ�ɹ�             ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                                        ��                                    ��  �����ļ��첽1/0�������ġ�   ��
��int io_destroy (aio_context_t ctx)      ��  ctx���ļ��첽1/0��������������    ��                              ��
��                                        ��                                    ������0��ʾ�ɹ�                 ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��int io_submit(aiio_context_t ctx,       ��  ctx���ļ��첽1/0����������������  ��  �ύ�ļ��첽1/0����������   ��
��                                        ��nr��һ���ύ���¼�������cbp����Ҫ   ��                              ��
��long nr, struct iocb *cbp[])            ��                                    ��ֵ��ʾ�ɹ��ύ���¼�����      ��
��                                        ���ύ���¼������е�����Ԫ�ص�ַ      ��                              ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��int io_cancel(aio_context_t ctx, struct ��  ctx���ļ��첽1/0������������      ��  ȡ��֮ǰʹ��io��sumbit�ύ  ��
��                                        ������iocb��Ҫȡ�����첽1/0��������   ����һ���ļ��첽I/O����������   ��
��iocb *iocb, struct io_event *result)    ��                                    ��                              ��
��                                        ��result��ʾ���������ִ�н��        ��O��ʾ�ɹ�                     ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                I                       ��  ctx���ļ��첽1/0������������      ��                              ��
��int io_getevents(aio_context_t ctx,     ������min_nr��ʾ����Ҫ��ȡmln_ nr��   ��                              ��
��long min_nr, lon, g nr,                 ���¼�����nr��ʾ�����ȡnr���¼���    ��  ���Ѿ���ɵ��ļ��첽I/O��   ��
��struct io  event "*events, struct       ������events����ĸ���һ������ͬ�ģ�  ���������ж�ȡ����              ��
��timespec *timeout)                      ��events��ִ����ɵ��¼����飻tlmeout ��                              ��
��         I "                            ���ǳ�ʱʱ�䣬Ҳ�����ڻ�ȡmm- nr��    ��                              ��
��                                        ���¼�ǰ�ĵȴ�ʱ��                    ��                              ��
�������������������������������������������ߩ������������������������������������ߩ�������������������������������
    ��9-7���оٵ���5�ַ����ṩ���ں˼�����ļ��첽I/O���ƣ�ʹ��ǰ��Ҫ�ȵ���io_setup������ʼ���첽I/O�����ġ���Ȼһ�����̿���ӵ��
����첽I/O�����ģ���ͨ����һ�����㹻�ˡ�����io_setup�������������첽I/O�����ĵ���������aio��context_t��
�ͣ��������������epoll_create���ص�������һ�����ǹᴩʼ�յġ�ע�⣬nr_ eventsֻ��ָ�����첽I/O���ٳ�ʼ��������������������û��
���������Դ�����첽I/O�¼���Ŀ��Ϊ�˱�����⣬������io_setup��epoll_create���жԱȣ����ǻ��Ǻ����Ƶġ�
    ��Ȼ��epoll���첽I/O���жԱȣ���ô��Щ�����൱��epoll_ctrl�أ�����io_submit��io_cancel������io_submit�൱�����첽I/O������¼���
��io_cancel���൱�ڴ��첽I/O���Ƴ��¼���io_submit���õ���һ���ṹ��iocb������򵥵ؿ�һ�����Ķ��塣
    struct iocb  f
    ��t�洢��ҵ����Ҫ��ָ�롣���磬��Nginx�У�����ֶ�ͨ���洢�Ŷ�Ӧ��ngx_event_tͤ����ָ
�롣��ʵ������io_getevents�����з��ص�io event�ṹ���data��Ա����ȫһ�µ�+��
    u int64 t aio data;
7 7����Ҫ����
u  int32��PADDED (aio_key,  aio_raservedl)j
���������룬��ȡֵ��Χ��io_iocb_cmd_t�е�ö������
u int16 t aio lio_opcode;
������������ȼ�
int16 t aio_reqprio,
����i41-������
u int32 t aio fildes;
��������д������Ӧ���û�̬������
u int64 t aio buf;
��������д�������ֽڳ���
u int64 t aio_nbytes;
��������д������Ӧ���ļ��е�ƫ����
int64 t aio offset;
7 7�����ֶ�
u int64��aio reserved2;
    ��+��ʾ��������ΪIOCB FLAG RESFD����������ں˵����첽I/O���������ʱʹ��eventfd��
��֪ͨ������epoll���ʹ��+��
    u int32��aio_flags��
������ʾ��ʹ��IOCB FLAG RESFD��־λʱ�����ڽ����¼�֪ͨ�ľ��
U int32 t aio resfd;
    ��ˣ������ú�iocb�ṹ��󣬾Ϳ������첽I/O�ύ�¼��ˡ�aio_lio_opcode������
ָ��������¼��Ĳ������ͣ�����ȡֵ��Χ���¡�
    typedef enum io_iocb_cmd_t{
    �����첽������
    IO_CMD_PREAD=O��
    �����첽д����
    IO_CMD_PWRITE=1��
    ����ǿ��ͬ��
    IO_CMD_FSYNC=2��
    ����Ŀǰ��ʹ��
    IO_CMD_FDSYNC=3��
    ����Ŀǰδʹ��
    IO_CMD_POLL=5��
    ���������κ�����
    IO_CMD_NOOP=6��
    )  io_iocb_cmd_t
    ��Nginx�У���ʹ����IO_CMD_PREAD���������ΪĿǰ��֧���ļ����첽I/O��ȡ����֧���첽I/O��д�롣������һ����Ҫ��ԭ�����ļ���
�첽I/O�޷����û��棬��д�ļ�����ͨ�����䵽�����еģ�Linux����ͳһ�������С��ࡱ����ˢ�µ����̵Ļ��ơ�
    ������ʹ��io submit���ں��ύ���ļ��첽I/O�������¼�����ʹ��io_cancel����Խ��Ѿ��ύ���¼�ȡ����
    ��λ�ȡ�Ѿ���ɵ��첽I/O�¼��أ�io_getevents�����������������൱��epoll�е�epoll_wait�����������õ���io_event�ṹ�壬���濴һ�����Ķ��塣
struct io event  {
    �������ύ�¼�ʱ��Ӧ��iocb�ṹ���е�aio_data��һ�µ�
    uint64 t  data;
    ����ָ���ύ�¼�ʱ��Ӧ��iocb�ṹ��
    uint64_t   obj��
    �����첽I/O����Ľṹ��res���ڻ����0ʱ��ʾ�ɹ���С��0ʱ��ʾʧ��
    int64һt    res��
    ��7��������
    int64һ��    res2 j
)��
    ���������ݻ�ȡ��io��event�ṹ�����飬�Ϳ��Ի���Ѿ���ɵ��첽I/O�����ˣ��ر���iocb�ṹ���е�aio data��Ա��io_event�е�data����
���ڴ���ָ�룬Ҳ����˵��ҵ���е����ݽṹ���¼���ɺ�Ļص������������
    �����˳�ʱ��Ҫ����io_destroy���������첽I/O�����ģ����൱�ڵ���close�ر�epoll����������
    Linux�ں��ṩ���ļ��첽I/O�����÷��ǳ��򵥣���������������ں���CPU��I/O�豸�Ǹ��Զ�����������һ���ԣ����ύ���첽I/O������
������ȫ����������������ֱ�����������鿴�첽I/O�����Ƿ���ɡ�
*/
//��ʼ���ļ��첽I/O��������
static int
io_setup(u_int nr_reqs, aio_context_t *ctx) //�൱��epoll�е�epoll_create
{
    return syscall(SYS_io_setup, nr_reqs, ctx);
}


static int
io_destroy(aio_context_t ctx)
{
    return syscall(SYS_io_destroy, ctx);
}

// ��λ�ȡ�Ѿ���ɵ��첽I/O�¼��أ�io_getevents�����������������൱��epoll�е�epoll_wait����
static int
io_getevents(aio_context_t ctx, long min_nr, long nr, struct io_event *events,
    struct timespec *tmo) //io_submitΪ����¼�
{
    return syscall(SYS_io_getevents, ctx, min_nr, nr, events, tmo);
}

/*
ngx_epoll_aio_init��ʼ��aio�¼��б� ngx_file_aio_read��Ӷ��ļ��¼�������ȡ��Ϻ�epoll�ᴥ��
ngx_epoll_eventfd_handler->ngx_file_aio_event_handler 
nginx file aioֻ�ṩ��read�ӿڣ����ṩwrite�ӿڣ���Ϊ�첽aioֻ�Ӵ��̶���д������aio��ʽһ��д���䵽
���̻��棬���Բ��ṩ�ýӿڣ�����첽ioд���ܻ����
*/

//aioʹ�ÿ��Բο�ngx_http_file_cache_aio_read  ngx_output_chain_copy_buf ����Ϊ��ȡ�ļ�׼����
static void
ngx_epoll_aio_init(ngx_cycle_t *cycle, ngx_epoll_conf_t *epcf)
{
    int                 n;
    struct epoll_event  ee;

/*
����eventfdϵͳ�����½�һ��eventfd���󣬵ڶ��������е�0��ʾeventfd�ļ�������ʼֵΪ0�� ϵͳ���óɹ����ص�����eventfd���������������
*/
#if (NGX_HAVE_SYS_EVENTFD_H)
    ngx_eventfd = eventfd(0, 0); //  
#else
    ngx_eventfd = syscall(SYS_eventfd, 0); //ʹ��Linux�е�323��ϵͳ���û�ȡһ�����������
#endif

    if (ngx_eventfd == -1) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                      "eventfd() failed");
        ngx_file_aio = 0;
        return;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                   "aio eventfd: %d", ngx_eventfd);

    n = 1;

    if (ioctl(ngx_eventfd, FIONBIO, &n) == -1) { //����ngx_eventfdΪ������
        ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                      "ioctl(eventfd, FIONBIO) failed");
        goto failed;
    }

    if (io_setup(epcf->aio_requests, &ngx_aio_ctx) == -1) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                      "io_setup() failed");
        goto failed;
    }

    ngx_eventfd_event.data = &ngx_eventfd_conn;//���������첽I/O���֪ͨ��ngx_eventfd_event�¼�������ngx_connection_t�����Ƕ�Ӧ��

    //�ú�����ngx_epoll_process_events��ִ��
    ngx_eventfd_event.handler = ngx_epoll_eventfd_handler; //���첽I/O�¼���ɺ�ʹ��ngx_epoll_eventfd handler��������
    ngx_eventfd_event.log = cycle->log;
    ngx_eventfd_event.active = 1;
    ngx_eventfd_conn.fd = ngx_eventfd;
    ngx_eventfd_conn.read = &ngx_eventfd_event; //ngx_eventfd_conn���ӵĶ��¼����������ngx_eventfd_event
    ngx_eventfd_conn.log = cycle->log;

    ee.events = EPOLLIN|EPOLLET;
    ee.data.ptr = &ngx_eventfd_conn;

    //��epoll����ӵ��첽I/O��֪ͨ������ngx_eventfd
    /*
    ������ngx_epoll_aio init��������첽I/O��epoll�����������ĳһ���첽I/O�¼���ɺ�ngx_eventfdѮ���ʹ��ڿ���״̬������epoll_wait��
    ����ngx_eventfd event�¼���ͻ�������Ļص�����ngx_epoll_eventfd handler�����Ѿ���ɵ��첽I/O�¼�
     */
    if (epoll_ctl(ep, EPOLL_CTL_ADD, ngx_eventfd, &ee) != -1) { //�ɹ���ֱ�ӷ���
        return;
    }

    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                  "epoll_ctl(EPOLL_CTL_ADD, eventfd) failed");

    //epoll_ctlʧ��ʱ�����aio��context   

    if (io_destroy(ngx_aio_ctx) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "io_destroy() failed");
    }

failed:

    if (close(ngx_eventfd) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "eventfd close() failed");
    }

    ngx_eventfd = -1;
    ngx_aio_ctx = 0;
    ngx_file_aio = 0;
}

#endif

static ngx_int_t
ngx_epoll_init(ngx_cycle_t *cycle, ngx_msec_t timer)
{
    ngx_epoll_conf_t  *epcf;

    epcf = ngx_event_get_conf(cycle->conf_ctx, ngx_epoll_module);

    if (ep == -1) {
       /*
        epoll_create����һ�������֮��epoll��ʹ�ö�����������������ʶ������size�Ǹ���epoll��Ҫ����Ĵ����¼���Ŀ������ʹ��epollʱ��
        �������close�ر���������ע��size����ֻ�Ǹ����ں����epoll����ᴦ����¼�������Ŀ���������ܹ�������¼�������������Linuxީ
        �µ�һЩ�ں˰汾��ʵ���У����size����û���κ����塣

        ����epoll_create���ں��д���epoll���������Ѿ�����������size��������ָ��epoll�ܹ����������¼���������Ϊ�����Linux�ں�
        �汾�У�epoll�ǲ�������������ģ�������Ϊcycle->connectionn/2��������cycle->connection_n��Ҳ��Ҫ��
        */
        ep = epoll_create(cycle->connection_n / 2);

        if (ep == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "epoll_create() failed");
            return NGX_ERROR;
        }

#if (NGX_HAVE_EVENTFD)
        if (ngx_epoll_notify_init(cycle->log) != NGX_OK) {
            ngx_epoll_module_ctx.actions.notify = NULL;
        }
#endif

#if (NGX_HAVE_FILE_AIO)

        ngx_epoll_aio_init(cycle, epcf);

#endif
    }

    if (nevents < epcf->events) {
        if (event_list) {
            ngx_free(event_list);
        }

        event_list = ngx_alloc(sizeof(struct epoll_event) * epcf->events,
                               cycle->log);
        if (event_list == NULL) {
            return NGX_ERROR;
        }
    }

    nevents = epcf->events;//nerentsҲ��������epoll_events�Ĳ���

    ngx_io = ngx_os_io;

    ngx_event_actions = ngx_epoll_module_ctx.actions;

#if (NGX_HAVE_CLEAR_EVENT)
    //Ĭ���ǲ���LTģʽ��ʹ��epoll�ģ�NGX USE CLEAR EVENT��ʵ���Ͼ����ڸ���Nginxʹ��ETģʽ
    ngx_event_flags = NGX_USE_CLEAR_EVENT
#else
    ngx_event_flags = NGX_USE_LEVEL_EVENT
#endif
                      |NGX_USE_GREEDY_EVENT
                      |NGX_USE_EPOLL_EVENT;

    return NGX_OK;
}


#if (NGX_HAVE_EVENTFD)
//ִ��ngx_epoll_notify���ͨ��epoll_wait����ִ�иú���ngx_epoll_notify_handler
//ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�
static ngx_int_t
ngx_epoll_notify_init(ngx_log_t *log)
{
    struct epoll_event  ee;

#if (NGX_HAVE_SYS_EVENTFD_H)
    notify_fd = eventfd(0, 0);
#else
    notify_fd = syscall(SYS_eventfd, 0);
#endif

    if (notify_fd == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "eventfd() failed");
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, log, 0,
                   "notify eventfd: %d", notify_fd);

    notify_event.handler = ngx_epoll_notify_handler;
    notify_event.log = log;
    notify_event.active = 1;

    notify_conn.fd = notify_fd;
    notify_conn.read = &notify_event;
    notify_conn.log = log;

    ee.events = EPOLLIN|EPOLLET;
    ee.data.ptr = &notify_conn;

    if (epoll_ctl(ep, EPOLL_CTL_ADD, notify_fd, &ee) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "epoll_ctl(EPOLL_CTL_ADD, eventfd) failed");

        if (close(notify_fd) == -1) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                            "eventfd close() failed");
        }

        return NGX_ERROR;
    }

    return NGX_OK;
}

//ִ��ngx_epoll_notify���ͨ��epoll_wait����ִ�иú���ngx_epoll_notify_handler
//ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�

static void //ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�
ngx_epoll_notify_handler(ngx_event_t *ev)
{
    ssize_t               n;
    uint64_t              count;
    ngx_err_t             err;
    ngx_event_handler_pt  handler;

    if (++ev->index == NGX_MAX_UINT32_VALUE) {
        ev->index = 0;

        n = read(notify_fd, &count, sizeof(uint64_t));

        err = ngx_errno;

        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "read() eventfd %d: %z count:%uL", notify_fd, n, count);

        if ((size_t) n != sizeof(uint64_t)) {
            ngx_log_error(NGX_LOG_ALERT, ev->log, err,
                          "read() eventfd %d failed", notify_fd);
        }
    }

    handler = ev->data;
    handler(ev);
}

#endif

/*
ngx_event_actions_t done�ӿ�����ngx_epoll_done����ʵ�ֵģ���Nginx�˳�����ʱ����õ����á�ngx_epoll_done��Ҫ�ǹر�epoll������ep��
ͬʱ�ͷ�event_1ist���顣
*/
static void
ngx_epoll_done(ngx_cycle_t *cycle)
{
    if (close(ep) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "epoll close() failed");
    }

    ep = -1;

#if (NGX_HAVE_EVENTFD)

    if (close(notify_fd) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "eventfd close() failed");
    }

    notify_fd = -1;

#endif

#if (NGX_HAVE_FILE_AIO)

    if (ngx_eventfd != -1) {

        if (io_destroy(ngx_aio_ctx) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "io_destroy() failed");
        }

        if (close(ngx_eventfd) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "eventfd close() failed");
        }

        ngx_eventfd = -1;
    }

    ngx_aio_ctx = 0;

#endif

    ngx_free(event_list);

    event_list = NULL;
    nevents = 0;
}

/*
epoll_ctlϵͳ����
    epoll_ctl��C���е�ԭ�����¡�
    int epoll_ctl(int  epfd, int  op, int  fd, struct  epoll_event*  event)j
    epoll_ctl��epoll��������ӡ��޸Ļ���ɾ������Ȥ���¼�������0��ʾ�ɹ�������
��һ1����ʱ��Ҫ����errno�������жϴ������͡�epoll_wait�������ص��¼���Ȼ��ͨ��
epoll_ctl��ӹ�epoll�еġ�����epfd��epoll_create���صľ������op�������������
9-2��
�����������������������ש���������������������������
��    ��    op��ȡֵ  ��    ����                  ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL_ADD     ��    ����µ��¼���epoll�� ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL MOD     ��    �޸�epoll�е��¼�     ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL_DEL     ��    ɾ��epoll�е��¼�     ��
�����������������������ߩ���������������������������
    ��3������fd�Ǵ����������׽��֣���4���������ڸ���epoll��ʲô�����¼���
��Ȥ����ʹ����epoll_event�ṹ�壬�����Ľ��ܹ���epollʵ�ֻ����л�Ϊÿһ���¼���
��epitem�ṹ�壬����epitem����һ��epoll_event���͵�event��Ա�����濴һ��epoll_
event�Ķ��塣
struct epoll_event{
        _uint32 t events;
        epoll data_t data;
};
events��ȡֵ����9-3��
��9-3 epoll_event��events��ȡֵ����
�������������������ש�������������������������������������������������������������������������������
��    eventsȡֵ  ��    ����                                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLIN         ��                                                                              ��
��                ��  ��ʾ��Ӧ�������������ݿ��Զ�����TCP���ӵ�Զ�������ر����ӣ�Ҳ�൱�ڿɶ���   ��
��                ��������Ϊ��Ҫ����������FIN����                                               ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLOUT        ��  ��ʾ��Ӧ�������Ͽ���д�����ݷ��ͣ����������η����������������TCP���ӣ����� ��
��                �������ɹ����¼��൱�ڿ�д�¼���                                                ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLRDHUP      ��  ��ʾTCP���ӵ�Զ�˹رջ��ر�����                                           ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLPRI        ��  ��ʾ��Ӧ���������н���������Ҫ��                                            ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLERR        ��  ��ʾ��Ӧ�����ӷ�������                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLHUP        ��  ��ʾ��Ӧ�����ӱ�����                                                        ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLET         ��  ��ʾ��������ʽ��������Ե����(ET)��ϵͳĬ��Ϊˮƽ����(LT��)                  ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLONESHOT    ��  ��ʾ������¼�ֻ����һ�Σ��´���Ҫ����ʱ�����¼���epoll                     ��
�������������������ߩ�������������������������������������������������������������������������������
��data��Ա��һ��epoll_data���ϣ��䶨�����¡�
typedef union epoll_data
       void        *ptr;
      int           fd;
     uint32_t            u32 ;
     uint64_t            u64 ;
} epoll_data_t;
    �ɼ������data��Ա��������ʹ�÷�ʽ��ء����磬ngx_epoll_moduleģ��ֻʹ����
�����е�ptr��Ա����Ϊָ��ngx_connection_t���ӵ�ָ�롣
*/ 
//ngx_epoll_add_event��ʾ���ĳ�����͵�(������д��ͨ��flagָ���ٷ���ʽ��NGX_CLEAR_EVENTΪET��ʽ��NGX_LEVEL_EVENTΪLT��ʽ)�¼���
//ngx_epoll_add_connection(��дһ�������ȥ, ʹ��EPOLLET���ش�����ʽ)
static ngx_int_t      //ͨ��flagָ���ٷ���ʽ��NGX_CLEAR_EVENTΪET��ʽ��NGX_LEVEL_EVENTΪLT��ʽ
ngx_epoll_add_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags) //�ú�����װΪngx_add_event�ģ�ʹ�õ�ʱ��Ϊngx_add_event
{ //һ�������¼��еı��Ķ�дͨ��ngx_handle_read_event  ngx_handle_write_event����¼�
    int                  op;
    uint32_t             events, prev;
    ngx_event_t         *e;
    ngx_connection_t    *c;
    struct epoll_event   ee;

    c = ev->data; //ÿ���¼���data��Ա����������Ӧ��ngx_connection_t����

    /*
    ��������event����ȷ����ǰ�¼��Ƕ��¼�����д�¼���������eventg�Ǽ���EPOLLIN��־λ����EPOLLOUT��־λ
     */
    events = (uint32_t) event;

    if (event == NGX_READ_EVENT) {
        e = c->write;
        prev = EPOLLOUT;
#if (NGX_READ_EVENT != EPOLLIN|EPOLLRDHUP)
        events = EPOLLIN|EPOLLRDHUP;
#endif

    } else {
        e = c->read;
        prev = EPOLLIN|EPOLLRDHUP;
#if (NGX_WRITE_EVENT != EPOLLOUT)
        events = EPOLLOUT;
#endif
    }

    //��һ�����epoll_ctlΪEPOLL_CTL_ADD,����ٴ���ӷ���activeΪ1,��epoll_ctlΪEPOLL_CTL_MOD
    if (e->active) { //����active��־λȷ���Ƿ�Ϊ��Ծ�¼����Ծ����������޸Ļ�������¼�
        op = EPOLL_CTL_MOD; 
        events |= prev; //�����active�ģ���events= EPOLLIN|EPOLLRDHUP|EPOLLOUT;

    } else {
        op = EPOLL_CTL_ADD;
    }

    ee.events = events | (uint32_t) flags; //����flags������events��־λ��
    /* ptr��Ա�洢����ngx_connection_t���ӣ��ɲμ�epoll��ʹ�÷�ʽ��*/
    ee.data.ptr = (void *) ((uintptr_t) c | ev->instance);

    if (e->active) {//modify
        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "epoll modify read and write event: fd:%d op:%d ev:%08XD", c->fd, op, ee.events);
    } else {//add
        if (event == NGX_READ_EVENT) {
            ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "epoll add read event: fd:%d op:%d ev:%08XD", c->fd, op, ee.events);
        } else
            ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "epoll add write event: fd:%d op:%d ev:%08XD", c->fd, op, ee.events);
    }
  
    //EPOLL_CTL_ADDһ�κ󣬾Ϳ���һֱͨ��epoll_wait����ȡ���¼������ǵ���EPOLL_CTL_DEL������ÿ�ζ��¼�����epoll_wait���غ�Ҫ�������EPOLL_CTL_ADD��
    //֮ǰ�������еĵط�����ע���ˣ���עΪÿ�ζ��¼�������Ҫ����addһ��
    if (epoll_ctl(ep, op, c->fd, &ee) == -1) {//epoll_wait() ϵͳ���õȴ����ļ������� c->fd ���õ� epoll ʵ���ϵ��¼�
        ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                      "epoll_ctl(%d, %d) failed", op, c->fd);
        return NGX_ERROR;
    }
    //�����ngx_add_event->ngx_epoll_add_event�а�listening�е�c->read->active��1�� ngx_epoll_del_event�а�listening����read->active��0
    //��һ�����epoll_ctlΪEPOLL_CTL_ADD,����ٴ���ӷ���activeΪ1,��epoll_ctlΪEPOLL_CTL_MOD
    ev->active = 1; //���¼���active��־λ��Ϊ1����ʾ��ǰ�¼��ǻ�Ծ��   ngx_epoll_del_event����0
#if 0
    ev->oneshot = (flags & NGX_ONESHOT_EVENT) ? 1 : 0;
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_epoll_del_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags)
{
    int                  op;
    uint32_t             prev;
    ngx_event_t         *e;
    ngx_connection_t    *c;
    struct epoll_event   ee;

    /*
     * when the file descriptor is closed, the epoll automatically deletes
     * it from its queue, so we do not need to delete explicitly the event
     * before the closing the file descriptor
     */

    if (flags & NGX_CLOSE_EVENT) {
        ev->active = 0;
        return NGX_OK;
    }

    c = ev->data;

    if (event == NGX_READ_EVENT) {
        e = c->write;
        prev = EPOLLOUT;

    } else {
        e = c->read;
        prev = EPOLLIN|EPOLLRDHUP;
    }

    if (e->active) {
        op = EPOLL_CTL_MOD;
        ee.events = prev | (uint32_t) flags;
        ee.data.ptr = (void *) ((uintptr_t) c | ev->instance);

    } else {
        op = EPOLL_CTL_DEL;
        ee.events = 0;
        ee.data.ptr = NULL;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "epoll del event: fd:%d op:%d ev:%08XD",
                   c->fd, op, ee.events);

    if (epoll_ctl(ep, op, c->fd, &ee) == -1) {
        ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                      "epoll_ctl(%d, %d) failed", op, c->fd);
        return NGX_ERROR;
    }
    //�����ngx_add_event->ngx_epoll_add_event�а�listening�е�c->read->active��1�� ngx_epoll_del_event�а�listening����read->active��0
    ev->active = 0;

    return NGX_OK;
}

//ngx_epoll_add_event��ʾ���ĳ�����͵�(������д��ʹ��LT�ٷ���ʽ)�¼���ngx_epoll_add_connection (��дһ�������ȥ, ʹ��EPOLLET���ش�����ʽ)
static ngx_int_t
ngx_epoll_add_connection(ngx_connection_t *c) //�ú�����װΪngx_add_conn�ģ�ʹ�õ�ʱ��Ϊngx_add_conn
{
    struct epoll_event  ee;

    ee.events = EPOLLIN|EPOLLOUT|EPOLLET|EPOLLRDHUP; //ע��������ˮƽ���� 
    ee.data.ptr = (void *) ((uintptr_t) c | c->read->instance);

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "epoll add connection(read and write): fd:%d ev:%08XD", c->fd, ee.events);

    if (epoll_ctl(ep, EPOLL_CTL_ADD, c->fd, &ee) == -1) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      "epoll_ctl(EPOLL_CTL_ADD, %d) failed", c->fd);
        return NGX_ERROR;
    }

    c->read->active = 1;
    c->write->active = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_epoll_del_connection(ngx_connection_t *c, ngx_uint_t flags)
{
    int                 op;
    struct epoll_event  ee;

    /*
     * when the file descriptor is closed the epoll automatically deletes
     * it from its queue so we do not need to delete explicitly the event
     * before the closing the file descriptor
     */

    if (flags & NGX_CLOSE_EVENT) { //����ú��������Ż����close(fd)����ô�Ͳ���epoll del�ˣ�ϵͳ���Զ�del
        c->read->active = 0;
        c->write->active = 0;
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "epoll del connection: fd:%d", c->fd);

    op = EPOLL_CTL_DEL;
    ee.events = 0;
    ee.data.ptr = NULL;

    if (epoll_ctl(ep, op, c->fd, &ee) == -1) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      "epoll_ctl(%d, %d) failed", op, c->fd);
        return NGX_ERROR;
    }

    c->read->active = 0;
    c->write->active = 0;

    return NGX_OK;
}


#if (NGX_HAVE_EVENTFD)
//ִ��ngx_epoll_notify���ͨ��epoll_wait����ִ�иú���ngx_epoll_notify_handler
//ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�

static ngx_int_t
ngx_epoll_notify(ngx_event_handler_pt handler)
{
    static uint64_t inc = 1;

    notify_event.data = handler;

    //ngx_notify->ngx_epoll_notifyֻ�ᴥ��epool_in������ͬʱ����epoll_out�������������¼�epoll_in,���ͬʱ����epoll_out
    if ((size_t) write(notify_fd, &inc, sizeof(uint64_t)) != sizeof(uint64_t)) {
        ngx_log_error(NGX_LOG_ALERT, notify_event.log, ngx_errno,
                      "write() to eventfd %d failed", notify_fd);
        return NGX_ERROR;
    }

    return NGX_OK;
}

#endif

void ngx_epoll_event_2str(uint32_t event, char* buf)
{
    if(event & EPOLLIN)
        strcpy(buf, "EPOLLIN ");

    if(event & EPOLLPRI) 
        strcat(buf, "EPOLLPRI ");
  
    if(event & EPOLLOUT)
        strcat(buf, "EPOLLOUT ");

    if(event & EPOLLRDNORM)
        strcat(buf, "EPOLLRDNORM ");

    if(event & EPOLLRDBAND)
        strcat(buf, "EPOLLRDBAND ");

    if(event & EPOLLWRNORM)
        strcat(buf, "EPOLLWRNORM ");

    if(event & EPOLLWRBAND)
        strcat(buf, "EPOLLWRBAND ");

    if(event & EPOLLMSG) 
        strcat(buf, "EPOLLMSG ");

    if(event & EPOLLERR)
        strcat(buf, "EPOLLERR ");
        
    if(event & EPOLLHUP)
        strcat(buf, "EPOLLHUP ");
        
    strcat(buf, " ");
}

//ngx_epoll_process_eventsע�ᵽngx_process_events��  
//��ngx_epoll_add_event���ʹ��
//�ú�����ngx_process_events_and_timers�е���
static ngx_int_t
ngx_epoll_process_events(ngx_cycle_t *cycle, ngx_msec_t timer, ngx_uint_t flags)//flags�����к���NGX_POST_EVENTS��ʾ�����¼�Ҫ�Ӻ���
{
    int                events;
    uint32_t           revents;
    ngx_int_t          instance, i;
    ngx_uint_t         level;
    ngx_err_t          err;
    ngx_event_t       *rev, *wev;
    ngx_queue_t       *queue;
    ngx_connection_t  *c;
    char epollbuf[256];
    
    /* NGX_TIMER_INFINITE == INFTIM */

    //ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "begin to epoll_wait, epoll timer: %M ", timer);

    /*
     ����epoll_wait��ȡ�¼���ע�⣬timer��������process_events����ʱ����ģ���9.7��9.8���л��ᵽ�������
     */
    //The call was interrupted by a signal handler before any of the requested events occurred or the timeout expired;
    //������źŷ���(������ngx_timer_signal_handler)���綨ʱ������᷵��-1
    //��Ҫ��ngx_add_event��ngx_add_conn���ʹ��        
    //event_list�洢���Ǿ����õ��¼��������select���Ǵ����û�ע����¼�����Ҫ������飬����ÿ��select���غ���Ҫ���������¼�����epoll����
    /*
    ������ȴ����¼������ͻ��������¼�(����ǴӸ����̼̳й�����ep��Ȼ�����ӽ���whileǰ��ngx_event_process_init->ngx_add_event���)��
    ���Ѿ��������ӵ�fd��д�¼��������ngx_event_accept->ngx_http_init_connection->ngx_handle_read_event
    */

/*
ngx_notify->ngx_epoll_notifyֻ�ᴥ��epoll_in������ͬʱ����epoll_out�������������¼�epoll_in,���ͬʱ����epoll_out
*/
    events = epoll_wait(ep, event_list, (int) nevents, timer);  //timerΪ-1��ʾ���޵ȴ�   nevents��ʾ���������ٸ��¼����������0
    //EPOLL_WAIT���û�ж�д�¼����߶�ʱ����ʱ�¼�������������˯�ߣ�������̻��ó�CPU

    err = (events == -1) ? ngx_errno : 0;

    //��flags��־λָʾҪ����ʱ��ʱ��������������µ�
    //Ҫ��ngx_timer_resolution���볬ʱ�����ʱ�䣬Ҫ��epoll��д�¼���ʱ�����ʱ��
    if (flags & NGX_UPDATE_TIME || ngx_event_timer_alarm) {
        ngx_time_update();
    }

    if (err) {
        if (err == NGX_EINTR) {

            if (ngx_event_timer_alarm) { //��ʱ����ʱ�����epoll_wait����
                ngx_event_timer_alarm = 0;
                return NGX_OK;
            }

            level = NGX_LOG_INFO;

        } else {
            level = NGX_LOG_ALERT;
        }

        ngx_log_error(level, cycle->log, err, "epoll_wait() failed");
        return NGX_ERROR;
    }

    if (events == 0) {
        if (timer != NGX_TIMER_INFINITE) {
            return NGX_OK;
        }

        ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                      "epoll_wait() returned no events without timeout");
        return NGX_ERROR;
    }

    //��������epoll_wait���ص������¼�
    for (i = 0; i < events; i++) { //��ngx_epoll_add_event���ʹ��
        /*
        �����������ᵽ��ngx_epoll_add_event���������Կ���ptr��Ա����ngx_connection_t���ӵĵ�ַ�������1λ�����⺬�壬��Ҫ�������ε�
          */
        c = event_list[i].data.ptr; //ͨ�����ȷ�����Ǹ�����

        instance = (uintptr_t) c & 1; //����ַ�����һλȡ��������instance������ʶ, ��ngx_epoll_add_event

        /*
          ������32λ����64λ���������ַ�����1λ�϶���0��������������������ngx_connection_t�ĵ�ַ��ԭ�������ĵ�ֵַ
          */ //ע�������c�п�����acceptǰ��c�����ڼ���Ƿ�ͻ��˷���tcp�����¼�,accept���سɹ�������´���һ��ngx_connection_t��������д�ͻ��˵�����
        c = (ngx_connection_t *) ((uintptr_t) c & (uintptr_t) ~1);

        rev = c->read; //ȡ�����¼� //ע�������c�п�����acceptǰ��c�����ڼ���Ƿ�ͻ��˷���tcp�����¼�,accept���سɹ�������´���һ��ngx_connection_t��������д�ͻ��˵�����

        if (c->fd == -1 || rev->instance != instance) { //�ж�������¼��Ƿ�Ϊ�����¼�
          //��fd�׽���������Ϊ-l����instance��־λ�����ʱ����ʾ����¼��Ѿ������ˣ����ô���
            /*
             * the stale event from a file descriptor
             * that was just closed in this iteration
             */

            ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                           "epoll: stale event %p", c);
            continue;
        }

        revents = event_list[i].events; //ȡ���¼�����
        ngx_epoll_event_2str(revents, epollbuf);

        memset(epollbuf, 0, sizeof(epollbuf));
        ngx_epoll_event_2str(revents, epollbuf);
        ngx_log_debug4(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "epoll: fd:%d %s(ev:%04XD) d:%p",
                       c->fd, epollbuf, revents, event_list[i].data.ptr);

        if (revents & (EPOLLERR|EPOLLHUP)) { //����Է�close���׽��֣�������Ӧ��
            ngx_log_debug2(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                           "epoll_wait() error on fd:%d ev:%04XD",
                           c->fd, revents);
        }

#if 0
        if (revents & ~(EPOLLIN|EPOLLOUT|EPOLLERR|EPOLLHUP)) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                          "strange epoll_wait() events fd:%d ev:%04XD",
                          c->fd, revents);
        }
#endif

        if ((revents & (EPOLLERR|EPOLLHUP)) 
             && (revents & (EPOLLIN|EPOLLOUT)) == 0)
        {
            /*
             * if the error events were returned without EPOLLIN or EPOLLOUT,
             * then add these flags to handle the events at least in one
             * active handler
             */

            revents |= EPOLLIN|EPOLLOUT; //epoll EPOLLERR|EPOLLHUPʵ������ͨ��������д�¼����ж�д����recv write����������쳣
        }

        if ((revents & EPOLLIN) && rev->active) { //����Ƕ��¼��Ҹ��¼��ǻ�Ծ��

#if (NGX_HAVE_EPOLLRDHUP)
            if (revents & EPOLLRDHUP) {
                rev->pending_eof = 1;
            }
#endif
            //ע�������c�п�����acceptǰ��c�����ڼ���Ƿ�ͻ��˷���tcp�����¼�,accept���سɹ�������´���һ��ngx_connection_t��������д�ͻ��˵�����
            rev->ready = 1; //��ʾ�Ѿ������ݵ�������ֻ�ǰ�accept�ɹ�ǰ�� ngx_connection_t->read->ready��1��accept���غ�����´����ӳ��л�ȡһ��ngx_connection_t
            //flags�����к���NGX_POST_EVENTS��ʾ�����¼�Ҫ�Ӻ���
            if (flags & NGX_POST_EVENTS) {
                /*
                ���Ҫ��post�������Ӻ�����¼�������Ҫ�ж������������¼�������ͨ�¼����Ծ�����������
                ��ngx_posted_accept_events���л���ngx_postedL events�����С�����post�����е��¼���ʱִ��
                */
                queue = rev->accept ? &ngx_posted_accept_events
                                    : &ngx_posted_events;

                ngx_post_event(rev, queue); 

            } else {
                //������յ��ͻ������ݣ�����Ϊngx_http_wait_request_handler  
                rev->handler(rev); //���Ϊ��ûaccept����Ϊngx_event_process_init������Ϊngx_event_accept������Ѿ��������ӣ��������Ϊngx_http_process_request_line
            }
        }

        wev = c->write;

        if ((revents & EPOLLOUT) && wev->active) {

            if (c->fd == -1 || wev->instance != instance) { //�ж�������¼��Ƿ�Ϊ�����¼�
                //��fd�׽���������Ϊ-1����instance��־λ�����ʱ����ʾ����¼��Ѿ����ڣ����ô���
                /*
                 *  the stale event from a file descriptor
                 * that was just closed in this iteration
                 */

                ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                               "epoll: stale event %p", c);
                continue;
            }

            wev->ready = 1;

            if (flags & NGX_POST_EVENTS) { 
                ngx_post_event(wev, &ngx_posted_events); //������¼���ӵ�post�������Ӻ���

            } else { //�����������д�¼��Ļص���������������¼�
                wev->handler(wev);
            }
        }
    }

    return NGX_OK;
}


#if (NGX_HAVE_FILE_AIO)
/*
�첽�ļ�i/o�����¼��Ļص�����Ϊngx_file_aio_event_handler�����ĵ��ù�ϵ����������epoll_wait(ngx_process_events_and_timers)�е���
ngx_epoll_eventfd_handler��������ǰ�¼����뵽ngx_posted_events�����У����Ӻ�ִ�еĶ����е���ngx_file_aio_event_handler����
*/
/*
 ���������¼����������ƾ�������ͨ��ngx_eventfd֪ͨ��������ngx_epoll_eventfd
handler�ص������������ļ��첽I/O�¼���������ġ�
    ��ô���������첽I/O���������ύ�첽I/O�����أ�����ngx_linux_aio read.c�ļ���
��ngx_file_aio_read�������ڴ��ļ��첽I/O������������Ḻ������ļ��Ķ�ȡ
*/
/*
ngx_epoll_aio_init��ʼ��aio�¼��б� ngx_file_aio_read��Ӷ��ļ��¼�������ȡ��Ϻ�epoll�ᴥ��
ngx_epoll_eventfd_handler->ngx_file_aio_event_handler 
nginx file aioֻ�ṩ��read�ӿڣ����ṩwrite�ӿڣ���Ϊ�첽aioֻ�Ӵ��̶���д������aio��ʽһ��д���䵽
���̻��棬���Բ��ṩ�ýӿڣ�����첽ioд���ܻ����
*/

//�ú�����ngx_process_events_and_timers��ִ��
static void
ngx_epoll_eventfd_handler(ngx_event_t *ev) //��epoll_wait�м�⵽aio���ɹ��¼������ߵ�����
{
    int               n, events;
    long              i;
    uint64_t          ready;
    ngx_err_t         err;
    ngx_event_t      *e;
    ngx_event_aio_t  *aio;
    struct io_event   event[64]; //һ������ദ��64���¼�
    struct timespec   ts;

    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ev->log, 0, "eventfd handler");
    //��ȡ�Ѿ���ɵ��¼���Ŀ�������õ�ready�У�ע�⣬���ready�ǿ��Դ���64��
    n = read(ngx_eventfd, &ready, 8);//����read������ȡ����ɵ�I/O�ĸ���   

    err = ngx_errno;

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, ev->log, 0, "aio epoll handler eventfd: %d", n);

    if (n != 8) {
        if (n == -1) {
            if (err == NGX_EAGAIN) {
                return;
            }

            ngx_log_error(NGX_LOG_ALERT, ev->log, err, "read(eventfd) failed");
            return;
        }

        ngx_log_error(NGX_LOG_ALERT, ev->log, 0,
                      "read(eventfd) returned only %d bytes", n);
        return;
    }

    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    //ready��ʾ��δ������¼�����ready����0ʱ��������
    while (ready) {
        //����io_getevents��ȡ�Ѿ���ɵ��첽I/O�¼�
        events = io_getevents(ngx_aio_ctx, 1, 64, event, &ts);

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "io_getevents: %l", events);

        if (events > 0) {
            ready -= events; //��ready��ȥ�Ѿ�ȡ�����¼�

            for (i = 0; i < events; i++) { //����event��������¼�

                ngx_log_debug4(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                               "io_event: %uXL %uXL %L %L",
                                event[i].data, event[i].obj,
                                event[i].res, event[i].res2);
                //data��Աָ������첽I/O�¼���Ӧ�ŵ�ʵ���¼�,�����ngx_event_tΪngx_event_aio_t->event
               /*��ngx_epoll_eventfd_handler��Ӧ,��ִ��ngx_file_aio_read���첽I/O����Ӷ��¼���Ȼ��ͨ��epoll�������ض�ȡ
               ���ݳɹ�����ִ��ngx_epoll_eventfd_handler*/
               //�����e����ngx_file_aio_read����ӽ����ģ��ں����ngx_post_event�л�ִ��ngx_file_aio_event_handler
                e = (ngx_event_t *) (uintptr_t) event[i].data; //��ngx_file_aio_read��io_submit��Ӧ

                e->complete = 1;
                e->active = 0;
                e->ready = 1;

                aio = e->data;
                aio->res = event[i].res;

                ngx_post_event(e, &ngx_posted_events); 
                //�����¼��ŵ�ngx_posted_events�������Ӻ�ִ��,ִ�иö��е�handle�ط���ngx_process_events_and_timers
            }

            continue;
        }

        if (events == 0) {
            return;
        }

        /* events == -1 */
        ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                      "io_getevents() failed");
        return;
    }
}

#endif


static void *
ngx_epoll_create_conf(ngx_cycle_t *cycle)
{
    ngx_epoll_conf_t  *epcf;

    epcf = ngx_palloc(cycle->pool, sizeof(ngx_epoll_conf_t));
    if (epcf == NULL) {
        return NULL;
    }

    epcf->events = NGX_CONF_UNSET;
    epcf->aio_requests = NGX_CONF_UNSET;

    return epcf;
}


static char *
ngx_epoll_init_conf(ngx_cycle_t *cycle, void *conf)
{
    ngx_epoll_conf_t *epcf = conf;

    ngx_conf_init_uint_value(epcf->events, 512);
    ngx_conf_init_uint_value(epcf->aio_requests, 32);

    return NGX_CONF_OK;
}
