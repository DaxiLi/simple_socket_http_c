/*********
 * email : moogila@outlook.com
 * Last Edit At : 2021-10-9 18:49
 * Last Edit By : J1s
 *
 * socket 编程实现 HTTP WEB 服务器
 *
 * 该程序的目标是,使用 c 语言, 减少文件数量,
 * 老实说,我应该使用 全局变量 但是,
 * 由于初期没有思考好, 我写了好几个类,好几个方法,结构体,
 * 过分设计
 * 后期,决定写一个项目结构尽量简单的 http simple 服务器,最好只有一个文件
 * 所以我,删了以前的代码(一堆 c++ 类),重新开始!
 *
 * 再后期,发现时间不够,决定写一个,功能也尽量简单的 http simple 服务器
 * 这使得代码像是拼凑上去的一样,不够干练简洁.
 * 好吧, 如果有时间的话,我应该重构一下它
 *
 * 可能是 years later
 *
 * 发现 c 自己管理所有内存,自己处理所有情况,并没有那么糟糕
 * 只是以前过于痴迷 c++ 的 stl 与一堆拿来即用的类
 *
 * enjoy!
 *
 * DOC : 应至少实现一下功能
 * 1.支持 GET 请求静态资源 访问服务器资源
 *      其他请求 和
 *      CGI 视情况而定,就目前情况而言,时间不足,无法实现
 * 2.多线程处理请求
 * 3.能够对浏览器发送的所有请求进行正确的回应
 * 4.使用 c 语言
 *
 *
 * 代码很烂
 * ****************** TODO LIST
 * 优化代码!
 *
 *
 *
 *
 * 但是,不得不说,
 * IT WORKS,That Is Enough!!!
 *
 *
 * =============================== REFACT
 *
 * 今天,我又对代码进行了重构,将局部变量尽可能使用全局变量
 * 考虑到 fork 时可能更高效
 * 其次,避免调用时,传参复制,虽然很多地方我是用了 inline 关键字
 *
 * 可以让项目结构整体有结构,但是不利于移植,和单独函数的理解
 * 对于代码规范整洁之道,我一直很疑惑
 * 到底怎样的代码才算是好代码
 *
 * Oh,世界上,没有好代码!!!
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <sys/epoll.h>


#include "log.h"
#include "log.c"

#define  ENABLE_DEBUG 1

#define checkError(ret) do{if(-1==ret){printf("[%d]err:%s\n", __LINE__, strerror(errno));exit(1);}}while(0)




#if ENABLE_DEBUG == 1
#define DEBUG(msg, ...) log_debug(msg)
#else
#define DEBUG(msg,...) ""
#endif
#ifndef DEFAULT_PORT
#define DEFAULT_PORT 8085
#endif
#ifndef DEFAULT_ROOT
#define DEFAULT_ROOT "./"
#endif
#ifndef DEFAULT_MAX_CLIENT
#define DEFAULT_MAX_CLIENT 256
#endif
#ifndef DEFAULT_IP
#define DEFAULT_IP "0.0.0.0"
#endif
#ifndef DAFAULT_RUN
#define DEFAULT_RUN 1
#endif
#ifndef MAX_CONNECT
#define MAX_CONNECT 256
#endif
#ifndef DEFAULT_TIMEOUT
#define DEFAULT_TIMEOUT 15
#endif


struct config {
    char *root;
    int max_client;
    unsigned int port;
    char *ip;
    int run;
    int timeout;
} serve_config;


struct run_time {
    unsigned int connected;

} runtime;

// 这一部分开始有点向 c++ 发展
// 好像没有用到
/**
 * http 请求的 头部信息
 * http 报文包括 通用首部、请求首部、响应首部、实体首部、扩展首部
 * 此处应包含大部分的通用首部、请求首部、实体首部
 */
struct HTTP_MSG {
    char *protocol;
    char *method;
    char *path;
    char *status_code;
    char *status_msg;
    // 通用首部
    char *date;             // 报文创建日期
    char *connection;       //允许客户端你和服务器指定与请求/响应有关选项
    char *mime_version;     // 发送端使用的 mime 版本
    char *transfer_encoding;//编码方式
    char *update;           // 发送端想要升级的新版本或协议
    char *via;              // 报文中间节点（代理、网关）
    // 请求首部
    char *client_ip;        // 客户端机器 ip
    char *from;             // 客户端用户的 Email 地址
    char *host;             // 接受请求的服务器、主机名
    char *referer;          // ？？包含当前请求的 URI 的文档的 URL （注：这个意思应该是说请求来源）
    char *ua_os;            //
    char *ua_pixels;
    char *user_agent;
    char *alive;

    char *accept;           // 能发送的媒体类型
    char *accept_encoding;
    char *accept_charset;

    // 太多了 ,本服务器不予支持,不写了
    // ================ 太多了，用不到，考虑删除一些请求头

