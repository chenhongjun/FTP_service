#ifndef _FTP_PROTO_H_
#define _FTP_PROTO_H_

#include "session.h"

void handle_child(session_t* psess);

int list_common(void);

#endif /*_FTP_PROTO_H_*/
