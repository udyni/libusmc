/***************************************************//**
 * @file    usmc_mutex.h
 * @date    May 2020
 * @author  Michele Devetta
 *
 * A mutex/mutex lock classes based on pthread_mutex and inspired by omnithread library
 *
 * LICENSE:
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *******************************************************/
 
#ifndef USMC_MUTEX_H
#define USMC_MUTEX_H

#include <pthread.h>


class USMC_mutex {
public:
    // Constructor and destructor
    USMC_mutex();
    ~USMC_mutex();

    // Acquire and release methods
    void acquire();
    void release();

private:
    pthread_mutex_t _mutex;
};


class USMC_lock {
public:
    // Constructor and destructor
    USMC_lock(USMC_mutex* mutex);
    ~USMC_lock();

private:
    USMC_mutex* _mutex;
};

#endif