    // 实体首部
    char *location;
    char *allow;            // 可接受的请求方法

    // 实体____内容首部
    char *content_type;     // 主体对象类型
    char *content_length;   // 主体长度
    char *content_md5;      // 主体 md5 校验
    char *range;            // 主体中此实体表示的字节范围
    char *eTag;
    char *expires;          // 过期重新请求的时间
    char *last_modified;    // 上次修改时间
    char *content;          // 载荷
}http_msg;

int init(int argc, char **argv);    // 初始化参数
int startup() ;                     //开始
void http_serve(int serve_socket);  // 处理 http 请求
static void GC_child(int iSig);     // 回收子线程
inline static void GET(int serve_socket);


char *line_buf[1024];           // 从 socket 读取一行缓冲
char *next_ele = 0;
int client_fd = -1;



/**
 * 安全的申请内存
 * @param size
 * @return
 */
static char *safe_malloc(size_t size) {
    char *p;
    p = (char *) malloc(size);
    if (p == 0) {
        log_error("malloc error!\n");
        exit(1);
    }
    return p;
}

/**
 * 将所给的字符传 ， 安全的复制下来
 * @param str
 * @return
 */
static char *str_dup(char *str) {
    if (str == 0) return 0;
    size_t size = strlen(str) + 1;
    char *ret_val = safe_malloc(size);
    strcpy(ret_val, str);
    return ret_val;
}

int start_listen() {
    int res = -1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd) {
        log_error("can not creat SOCKET!\n");
        return -1;
    }
    log_info("creat socket!\n");
    int reuse = 1;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) {
        log_error("socketopt error!\n");
        return -1;
    }
    log_info("socket opt!\n");
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    DEBUG("memset\n");
    address.sin_family = AF_INET;
    DEBUG("AF_INET\n");
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(serve_config.port);
    DEBUG("SIN_PORT\n");
//    inet_pton(AF_INET, serve_config.ip,
//              &address.sin_addr);  // res.sin_addr.s_addr = htonl(INADDR_ANY);
    DEBUG("pton\n");
    bzero(&(address.sin_zero), 8);
    DEBUG("BEFORE BIND\n");
    res = bind(fd, (struct sockaddr *) &address, sizeof(address));
    checkError(res);
    if (-1 == res) {
        log_error("bind socket error!\n");
        DEBUG("%d\n",errno);
        return -1;
    }
    log_info("bind socket!\n");

    res = listen(fd, 5);
    if (-1 == res) {
        log_error("listen socket error!\n");
        return -2;
    }
    log_info("listen socket!\n");
    log_success("start listen on %s:%d\n", serve_config.ip, serve_config.port);
    return fd;
}


