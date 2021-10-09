//
// Created by aja on 2021/10/6.
//

#ifndef PROJ_HTTP_H
#define PROJ_HTTP_H

#include <map>

enum HTTP_METHOD {
    GET = 1,
    POST,
    HEAD,
    PUT,
    OPTIONS,
    DELETE,

};

std::string RES_404 =
        "<HTML><TITLE>Not Found</TITLE>\r\n"
        "<BODY><P>The server could not fulfill\r\n"
        "your request because the resource specified\r\n"
        "is unavailable or nonexistent.\r\n"
        "</BODY></HTML>\r\n";

std::string RES_501_ =
        "<HTML><HEAD><TITLE>Method Not Implemented\r\n"
        "</TITLE></HEAD>\r\n"
        "<BODY><P>HTTP request method not supported.\r\n"
        "</BODY></HTML>\r\n";

class HTTP {
    std::map<std::string,std::string> HEADER;
    std::string BODY;

public:
    enum HTTP_METHOD METHOD;
    std::string HOST;
    std::string URL;


public:
    HTTP(int serve);
    std::string getHead(std::string name);
    std::string parseURL(std::string url);
    void GET();
    void PUT();
    void POST();
    void DELETE();
    void HEAD();
    void OPTIONS();
    ~HTTP();
};


#endif //PROJ_HTTP_H
