



#include <iostream>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "HTTP.h"


/************ CONFIG ****************/
#define LISTEN_PORT 8080
#define MAX_CONNECT 100
#define SERVE_HOST "0.0.0.0"
//#define

typedef struct config serve_config;
//typedef struct http_msg httpMsg;
//
//struct  http_msg {
//    map<std::string,std::string > HEADER;
//};

struct config{
    int listen_port;
    char listen_ip[17];
    int max_client;
    int connected;
};

serve_config S_CONFIG = {8088,"0.0.0.0",255,0};

std::string payload =
        "HTTP/1.1 200 OK\r\n"
        "Server: nginx/1.15.11\r\n"
        "Date: Wed, 29 Sep 2021 15:05:56 GMT\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 146\r\n"
        //        "Last-Modified: Wed, 29 Sep 2021 15:05:51 GMT\r\n"
        "Connection: keep-alive\r\n"
        "ETag: \"615480cf-92\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "\r\n"
        "<html>\r\n"
        "<head>\r\n"
        "    <title>hello world</title>\r\n"
        "</head>\r\n"
        "<body>\r\n"
        "    <h1>\r\n"
        "        HELLO J1S\r\n"
        "    </h1>\r\n"
        "    <p>GOTO YOURS</p>\r\n"
        "</body>\r\n"
        "\r\n"
        "</html>\r\n";

sockaddr_in SockAddress(int port, const std::string& ip) {
    sockaddr_in res;
    res.sin_family = AF_INET;
    res.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(),
              &res.sin_addr);  // res.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(res.sin_zero), 8);
    return res;
}

/**
 * 一个文件类型映射表,数目太多,直接借用了别人写好的
 * 类型列表按字母排列,以便进行二分查找
 */
