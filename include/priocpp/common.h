#ifndef _MOL_DEF_GUARD_DEFINE_MOD_LIBEVENT_COMMON_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_LIBEVENT_COMMON_DEF_GUARD_

#ifndef _WIN32
#include <unistd.h>
#else
typedef unsigned short ushort;
#include <winsock2.h>
#endif

#include <vector>
#include <deque>
#include <string.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "reprocpp/promise.h"
   
namespace prio  {


class IoEx  : public repro::Ex { public : IoEx(const std::string& s) : repro::Ex(s) {} };

class IoErr : public IoEx { public : IoErr(const std::string& s) : IoEx(s) {} };
class IoEof : public IoEx { public : IoEof(const std::string& s) : IoEx(s) {} };
class IoTimeout : public IoEx { public : IoTimeout(const std::string& s) : IoEx(s) {} };


std::string trim(const std::string& input);

std::string nonce(unsigned int n);

std::string base64_encode(const std::string& bytes_to_encode);
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
std::string base64_decode(const std::string& encoded_string);

#ifndef _WIN32
typedef int socket_t;
#else
typedef intptr_t socket_t;
#endif


//////////////////////////////////////////////////////////////
// forwards
//////////////////////////////////////////////////////////////

struct TimeoutImpl;
struct SignalImpl;
struct ListenerImpl;
struct IOImpl;
struct SslCtxImpl;

class Loop;
class ThreadPool;
class Connection;
class Timeout;
class Signal;
class Listener;
class IO;
class SslCtx;

typedef std::shared_ptr<Connection> ConnectionPtr;

}

#endif

