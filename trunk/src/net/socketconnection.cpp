#include "message.h"
#include "socket.h"
#include "simple_messenger.h"
#include "socketconnection.h"

SocketConnection::~SocketConnection()
{
    if (_socket)
    {
        _socket->dec();
        _socket = NULL;
    }
}

Socket* SocketConnection::get_socket()
{
    Mutex::Locker locker(_lock);
    if (_socket)
    {
        return _socket->get();
    }
    
    return NULL;
}

bool SocketConnection::try_get_socket(Socket** s)
{
    Mutex::Locker locker(_lock);
    if (_failed)
    {
        *s = NULL;
    }
    else
    {
        if (_socket)
        {
            *s = _socket->get();
        }
        else
        {
            *s = NULL;
        }
    }
    
    return !_failed;
}

bool SocketConnection::clear_socket(Socket* s)
{
    Mutex::Locker locker(_lock);
    
    if (s == _socket)
    {
        _socket->dec();
        _socket = NULL;
        _failed = true;
        return true;
    }
    
    return false;
}

void SocketConnection::reset_socket(Socket* s)
{
    Mutex::Locker locker(_lock);
    
    if (_socket)
    {
        DEBUG_LOG("current ref count is %d", _socket->get_ref());
        _socket->dec();
    }
    
    _socket = s->get();
    
    DEBUG_LOG("new socket ref count is %d", _socket->get_ref());
}

bool SocketConnection::is_connected()
{
    return static_cast<SimpleMessenger*>(_msgr)->is_connected(this);
}

int SocketConnection::send_message(Message* m)
{
    return static_cast<SimpleMessenger*>(_msgr)->send_message(m, this);
}

void SocketConnection::send_keepalive()
{
    if (_msgr)
    {
        static_cast<SimpleMessenger*>(_msgr)->send_keepalive(this);
    }
}

void SocketConnection::mark_down()
{
    if (_msgr)
    {
        static_cast<SimpleMessenger*>(_msgr)->mark_down(this);
    }
}

void SocketConnection::mark_disposable()
{
    if (_msgr)
    {
        static_cast<SimpleMessenger*>(_msgr)->mark_disposable(this);
    }
}

