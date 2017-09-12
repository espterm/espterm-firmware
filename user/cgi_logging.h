//
// Created by MightyPork on 2017/09/10.
//

#ifndef ESPTERM_CGI_LOGGING_H
#define ESPTERM_CGI_LOGGING_H

#if DEBUG_CGI
#define cgi_warn warn
#define cgi_dbg dbg
#define cgi_info info
#else
#define cgi_warn(fmt, ...)
#define cgi_dbg(fmt, ...)
#define cgi_info(fmt, ...)
#endif

#endif //ESPTERM_CGI_LOGGING_H
