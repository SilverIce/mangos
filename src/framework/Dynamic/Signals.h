#pragma once

#include "Platform/Define.h"

#include <list>

template<typename F_Ptr_Type>
struct function_ptr
{
    function_ptr() : fptr(0) {}

    F_Ptr_Type fptr;

    void operator = (const F_Ptr_Type& f) { fptr = f; }

    void sign(const F_Ptr_Type& f) { fptr = f; }
};

/*
    MAKE_EVENT(class_name, event_name, event_args)

    class_name - class namespace, ...
    event_name - name of the event (name should be unique)
    event_args - event arguments, amount of arguments are unlimited,
                 put 'void' argument if there wouldn't any args
*/

#define MAKE_EVENT( class_, name, ... )\
        function_ptr<void (class_::*)( __VA_ARGS__ ) > name\

template<typename Events> class Observer;

template<typename Events>
class IRecvr : public Events
{
protected:

    typedef typename Observer<Events> _Observer;
    typedef typename IRecvr<Events> _IRecvr;
    typedef typename Events _Events;

    friend class _Observer;

public:

    IRecvr() : obs(0)
    {
    }

    // should have private access only
    void Observe(_Observer * o);
    void Detach();

    template<typename F_Ptr_Type>
    void connect(function_ptr<F_Ptr_Type> Events::*e, F_Ptr_Type handler)
    {
        (this->*e).sign(handler);
    }

    template<typename F_Ptr_Type, typename Derived_F_Ptr_Type>
    void connect(function_ptr<F_Ptr_Type> Events::*e, Derived_F_Ptr_Type handler)
    {
        (this->*e).sign(static_cast<F_Ptr_Type>(handler));
    }

    template<typename F_Ptr_Type>
    void disconnect(function_ptr<F_Ptr_Type> Events::*e)
    {
        (this->*e).sign(static_cast<F_Ptr_Type>(NULL));
    }

    void disconnect_all()
    {
        memset(static_cast<Events*>(this),NULL,sizeof(Events));
    }

private:

    typename std::list<_IRecvr*>::iterator position;
    _Observer * obs;
};


template<typename Events >
class Observer
{
    typedef Events _Events;
    typedef typename IRecvr<_Events> _IRecvr;

    friend class _IRecvr;

    std::list<_IRecvr*> m_receivers;

    void attach(_IRecvr & c)
    {
        c.position = m_receivers.insert(m_receivers.end(),&c);
    }

    void detach(_IRecvr & c)
    {
        m_receivers.erase(c.position);
    }

public:

    #define NOTIFY(...)\
    {\
        if (m_receivers.empty())\
            return;\
        \
        for(std::list<_IRecvr*>::iterator itr = m_receivers.begin(); itr != m_receivers.end();)\
        {\
            _IRecvr &rec = **(itr++);\
            if (Sign ptr = (rec.*e).fptr)\
                (rec.*ptr)( __VA_ARGS__ );\
        }\
    }\

    template<typename Sign>
    void Notify(function_ptr<Sign> Events::*e)
    {
        if (m_receivers.empty())
            return;

        for(std::list<_IRecvr*>::iterator itr = m_receivers.begin(); itr != m_receivers.end();)
        {
            _IRecvr &rec = **(itr++);
            if (Sign ptr = (rec.*e).fptr)
                (rec.*ptr)();
        }
    }

    template<typename Sign,typename Arg1>
    void Notify(function_ptr<Sign> Events::*e,Arg1 a1) NOTIFY(a1)

    template<typename Sign,typename Arg1,typename Arg2>
    void Notify(function_ptr<Sign> Events::*e,Arg1 a1,Arg2 a2) NOTIFY(a1,a2)

    template<typename Sign,typename Arg1,typename Arg2,typename Arg3>
    void Notify(function_ptr<Sign> Events::*e,Arg1 a1,Arg2 a2,Arg2 a3) NOTIFY(a1,a2,a3)
        
    template<typename Sign,typename Arg1,typename Arg2,typename Arg3,typename Arg4>
    void Notify(function_ptr<Sign> Events::*e,Arg1 a1,Arg2 a2,Arg2 a3,Arg4 a4) NOTIFY(a1,a2,a3,a4)

    #undef NOTIFY
};

template<typename Events>
void IRecvr<Events>::Observe(_Observer * o)
{
    if (obs)
        obs->detach(*this);

    obs = o;

    if (obs)
        obs->attach(*this);
}

template<typename Events>
void IRecvr<Events>::Detach()
{
    assert(obs);
    obs->detach(*this);
    obs = NULL;
}
