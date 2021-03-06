/*
 * (C) 2007-2010 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id$
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *
 */

#ifndef TBSYS_COND_H
#define TBSYS_COND_H
#include "tbsys.h"
#include "Time.h"
#include "ThreadException.h"

namespace tbutil
{
template<class T> class Monitor;
class Mutex;
class RecMutex;
/** 
 * @brief linux线程条件变量
 */
/**
 * 其实这里用public继承也行，但是不优雅
 * 因为这里是一个 is-implemented-in-terms-of 而非 is-a 的关系
 **/
class Cond : private noncopyable
{
public:

    Cond();
    ~Cond();

    /** 
     * @brief 发信号给线程
     */
    void signal();

    /** 
     * @brief 广播信号给线程
     */
    void broadcast();

    /** 
     * @brief 线程阻塞等待 
     */
    template <typename Lock> inline bool 
    wait(const Lock& lock) const
    {
        /**
         * 函数 acquired() 见 tbsys/src/Lock.h
         * 只有类Mutex 或 类RecMutex 获得了锁或者刚开始构造的时候，_acquired才为true
         **/
        if(!lock.acquired())
        {
#ifdef _NO_EXCEPTION
            TBSYS_LOG(ERROR,"%s","ThreadLockedException");
            return false;
#else
            throw ThreadLockedException(__FILE__, __LINE__);
#endif
        }
        return waitImpl(lock._mutex);
    }

    /** 
     * @brief 线程阻塞等待,到达定时时间自动运行
     */
    template <typename Lock> inline bool
    timedWait(const Lock& lock, const Time& timeout) const
    {
        /**
         * 函数 acquired() 见 tbsys/src/Lock.h
         * 只有类Mutex 或 类RecMutex 获得了锁或者刚开始构造的时候，_acquired才为true
         **/
        if(!lock.acquired())
        {
#ifdef _NO_EXCEPTION
            TBSYS_LOG(ERROR,"%s","ThreadLockedException");
            return false;
#else
            throw ThreadLockedException(__FILE__, __LINE__);
#endif
        }
        return timedWaitImpl(lock._mutex, timeout);
    }

private:

    friend class Monitor<Mutex>;
    friend class Monitor<RecMutex>;

    template <typename M> bool waitImpl(const M&) const;
    template <typename M> bool timedWaitImpl(const M&, const Time&) const;

    mutable pthread_cond_t _cond;
};

template <typename M> inline bool 
Cond::waitImpl(const M& mutex) const
{
    typedef typename M::LockState LockState;
    
    LockState state;
    mutex.unlock(state);
    const int rc = pthread_cond_wait(&_cond, state.mutex);
    mutex.lock(state);
    
#ifdef _NO_EXCEPTION
    if( 0 != rc )
    {
        TBSYS_LOG(ERROR,"%s","ThreadSyscallException");
        return false;
    } 
#else
    if(0 != rc)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, rc);
    }
#endif
    return true;
}

template <typename M> inline bool
Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
    if(timeout < Time::microSeconds(0))
    {
#ifdef _NO_EXCEPTION
        TBSYS_LOG(ERROR,"%s","InvalidTimeoutException");
        return false;
#else
        throw InvalidTimeoutException(__FILE__, __LINE__, timeout);
#endif
    }

    typedef typename M::LockState LockState;
    
    LockState state;
    mutex.unlock(state);
   
    timeval tv = Time::now(Time::Realtime) + timeout;
    timespec ts;
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    
    const int rc = pthread_cond_timedwait(&_cond, state.mutex, &ts);
    mutex.lock(state);
    
    if(rc != 0)
    {
#ifdef _NO_EXCEPTION
        if ( rc != ETIMEDOUT )
        {
            TBSYS_LOG(ERROR,"%s","ThreadSyscallException");
            return false;
        }
#else
        if(rc != ETIMEDOUT)
        {
            throw ThreadSyscallException(__FILE__, __LINE__, rc);
        }
#endif
    }
    return true;
}
}// end namespace 
#endif