static const struct {
    const char *zSuffix;       /* The file suffix */
    int size;                  /* Length of the suffix */
    const char *zMimetype;     /* The corresponding mimetype */
} aMime[] = {
        { "ai",         2, "application/postscript"            },
        { "aif",        3, "audio/x-aiff"                      },
        { "aifc",       4, "audio/x-aiff"                      },
        { "aiff",       4, "audio/x-aiff"                      },
        { "arj",        3, "application/x-arj-compressed"      },
        { "asc",        3, "text/plain"                        },
        { "asf",        3, "video/x-ms-asf"                    },
        { "asx",        3, "video/x-ms-asx"                    },
        { "au",         2, "audio/ulaw"                        },
        { "avi",        3, "video/x-msvideo"                   },
        { "bat",        3, "application/x-msdos-program"       },
        { "bcpio",      5, "application/x-bcpio"               },
        { "bin",        3, "application/octet-stream"          },
        { "c",          1, "text/plain"                        },
        { "cc",         2, "text/plain"                        },
        { "ccad",       4, "application/clariscad"             },
        { "cdf",        3, "application/x-netcdf"              },
        { "class",      5, "application/octet-stream"          },
        { "cod",        3, "application/vnd.rim.cod"           },
        { "com",        3, "application/x-msdos-program"       },
        { "cpio",       4, "application/x-cpio"                },
        { "cpt",        3, "application/mac-compactpro"        },
        { "csh",        3, "application/x-csh"                 },
        { "css",        3, "text/css"                          },
        { "dcr",        3, "application/x-director"            },
        { "deb",        3, "application/x-debian-package"      },
        { "dir",        3, "application/x-director"            },
        { "dl",         2, "video/dl"                          },
        { "dms",        3, "application/octet-stream"          },
        { "doc",        3, "application/msword"                },
        { "drw",        3, "application/drafting"              },
        { "dvi",        3, "application/x-dvi"                 },
        { "dwg",        3, "application/acad"                  },
        { "dxf",        3, "application/dxf"                   },
        { "dxr",        3, "application/x-director"            },
        { "eps",        3, "application/postscript"            },
        { "etx",        3, "text/x-setext"                     },
        { "exe",        3, "application/octet-stream"          },
        { "ez",         2, "application/andrew-inset"          },
        { "f",          1, "text/plain"                        },
        { "f90",        3, "text/plain"                        },
        { "fli",        3, "video/fli"                         },
        { "flv",        3, "video/flv"                         },
        { "gif",        3, "image/gif"                         },
        { "gl",         2, "video/gl"                          },
        { "gtar",       4, "application/x-gtar"                },
        { "gz",         2, "application/x-gzip"                },
        { "hdf",        3, "application/x-hdf"                 },
        { "hh",         2, "text/plain"                        },
        { "hqx",        3, "application/mac-binhex40"          },
        { "h",          1, "text/plain"                        },
        { "htm",        3, "text/html; charset=utf-8"          },
        { "html",       4, "text/html; charset=utf-8"          },
        { "ice",        3, "x-conference/x-cooltalk"           },
        { "ief",        3, "image/ief"                         },
        { "iges",       4, "model/iges"                        },
        { "igs",        3, "model/iges"                        },
        { "ips",        3, "application/x-ipscript"            },
        { "ipx",        3, "application/x-ipix"                },
        { "jad",        3, "text/vnd.sun.j2me.app-descriptor"  },
        { "jar",        3, "application/java-archive"          },
        { "jpeg",       4, "image/jpeg"                        },
        { "jpe",        3, "image/jpeg"                        },
        { "jpg",        3, "image/jpeg"                        },
        { "js",         2, "application/x-javascript"          },
        { "kar",        3, "audio/midi"                        },
        { "latex",      5, "application/x-latex"               },
        { "lha",        3, "application/octet-stream"          },
        { "lsp",        3, "application/x-lisp"                },
        { "lzh",        3, "application/octet-stream"          },
        { "m",          1, "text/plain"                        },
        { "m3u",        3, "audio/x-mpegurl"                   },
        { "man",        3, "application/x-troff-man"           },
        { "me",         2, "application/x-troff-me"            },
        { "mesh",       4, "model/mesh"                        },
        { "mid",        3, "audio/midi"                        },
        { "midi",       4, "audio/midi"                        },
        { "mif",        3, "application/x-mif"                 },
        { "mime",       4, "www/mime"                          },
        { "movie",      5, "video/x-sgi-movie"                 },
        { "mov",        3, "video/quicktime"                   },
        { "mp2",        3, "audio/mpeg"                        },
        { "mp2",        3, "video/mpeg"                        },
        { "mp3",        3, "audio/mpeg"                        },
        { "mpeg",       4, "video/mpeg"                        },
        { "mpe",        3, "video/mpeg"                        },
        { "mpga",       4, "audio/mpeg"                        },
        { "mpg",        3, "video/mpeg"                        },
        { "ms",         2, "application/x-troff-ms"            },
        { "msh",        3, "model/mesh"                        },
        { "nc",         2, "application/x-netcdf"              },
        { "oda",        3, "application/oda"                   },
        { "ogg",        3, "application/ogg"                   },
        { "ogm",        3, "application/ogg"                   },
        { "pbm",        3, "image/x-portable-bitmap"           },
        { "pdb",        3, "chemical/x-pdb"                    },
        { "pdf",        3, "application/pdf"                   },
        { "pgm",        3, "image/x-portable-graymap"          },
        { "pgn",        3, "application/x-chess-pgn"           },
        { "pgp",        3, "application/pgp"                   },
        { "pl",         2, "application/x-perl"                },
        { "pm",         2, "application/x-perl"                },
        { "png",        3, "image/png"                         },
        { "pnm",        3, "image/x-portable-anymap"           },
        { "pot",        3, "application/mspowerpoint"          },
        { "ppm",        3, "image/x-portable-pixmap"           },
        { "pps",        3, "application/mspowerpoint"          },
        { "ppt",        3, "application/mspowerpoint"          },
        { "ppz",        3, "application/mspowerpoint"          },
        { "pre",        3, "application/x-freelance"           },
        { "prt",        3, "application/pro_eng"               },
        { "ps",         2, "application/postscript"            },
        { "qt",         2, "video/quicktime"                   },
        { "ra",         2, "audio/x-realaudio"                 },
        { "ram",        3, "audio/x-pn-realaudio"              },
        { "rar",        3, "application/x-rar-compressed"      },
        { "ras",        3, "image/cmu-raster"                  },
        { "ras",        3, "image/x-cmu-raster"                },
        { "rgb",        3, "image/x-rgb"                       },
        { "rm",         2, "audio/x-pn-realaudio"              },
        { "roff",       4, "application/x-troff"               },
        { "rpm",        3, "audio/x-pn-realaudio-plugin"       },
        { "rtf",        3, "application/rtf"                   },
        { "rtf",        3, "text/rtf"                          },
        { "rtx",        3, "text/richtext"                     },
        { "scm",        3, "application/x-lotusscreencam"      },
        { "set",        3, "application/set"                   },
        { "sgml",       4, "text/sgml"                         },
        { "sgm",        3, "text/sgml"                         },
        { "sh",         2, "application/x-sh"                  },
        { "shar",       4, "application/x-shar"                },
        { "silo",       4, "model/mesh"                        },
        { "sit",        3, "application/x-stuffit"             },
        { "skd",        3, "application/x-koan"                },
        { "skm",        3, "application/x-koan"                },
        { "skp",        3, "application/x-koan"                },
        { "skt",        3, "application/x-koan"                },
        { "smi",        3, "application/smil"                  },
        { "smil",       4, "application/smil"                  },
        { "snd",        3, "audio/basic"                       },
        { "sol",        3, "application/solids"                },
        { "spl",        3, "application/x-futuresplash"        },
        { "src",        3, "application/x-wais-source"         },
        { "step",       4, "application/STEP"                  },
        { "stl",        3, "application/SLA"                   },
        { "stp",        3, "application/STEP"                  },
        { "sv4cpio",    7, "application/x-sv4cpio"             },
        { "sv4crc",     6, "application/x-sv4crc"              },
        { "svg",        3, "image/svg+xml"                     },
        { "swf",        3, "application/x-shockwave-flash"     },
        { "t",          1, "application/x-troff"               },
        { "tar",        3, "application/x-tar"                 },
        { "tcl",        3, "application/x-tcl"                 },
        { "tex",        3, "application/x-tex"                 },
        { "texi",       4, "application/x-texinfo"             },
        { "texinfo",    7, "application/x-texinfo"             },
        { "tgz",        3, "application/x-tar-gz"              },
        { "tiff",       4, "image/tiff"                        },
        { "tif",        3, "image/tiff"                        },
        { "tr",         2, "application/x-troff"               },
        { "tsi",        3, "audio/TSP-audio"                   },
        { "tsp",        3, "application/dsptype"               },
        { "tsv",        3, "text/tab-separated-values"         },
        { "txt",        3, "text/plain"                        },
        { "unv",        3, "application/i-deas"                },
        { "ustar",      5, "application/x-ustar"               },
        { "vcd",        3, "application/x-cdlink"              },
        { "vda",        3, "application/vda"                   },
        { "viv",        3, "video/vnd.vivo"                    },
        { "vivo",       4, "video/vnd.vivo"                    },
        { "vrml",       4, "model/vrml"                        },
        { "vsix",       4, "application/vsix"                  },
        { "wav",        3, "audio/x-wav"                       },
        { "wax",        3, "audio/x-ms-wax"                    },
        { "wiki",       4, "application/x-fossil-wiki"         },
        { "wma",        3, "audio/x-ms-wma"                    },
        { "wmv",        3, "video/x-ms-wmv"                    },
        { "wmx",        3, "video/x-ms-wmx"                    },
        { "wrl",        3, "model/vrml"                        },
        { "wvx",        3, "video/x-ms-wvx"                    },
        { "xbm",        3, "image/x-xbitmap"                   },
        { "xlc",        3, "application/vnd.ms-excel"          },
        { "xll",        3, "application/vnd.ms-excel"          },
        { "xlm",        3, "application/vnd.ms-excel"          },
        { "xls",        3, "application/vnd.ms-excel"          },
        { "xlw",        3, "application/vnd.ms-excel"          },
        { "xml",        3, "text/xml"                          },
        { "xpm",        3, "image/x-xpixmap"                   },
        { "xwd",        3, "image/x-xwindowdump"               },
        { "xyz",        3, "chemical/x-pdb"                    },
        { "zip",        3, "application/zip"                   },
};