int accept_socket(int listen_fd) {
    DEBUG("accept socket !\n");
    static struct sockaddr_in client;
    socklen_t client_address_length = sizeof client;
    int accept_fd = accept(listen_fd, (struct sockaddr *) &client,
                           &client_address_length);
    if (-1 == accept_fd) {
//        log_error(strerror(errno) );
        log_error("accept aocket fail!\n");
        printf("\n");
        return -1;
    }
    log_info("accept from: %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    return accept_fd;
}

//********* 根据后缀名查找相应的主题类型 ，，， copy
/*
** Guess the mime-type of a document based on its name.
*/
const char *GetMimeType(const char *zName, int nName) {
    const char *z;
    int i;
    int first, last;
    int len;
    char zSuffix[20];

    /* A table of mimetypes based on file suffixes.
    ** Suffixes must be in sorted order so that we can do a binary
    ** search to find the mime-type
    */
    static const struct {
        const char *zSuffix;       /* The file suffix */
        int size;                  /* Length of the suffix */
        const char *zMimetype;     /* The corresponding mimetype */
    } aMime[] = {
            {"ai",      2, "application/postscript"},
            {"aif",     3, "audio/x-aiff"},
            {"aifc",    4, "audio/x-aiff"},
            {"aiff",    4, "audio/x-aiff"},
            {"arj",     3, "application/x-arj-compressed"},
            {"asc",     3, "text/plain"},
            {"asf",     3, "video/x-ms-asf"},
            {"asx",     3, "video/x-ms-asx"},
            {"au",      2, "audio/ulaw"},
            {"avi",     3, "video/x-msvideo"},
            {"bat",     3, "application/x-msdos-program"},
            {"bcpio",   5, "application/x-bcpio"},
            {"bin",     3, "application/octet-stream"},
            {"c",       1, "text/plain"},
            {"cc",      2, "text/plain"},
            {"ccad",    4, "application/clariscad"},
            {"cdf",     3, "application/x-netcdf"},
            {"class",   5, "application/octet-stream"},
            {"cod",     3, "application/vnd.rim.cod"},
            {"com",     3, "application/x-msdos-program"},
            {"cpio",    4, "application/x-cpio"},
            {"cpt",     3, "application/mac-compactpro"},
            {"csh",     3, "application/x-csh"},
            {"css",     3, "text/css"},
            {"dcr",     3, "application/x-director"},
            {"deb",     3, "application/x-debian-package"},
            {"dir",     3, "application/x-director"},
            {"dl",      2, "video/dl"},
            {"dms",     3, "application/octet-stream"},
            {"doc",     3, "application/msword"},
            {"drw",     3, "application/drafting"},
            {"dvi",     3, "application/x-dvi"},
            {"dwg",     3, "application/acad"},
            {"dxf",     3, "application/dxf"},
            {"dxr",     3, "application/x-director"},
            {"eps",     3, "application/postscript"},
            {"etx",     3, "text/x-setext"},
            {"exe",     3, "application/octet-stream"},
            {"ez",      2, "application/andrew-inset"},
            {"f",       1, "text/plain"},
            {"f90",     3, "text/plain"},
            {"fli",     3, "video/fli"},
            {"flv",     3, "video/flv"},
            {"gif",     3, "image/gif"},
            {"gl",      2, "video/gl"},
            {"gtar",    4, "application/x-gtar"},
            {"gz",      2, "application/x-gzip"},
            {"hdf",     3, "application/x-hdf"},
            {"hh",      2, "text/plain"},
            {"hqx",     3, "application/mac-binhex40"},
            {"h",       1, "text/plain"},
            {"htm",     3, "text/html; charset=utf-8"},
            {"html",    4, "text/html; charset=utf-8"},
            {"ice",     3, "x-conference/x-cooltalk"},
            {"ief",     3, "image/ief"},
            {"iges",    4, "model/iges"},
            {"igs",     3, "model/iges"},
            {"ips",     3, "application/x-ipscript"},
            {"ipx",     3, "application/x-ipix"},
            {"jad",     3, "text/vnd.sun.j2me.app-descriptor"},
            {"jar",     3, "application/java-archive"},
            {"jpeg",    4, "image/jpeg"},
            {"jpe",     3, "image/jpeg"},
            {"jpg",     3, "image/jpeg"},
            {"js",      2, "application/x-javascript"},
            {"kar",     3, "audio/midi"},
            {"latex",   5, "application/x-latex"},
            {"lha",     3, "application/octet-stream"},
            {"lsp",     3, "application/x-lisp"},
            {"lzh",     3, "application/octet-stream"},
            {"m",       1, "text/plain"},
            {"m3u",     3, "audio/x-mpegurl"},
            {"man",     3, "application/x-troff-man"},
            {"me",      2, "application/x-troff-me"},
            {"mesh",    4, "model/mesh"},
            {"mid",     3, "audio/midi"},
            {"midi",    4, "audio/midi"},
            {"mif",     3, "application/x-mif"},
            {"mime",    4, "www/mime"},
            {"movie",   5, "video/x-sgi-movie"},
            {"mov",     3, "video/quicktime"},
            {"mp2",     3, "audio/mpeg"},
            {"mp2",     3, "video/mpeg"},
            {"mp3",     3, "audio/mpeg"},
            {"mpeg",    4, "video/mpeg"},
            {"mpe",     3, "video/mpeg"},
            {"mpga",    4, "audio/mpeg"},
            {"mpg",     3, "video/mpeg"},
            {"ms",      2, "application/x-troff-ms"},
            {"msh",     3, "model/mesh"},
            {"nc",      2, "application/x-netcdf"},
            {"oda",     3, "application/oda"},
            {"ogg",     3, "application/ogg"},
            {"ogm",     3, "application/ogg"},
            {"pbm",     3, "image/x-portable-bitmap"},
            {"pdb",     3, "chemical/x-pdb"},
            {"pdf",     3, "application/pdf"},
            {"pgm",     3, "image/x-portable-graymap"},
            {"pgn",     3, "application/x-chess-pgn"},
            {"pgp",     3, "application/pgp"},
            {"pl",      2, "application/x-perl"},
            {"pm",      2, "application/x-perl"},
            {"png",     3, "image/png"},
            {"pnm",     3, "image/x-portable-anymap"},
            {"pot",     3, "application/mspowerpoint"},
            {"ppm",     3, "image/x-portable-pixmap"},
            {"pps",     3, "application/mspowerpoint"},
            {"ppt",     3, "application/mspowerpoint"},
            {"ppz",     3, "application/mspowerpoint"},
            {"pre",     3, "application/x-freelance"},
            {"prt",     3, "application/pro_eng"},
            {"ps",      2, "application/postscript"},
            {"qt",      2, "video/quicktime"},
            {"ra",      2, "audio/x-realaudio"},
            {"ram",     3, "audio/x-pn-realaudio"},
            {"rar",     3, "application/x-rar-compressed"},
            {"ras",     3, "image/cmu-raster"},
            {"ras",     3, "image/x-cmu-raster"},
            {"rgb",     3, "image/x-rgb"},
            {"rm",      2, "audio/x-pn-realaudio"},
            {"roff",    4, "application/x-troff"},
            {"rpm",     3, "audio/x-pn-realaudio-plugin"},
            {"rtf",     3, "application/rtf"},
            {"rtf",     3, "text/rtf"},
            {"rtx",     3, "text/richtext"},
            {"scm",     3, "application/x-lotusscreencam"},
            {"set",     3, "application/set"},
            {"sgml",    4, "text/sgml"},
            {"sgm",     3, "text/sgml"},
            {"sh",      2, "application/x-sh"},
            {"shar",    4, "application/x-shar"},
            {"silo",    4, "model/mesh"},
            {"sit",     3, "application/x-stuffit"},
            {"skd",     3, "application/x-koan"},
            {"skm",     3, "application/x-koan"},
            {"skp",     3, "application/x-koan"},
            {"skt",     3, "application/x-koan"},
            {"smi",     3, "application/smil"},
            {"smil",    4, "application/smil"},
            {"snd",     3, "audio/basic"},
            {"sol",     3, "application/solids"},
            {"spl",     3, "application/x-futuresplash"},
            {"src",     3, "application/x-wais-source"},
            {"step",    4, "application/STEP"},
            {"stl",     3, "application/SLA"},
            {"stp",     3, "application/STEP"},
            {"sv4cpio", 7, "application/x-sv4cpio"},
            {"sv4crc",  6, "application/x-sv4crc"},
            {"svg",     3, "image/svg+xml"},
            {"swf",     3, "application/x-shockwave-flash"},
            {"t",       1, "application/x-troff"},
            {"tar",     3, "application/x-tar"},
            {"tcl",     3, "application/x-tcl"},
            {"tex",     3, "application/x-tex"},
            {"texi",    4, "application/x-texinfo"},
            {"texinfo", 7, "application/x-texinfo"},
            {"tgz",     3, "application/x-tar-gz"},
            {"tiff",    4, "image/tiff"},
            {"tif",     3, "image/tiff"},
            {"tr",      2, "application/x-troff"},
            {"tsi",     3, "audio/TSP-audio"},
            {"tsp",     3, "application/dsptype"},
            {"tsv",     3, "text/tab-separated-values"},
            {"txt",     3, "text/plain"},
            {"unv",     3, "application/i-deas"},
            {"ustar",   5, "application/x-ustar"},
            {"vcd",     3, "application/x-cdlink"},
            {"vda",     3, "application/vda"},
            {"viv",     3, "video/vnd.vivo"},
            {"vivo",    4, "video/vnd.vivo"},
            {"vrml",    4, "model/vrml"},
            {"vsix",    4, "application/vsix"},
            {"wav",     3, "audio/x-wav"},
            {"wax",     3, "audio/x-ms-wax"},
            {"wiki",    4, "application/x-fossil-wiki"},
            {"wma",     3, "audio/x-ms-wma"},
            {"wmv",     3, "video/x-ms-wmv"},
            {"wmx",     3, "video/x-ms-wmx"},
            {"wrl",     3, "model/vrml"},
            {"wvx",     3, "video/x-ms-wvx"},
            {"xbm",     3, "image/x-xbitmap"},
            {"xlc",     3, "application/vnd.ms-excel"},
            {"xll",     3, "application/vnd.ms-excel"},
            {"xlm",     3, "application/vnd.ms-excel"},
            {"xls",     3, "application/vnd.ms-excel"},
            {"xlw",     3, "application/vnd.ms-excel"},
            {"xml",     3, "text/xml"},
            {"xpm",     3, "image/x-xpixmap"},
            {"xwd",     3, "image/x-xwindowdump"},
            {"xyz",     3, "chemical/x-pdb"},
            {"zip",     3, "application/zip"},
    };

    for (i = nName - 1; i > 0 && zName[i] != '.'; i--) {}
    z = &zName[i + 1];
    len = nName - i;
    if (len < (int) sizeof(zSuffix) - 1) {
        strcpy(zSuffix, z);
        for (i = 0; zSuffix[i]; i++) zSuffix[i] = tolower(zSuffix[i]);
        first = 0;
        last = sizeof(aMime) / sizeof(aMime[0]);
        while (first <= last) {
            int c;
            i = (first + last) / 2;
            c = strcmp(zSuffix, aMime[i].zSuffix);
            if (c == 0) return aMime[i].zMimetype;
            if (c < 0) {
                last = i - 1;
            } else {
                first = i + 1;
            }
        }
    }
    return "application/octet-stream";
}




/*
** The following table contains 1 for all characters that are permitted in
** the part of the URL before the query parameters and fragment.
**
** 这段代码借用自 althttp 发现这种查表法更高效一点
** 如果我写,我肯定会来一串数组,然后条件判断
** 我觉得很符合老师说的,现在以空间换速度的思想,于是借用过来
** Allowed characters:  0-9a-zA-Z,-./:_~
**
** Disallowed characters include:  !"#$%&'()*+;<=>?[\]^{|}
*/
static const char allowedInName[] = {
        /*  x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xa  xb  xc  xd  xe  xf */
/* 0x */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 1x */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 2x */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
/* 3x */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
/* 4x */   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 5x */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
/* 6x */   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 7x */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0,
/* 8x */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 9x */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* Ax */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* Bx */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* Cx */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* Dx */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* Ex */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* Fx */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

void parseURl(char *buf) {
    char *p = buf + 1;
    char *index = buf;
    while (*p) {
        while (!allowedInName[*index]) {
            if ('%' == *index && p[0] != 0 && p[1] != 0) {
                *index = (p[0] - '0') < 4;
                *index += (p[1] - '0');
                p += 2;
            } else {
                *index = '_';
            }
        }
        index++;
        *index = *p;
        p++;
    }
    index[1] = 0;
}




/* Render seconds since 1970 as an RFC822 date string.  Return
** a pointer to that string in a static buffer.
*/
static char *Rfc822Date(time_t t) {
    struct tm *tm;
    static char zDate[100];
    tm = gmtime(&t);
    strftime(zDate, sizeof(zDate), "%a, %d %b %Y %H:%M:%S %Z", tm);
    return zDate;
}

/**************************
 * 偷懒 从 althttp copy 来的 日期格式函数
 * 为什么要重复造轮子呢！
 * 就是解析年月日、时分秒、然后计算为秒数构造时间
** Parse an RFC822-formatted timestamp as we'd expect from HTTP and return
** a Unix epoch time. <= zero is returned on failure.
*/
time_t ParseRfc822Date(const char *zDate) {
    int mday, mon, year, yday, hour, min, sec;
    char zIgnore[4];
    char zMonth[4];
    static const char *const azMonths[] =
            {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (7 == sscanf(zDate, "%3[A-Za-z], %d %3[A-Za-z] %d %d:%d:%d", zIgnore,
                    &mday, zMonth, &year, &hour, &min, &sec)) {
        if (year > 1900) year -= 1900;
        for (mon = 0; mon < 12; mon++) {
            if (!strncmp(azMonths[mon], zMonth, 3)) {
                int nDay;
                int isLeapYr;
                static int priorDays[] =
                        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
                isLeapYr = year % 4 == 0 && (year % 100 != 0 || (year + 300) % 400 == 0);
                yday = priorDays[mon] + mday - 1;
                if (isLeapYr && mon > 1) yday++;
                nDay = (year - 70) * 365 + (year - 69) / 4 - year / 100 + (year + 300) / 400 + yday;
                return ((time_t)(nDay * 24 + hour) * 60 + min) * 60 + sec;
            }
        }
    }
    return 0;
}

/************** TEST
 * TEST *Rfc822Date
 * @param serve_socket
 */

void test_Rfc822Date() {
    log_info("start test *Rfc822Date\n");
    time_t t;
//    gettimeofday(&t,0);
    time(&t);
    char *p = Rfc822Date(t);
    printf("%s\n", p);


    log_info("test *Rfc822Date end \n");
}


/**
 * 从给定字符串 获取到第一个以空格分割的元素
 * 遇到 \r\n 将直接返回
 * @param buf
 * @return
 */
static char *get_element_by_space(char *input, char **next_p) {
    char *res = 0;
    if (input == 0 || *input == 0) {
        *next_p = 0;
        return 0;
    }
    // 忽略前面的空白字符
    while (isspace(*input)) input++;
    // 字符串 只有纯空格
    if (*input == 0) {
        *next_p = 0;
        return 0;
    }
    res = input;
    while (!isspace(*input)) {
        input++;
    }
    *input = 0;
    *next_p = input + 1;
    return res;
}

char *response_501() {
    static char buf[256];
    time_t now;
    time(&now);
    sprintf(buf,
            "HTTP/1.1 501 Not Implemented\r\n"
            "Content-Length: %d\r\n"
            "Content-type: text/plain; charset=utf-8\r\n"
            "Data: %s\r\n"
            "\r\n"
            "The method is not implemented on this server.\n",
            strlen("The method is not implemented on this server.\n"),
            Rfc822Date(now));
    return buf;
}

char *response_bad_request_400() {
    static char buf[256];
    time_t now;
    time(&now);
    sprintf(buf,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-type: text/plain; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Data: %s\r\n"
            "\r\n"
            "400 Bad Request\r\n",
            strlen("400 Bad Request\r\n"),
            Rfc822Date(now));

    return buf;
}

char *response_403() {
    static char buf[128];
    time_t now;
    time(&now);
    sprintf(buf,
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-type: text/plain; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Data: %s\r\n"
            "\r\n"
            "403 Forbidden\n",
            strlen("403 Forbidden\n"),
            Rfc822Date(now));

    return buf;
}

char *response_404() {
    static char buf[512];
    time_t now;
    time(&now);
    sprintf(buf,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Data: %s\r\n"
            "\r\n"
            "<html>\n"
            "<head><title>404 Not Found</title></head>\n"
            "<body><h1>Document Not Found</h1>\n"
            "The document is not available on this server\n"
            "</body>\n"
            "</html>\n",
            strlen("<html>\n"
                   "<head><title>404 Not Found</title></head>\n"
                   "<body><h1>Document Not Found</h1>\n"
                   "The document is not available on this server\n"
                   "</body>\n"
                   "</html>\n"),
            Rfc822Date(now));

    return buf;
}



/***
 * 写了一天，很累，不想写注释
 * 但是，里面有个问题，我必须写
 *
 * struct stat 使用 stat(filepath,&stat_buf)
 * 按照说法，成功 return 0 else -1
 * 但是，在检测是否成功时，一直一直失败，不成功。检查参数无错误，
 * 甚至将参数填入固定字符串写死，仍然报错
 * 但是，如果将这个条件语句注释，取消错误检测， it  works ！！！！！
 *
 * 后来 检查文件指针也出现了错误，，，/😵，
 * 悬疑。。。。。。
 * 后来，改用 lstat()
 * 重启 机器,差不多算解决了问题,只是不知道原因,,那就怪缓存吧......
 *
 * 代码注释->
 *
 * 不写了!!!!
 * @param serve_socket
 * @param filename
 * @return
 */
//const char response_404[]

static int send_file(int serve_socket, char *filename) {
    log_info("sendfile===========\n");
    printf("\n\n\"%s\"\n\n", filename);
    char head[128];
    char buf[256];
    int status_code = 200;
    char status_str[64] = "OK";
    const char *zContentType;
    time_t t;
    FILE *fin;
    char zETag[100];
//    struct stat stat_s;
    struct stat stat_buf;//= &stat_s;
    if (lstat(filename, &stat_buf) != 0) {
        log_error("%d %d %d %d %d %d %d\n\t%d\n", EBADF, EFAULT, ELOOP, ENAMETOOLONG, ENOENT, ENOMEM, ENOTDIR, errno);
        log_error("stat file fail! \n\t: %s", filename);
        char *res = response_404();
        send(serve_socket, res, strlen(res), 0);
        exit(0);
    }
    log_info("fopen(%s,'rb')\n", filename);
    fin = fopen(filename, "r");
    if (0 == fin) {
        log_error("%s\n", system("pwd"));
        log_error("open file fail! \n\t: %s\n\terrno: %d\n", filename, errno);
        char *res = response_404();
        send(serve_socket, res, strlen(res), 0);
        exit(0);
    }

    sprintf(zETag, "m%xs%x", (int) stat_buf.st_mtime, (int) stat_buf.st_size);

    zContentType = GetMimeType(filename, strlen(filename));

    sprintf(head, "HTTP/1.1 %d %s\r\n", status_code, status_str);
    send(serve_socket, head, strlen(head), 0);

    sprintf(buf, "Last-Modified", stat_buf.st_mtime);
    send(serve_socket, buf, strlen(buf), 0);
    sprintf(buf, "Cache-Control: max-age=%d\r\n", 10);
    send(serve_socket, buf, strlen(buf), 0);
    sprintf(buf, "ETag: \"%s\"\r\n", zETag);
    send(serve_socket, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: %s; charset=utf-8\r\n", zContentType);
    send(serve_socket, buf, strlen(buf), 0);
    sprintf(buf, "Content-length: %d\r\n\r\n", (int) stat_buf.st_size);
    send(serve_socket, buf, strlen(buf), 0);

    sendfile(serve_socket, fileno(fin), 0, stat_buf.st_size);
    log_info("send file end\n");
    close(serve_socket);
    exit(0);
}


/**
 * 从 socket 中读取一行数据
 *  * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 *             Returns: the number of bytes stored (excluding null)
 */
int get_line(int sock, char *buf, int size) {
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n')) {
        //recv()包含于<sys/socket.h>,参读《TLPI》P1259,
        //读一个字节的数据存放在 c 中
        n = recv(sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        } else
            c = '\n';
    }
    buf[i] = '\0';
    return (i);
}


inline static void GET(int serve_socket) {
//    int
    // TEST
//    send(serve_socket,response_bad_request_400(),strlen(response_bad_request_400()),0);
    //TEST END
    log_info("GET in path :%s\n", http_msg.path);
    FILE *fin = 0;
    if (http_msg.path == 0) {
        log_error("GET :error path!\n");
        send(serve_socket, response_bad_request_400(), strlen(response_bad_request_400()), 0);
    }
    parseURl(http_msg.path);
    log_info("path after parse: %s\n", http_msg.path);
    // 路径过长，拒绝访问
    if (strlen(http_msg.path) > 200) {
        log_error("GET :too long path!\n");
        char *res = response_403();
        send(serve_socket, res, strlen(res), 0);
        exit(0);
    }
    char tmp_path[256] = {0};
    strcat(tmp_path, serve_config.root);
    if (*http_msg.path != '/') {
        log_error("path not start with '/'!\n");
        char *res = response_403();
        send(serve_socket, res, strlen(res), 0);
        exit(0);
    }
    strcat(tmp_path, http_msg.path + 1);      // 忽略 / 符号
    log_info("request path: %s\n", tmp_path);
    log_info("path: %s\n", http_msg.path);

    send_file(serve_socket, tmp_path);
}


static void GC_child(int iSig) {
    while (waitpid(0, 0, WNOHANG) > 0) {
        runtime.connected--;
    }
}

void http_serve(int serve_socket) {
    alarm(serve_config.timeout);
    signal(SIGALRM, serve_config.timeout);

    // 设置 alarm 以便处理超时 退出进程
//    static char line_buf[1024];
//    static char *next_ele = 0;
    char *res = 0;              //  暂存返回字符串指针
    get_line(serve_socket, line_buf, 1024);
    http_msg.method = get_element_by_space(line_buf, &next_ele);
    http_msg.path = get_element_by_space(next_ele, &next_ele);
    http_msg.protocol = get_element_by_space(next_ele, &next_ele);
    log_info("methd: %s path: %s prtcl: %s\n", http_msg.method, http_msg.path, http_msg.protocol);
    // 非 HTTP 协议 或 非支持的 HTTP 版本
    if (http_msg.protocol == 0 || strncmp(http_msg.protocol, "HTTP/", 5) != 0 || strlen(http_msg.protocol) != 8) {
        log_error("unavliable protocol: %s\n", http_msg.protocol);
        res = response_bad_request_400();
        send(serve_socket, res, strlen(res), 0);
        exit(0);
    }
    // 不支持的请求头
    if (strcmp("GET", http_msg.method) == 0) {
        log_info("GET MOV IN PATH: %s\n", http_msg.path);
        GET(serve_socket);
    } else if (strcmp("POST", http_msg.path) == 0) {
        send(serve_socket, response_501(), strlen(response_501()), 0);
    } else if (strcmp("HEAD", http_msg.path) == 0) {
        send(serve_socket, response_501(), strlen(response_501()), 0);
    } else {
        send(serve_socket, response_501(), strlen(response_501()), 0);
//        if (http_msg->alive == 0)
        exit(0);
    }
    close(serve_socket);
    return;

}


/************************ NOTE *************
 *  linux 中 fork 可产生一个子进程，练习之前看过的 UNIX 系统内核的进程实现
 *  ：
 *  unix 系统维护一张进程链表，所以，fork 生出一个进程实际上就是在这个进程链表上复制
 *  一个父进程的节点。所以，在享有的各种资源上 子进程与父进程是一样的
 *  所以 fork 时，子进程 父进程从 fork 处开始分道扬镳，在子进程中：
 *  fork 返回 0
 *  在父进程中：
 *  fork 返回 子进程 pid
 *  所以通过 pid 值区分当前进程
 *
 *  子进程完成后通过 _exit() 或 exit() 退出
 *  _exit() 直接返回 不清理子线程资源
 *  exi() 清理资源后返回
 *
 *  在子进程退出以后    ==============================================
 *  父进程必须调用 wait() 或 waitpid() 进行监听子进程状态和回收子进程
 *  如果 父进程不回收，那么子进程将会变为僵尸进程
 *  wait() 和 waitpid() 为阻塞函数，会一直等待子进程退出
 *
 *  waitpid() 原型 pid_t waitpid(pid_t pid,int * status,int options);
 *
 *  pid 取值如下：
 *      pid<-1 等待进程组识别码为 pid 绝对值的任何子进程。
 *      pid=-1 等待任何子进程,相当于 wait()。
 *      pid=0 等待进程组识别码与进程相同的任何子进程。
 *      pid>0 等待任何子进程识别码为 pid 的子进程。
 *
 *  ========== 非阻塞 监听 =========================
 *  可使用 将 options 设为 WNOHANG
 *  waitpid() 会立即返回
 *  规则如下：
 *      WNOHANG 若pid指定的子进程没有结束，则waitpid()函数返回0，不予以等待。若结束，则返回该子进程的ID。
 *      WUNTRACED 若子进程进入暂停状态，则马上返回，但子进程的结束状态不予以理会。WIFSTOPPED(status)宏确定返回值是否对应与一个暂停子进程。
*       子进程的结束状态返回后存于 status,底下有几个宏可判别结束情况:
*       WIFEXITED(status)如果若为正常结束子进程返回的状态，则为真；对于这种情况可执行WEXITSTATUS(status)，取子进程传给exit或_eixt的低8位。
*       WEXITSTATUS(status)取得子进程 exit()返回的结束代码,一般会先用 WIFEXITED 来判断是否正常结束才能使用此宏。
*       WIFSIGNALED(status)若为异常结束子进程返回的状态，则为真；对于这种情况可执行WTERMSIG(status)，取使子进程结束的信号编号。
*       WTERMSIG(status) 取得子进程因信号而中止的信号代码,一般会先用 WIFSIGNALED 来判断后才使用此宏。
*       WIFSTOPPED(status) 若为当前暂停子进程返回的状态，则为真；对于这种情况可执行WSTOPSIG(status)，取使子进程暂停的信号编号。
*       WSTOPSIG(status) 取得引发子进程暂停的信号代码,一般会先用 WIFSTOPPED 来判断后才使用此宏。
*       如果执行成功则返回子进程识别码(PID) ,如果有错误发生则返回
*       返回值-1。失败原因存于 errno 中。
*
 */
/**
 *
 * @return 1 continue process http ELSE return main
 */
int startup() {
    log_info("start...\n");
    int client_fd = -1;
    int listen_fd = start_listen();
    if (listen_fd == -1) {
        log_error("start listen failed!\n");
        return 0;
    }
    while (1) {
        client_fd = accept_socket(listen_fd);
        if (-1 == client_fd) {
            log_error("connect error!\n");
            continue;
        }
        if (runtime.connected > serve_config.max_client) {
            close(client_fd);
            log_warning("max connected!\n");
            continue;
        }
        static int pid;
        pid = fork();
        if (pid > 0) {
            close(client_fd);
            runtime.connected++;
            while (waitpid(0, 0, WNOHANG) > 0) {
                runtime.connected--;
            }
            alarm(serve_config.timeout);
            signal(SIGALRM, GC_child);
            continue;
        } else if (pid == 0) {
            close(listen_fd);
//            http_serve(client_fd);
            return client_fd;
        } else {
            log_error("can not reach here!\n");
            return 0;
        }

    }
    return 0;
}



int init(int argc, char **argv) {
    char *z;
    char *zArg;
    memset(&serve_config, 0, sizeof serve_config);
    for (int i = 1; i < argc - 1; i++) {
        z = argv[i];
        zArg = argv[i + 1];
        DEBUG("%s\n", z);
        if (z[0] != '-') {
            continue;
        }
        if (strcmp("--root", z) == 0
            || strcmp("-r", z + 1) == 0) {
            if (0 != access(zArg, 0)) {
                log_error("the dir \"%s\" is not exsisi or hava no permition to visit!\n", zArg);
                return 0;
            }
            serve_config.root = str_dup(zArg);
            log_info("root set as %s\n", serve_config.root);
        } else if (strcmp("--port", z) == 0 || strcmp("-p", z) == 0) {
            unsigned int p = atoi(zArg);
            if (p == 0 || p > 65556) {
                log_error("port [%s] error!\n", zArg);
                return 0;
            }
            serve_config.port = p;
            log_info("port set as %d\n", p);
        }  else if (strcmp("--timeout", z) == 0 || strcmp("-t", z) == 0) {
            unsigned int p = atoi(zArg);
            if (p < 1) {
                log_error("timeout [%s] error!\n", zArg);
                return 0;
            }
            serve_config.port = p;
            log_info("timeout set as %d\n", p);
        } else if (strcmp("--maxclient", z) == 0 || strcmp("-mc", z) == 0) {
            unsigned int p = atoi(zArg);
            if (p == 0) {
                log_error("max client [%s] error!\n", zArg);
                return 0;
            }
            serve_config.max_client = p;
            log_info("max client set as %d\n", p);
        } else if (strcmp("--ip", z) == 0 || strcmp("-i", z) == 0) {
            serve_config.ip = str_dup(zArg);
            DEBUG("ip set as %s\n", serve_config.ip);
        } else {
            log_error("unknown arg '%s'\n", z);
            return 0;
        }
    }
    if (serve_config.port == 0) {
        serve_config.port = DEFAULT_PORT;
    }
    if (serve_config.root == 0) {
        serve_config.root = str_dup(DEFAULT_ROOT);
    }
    if (serve_config.ip == 0) {
        serve_config.ip = str_dup(DEFAULT_IP);
    }
    if (serve_config.max_client == 0) {
        serve_config.max_client = DEFAULT_MAX_CLIENT;
    }
    if (serve_config.timeout <= 0)
    {
        serve_config.timeout = DEFAULT_TIMEOUT;
    }
    serve_config.run = 1;
    return 1;
}



int main(int argc, char **argv) {
    if (0 == init(argc, argv)) {
        log_info("start fail!\n");
        return 0;
    }
    int res = startup();
    if (res) {
        http_serve(res);
    }

    return 0;
}

