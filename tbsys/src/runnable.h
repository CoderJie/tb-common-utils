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

#ifndef TBSYS_RUNNABLE_H_
#define TBSYS_RUNNABLE_H_

namespace tbsys {

/** 
 * @brief Runnable是一个抽象类，它拥有一个run纯虚方法
 * 主要用于Thread类 (tbsys/src/thread.h中的 CThread)  
 **/
class Runnable {

public:
    /*
     * 析构
     */
    virtual ~Runnable() {
    }
    /**
     * 运行入口函数
     */
    virtual void run(CThread *thread, void *arg) = 0;
};

}





/* 结合类Runnable 和类CThread 我们可以将线程面向对象化并也可以很方便地控制与管理线程，下面是一个小例子*/
#if 0
    #include <stdio.h>  
    #include <tbsys.h>  
    using namespace tbsys;  
    
    /* 用户自己定义的继承于类Runnable 的类 */
    class Myrunable : public Runnable {
        
        /* 用户需要自己实现纯虚函数run */ 
        void run(CThread *thread, void *arg) {  
            printf("This is my thread !\n");  
        }  
    };  
    
    int main() {  
        
        /* 创建自己的runable类 */ 
        Myrunable test;  
        
        /* 创建线程管理实例 */ 
        CThread thread;  
        
        /* 启动线程 */ 
        thread.start(&test, NULL);  
        
        thread.join();  
        
        return 0;  
    }  
#endif


#endif /*RUNNABLE_H_*/
