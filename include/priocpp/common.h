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


class IoEx  : public repro::ReproEx<IoEx> { public : IoEx(const std::string& s) : repro::ReproEx<IoEx>(s) {} };

class IoErr : public repro::ReproEx<IoErr> { public : IoErr(const std::string& s) : repro::ReproEx<IoErr>(s) {} };
class IoEof : public repro::ReproEx<IoEof> { public : IoEof(const std::string& s) : repro::ReproEx<IoEof>(s) {} };
class IoTimeout : public repro::ReproEx<IoTimeout> { public : IoTimeout(const std::string& s) : repro::ReproEx<IoTimeout>(s) {} };


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