void cat(int client, FILE *resource)
{
    char buf[1024];
    //从文件文件描述符中读取指定内容
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

int ListenFd(int port, const std::string& ip, int backlog = 5) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // 创建套接字
    //  协议族，类型，协议 ipv4 提供面向连接的稳定数据传输，即TCP协议
    /*
     * ============ 族
     * AF_INET      ipv4
     * AF_INET6     ipv6
     * AF_LOCAL     unix 域协议
     * AF_ROUTE     路由套接口
     * AF_KEY       密钥套接口
     *
     * =========== 类型
     * SOCK_STREAM  字节流套接口
     * SOCK_DGRAW   数据报套接口
     * SOCK_RAW     原始套接口
     */

    if (fd == -1) {
//        std::cout << "errno: " << strerror(errno) << '\n';
        return -1;
    }
    int reuse = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
//        std::cout << "errno: " << strerror(errno) << '\n';
        return -1;
    }
    sockaddr_in address = SockAddress(port, ip);
    if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
//        std::cout << "errno: " << strerror(errno) << '\n';
        return -1;
    }
    if (listen(fd, backlog) == -1) {
//        std::cout << "errno: " << strerror(errno) << '\n';
        return -1;
    }

    return fd;
}

int AcceptFd(int listen_fd) {
    sockaddr_in client;
    socklen_t client_address_length = sizeof(client);
    int accept_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client),
                           &client_address_length);
    if (accept_fd == -1) {
//        std::cout << "errno: " << strerror(errno) << '\n';
        return -1;
    }
    std::cout << "receive connect request from " << inet_ntoa(client.sin_addr)
              << ':' << ntohs(client.sin_port) << '\n';
    return accept_fd;
}

std::string Recve(int accept_fd)
{
    char recBuf[1024];
    recv(accept_fd, recBuf, sizeof (recBuf), 0);
    return std ::string(recBuf);
}

void Send(int accept_fd, const std::string& s) {
    send(accept_fd, s.c_str(), strlen(s.c_str()), 0);
}

/**
 *  超时处理 signal
 * @param iSig
 */
static void Timeout(int iSig){
    printf("signal : %d\n",iSig);
}


void *start_listen(void *arg)
{
    serve_config * config = (serve_config*)arg;
    int listen_fd = ListenFd(config->listen_port,config->listen_ip);
    if (listen_fd == -1)
    {
        std::cout << "error : start listen error!"<<std::endl;
        return 0;
    }
    int accept_fd = AcceptFd(listen_fd);
    std::string retval =  Recve(accept_fd);
    std::cout  << retval <<std::endl;
    Send(accept_fd, "welcome to join");
    close(accept_fd);
    close(listen_fd);
    return 0;
}

/**
 * 从 socket 中读取一行数据
 *  * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 *             Returns: the number of bytes stored (excluding null)
 */
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        //recv()包含于<sys/socket.h>,参读《TLPI》P1259,
        //读一个字节的数据存放在 c 中
        n = recv(sock, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
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
/* 0x */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 1x */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 2x */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
/* 3x */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,
/* 4x */   0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 5x */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  1,
/* 6x */   0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 7x */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  1,  0,
/* 8x */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 9x */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* Ax */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* Bx */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* Cx */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* Dx */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* Ex */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* Fx */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

void parseURl(char *buf)
{
    char *p = buf + 1;
    char *index = buf;
    while (*p)
    {
        while ( !allowedInName[*index] )
        {
            if ('%' == *index && p[0] != 0 && p[1] != 0)
            {
                *index = (p[0] - '0') < 4;
                *index += (p[1] - '0');
                p += 2;
            } else
            {
                *index = '_';
            }
        }
        index++;
        *index = *p;
        p++;
    }
    *index = 0;
}

std::string RES_501 =
        "HTTP/1.0 501 Method Not Implemented\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<HTML><HEAD><TITLE>Method Not Implemented\r\n"
        "</TITLE></HEAD>\r\n"
        "<BODY><P>HTTP request method not supported.\r\n"
        "</BODY></HTML>\r\n";

void* process_request(void *serve)
{
    int serve_socket = *(int*)serve;

    if (S_CONFIG.connected > S_CONFIG.max_client)
    {
        Send(serve_socket,RES_501);
    }
    S_CONFIG.connected++;

    printf("max: %d | connected: %d\n", S_CONFIG.max_client,S_CONFIG.connected);
    signal(SIGALRM, Timeout);
    alarm(2);
//    std::string req = Recve(serve_socket);
//    std::cout << req <<std::endl;
//    sleep(1);
    char buf[1024] = {0};
    char method[10] = {0};
    char path[256] = {0};
    char version[20] = {0};
    get_line(serve_socket,buf,1024);

    sscanf(buf,"%s %s %s",method,path,version);

    if (0 == strcmp(method, "GET"))
    {
        parseURl(path);
        printf("%s %s %s",method,path,version);
    } else if (0 == strcmp(method,"POST"))
    {

        parseURl(path);
        printf("%s %s %s",method,path,version);
    } else if ( 0 == strcmp(method,"HEAD"))
    {

        parseURl(path);
        printf("%s %s %s",method,path,version);
    } else if (0 == strcmp(method,"PUT"))
    {

        parseURl(path);
        printf("%s %s %s",method,path,version);
    } else if (0 == strcmp(method, "DELETE"))
    {

        parseURl(path);
        printf("%s %s %s",method,path,version);
    } else if (0 == strcmp(method,"OPTIONS"))
    {

        parseURl(path);
        printf("%s %s %s",method,path,version);
    } else
    {

        parseURl(path);
        printf("%s %s %s",method,path,version);
    }
//    for (int i = 0; i < 6;i++)
//    {
//        sleep(1);
//        printf("%s | sleep : %d\n",path,i);
//    }
    Send(serve_socket,payload);
    close(serve_socket);
    S_CONFIG.connected--;
    printf("max: %d | connected: %d\n", S_CONFIG.max_client,S_CONFIG.connected);
    return 0;
}

void startup(){
    int listen_fd = ListenFd(8088,"0.0.0.0");
    if (listen_fd == -1)
    {
        std::cout << "error : start listen error!"<<std::endl;
        return;
    }
    int serve_socket = -1;
    int rc = -1;
    while (1)
    {
        serve_socket = AcceptFd(listen_fd);
        if (serve_socket == -1)
        {
            printf("error : 连接失败!");
            continue;
        }
        if (S_CONFIG.connected > S_CONFIG.max_client)
        {
            Send(serve_socket,RES_501);
            sleep(1);
        }
        pthread_t tid = 0;

        std :: cout << "创建新线程" << std::endl;
        rc = pthread_create(&tid,NULL,process_request,&serve_socket);
        if (rc == -1)
        {
            std::cout << "新建线程失败！" <<std::endl;
        }
    }
    close(listen_fd);
}

int main (){
    printf("INFO:start serve on %s:%d\n","0.0.0.0",8088);
    //    //    ====================== TEST ++++++
    //    char str[100] = "/this%20is^a[]%25%20test%20url.html";
    //    printf("%s\n",str);
    //    parseURl(str);
    //    printf("%s\n",str);

    startup();


    printf("main end! exit 0!\n");
    return 0;
}