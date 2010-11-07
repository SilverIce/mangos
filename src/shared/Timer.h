/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MANGOS_TIMER_H
#define MANGOS_TIMER_H

#include "Platform/CompilerDefs.h"
#include <ace/OS_NS_sys_time.h>

inline uint32 getMSTime()
{
    static const ACE_Time_Value ApplicationStartTime = ACE_OS::gettimeofday();
    return (ACE_OS::gettimeofday() - ApplicationStartTime).msec();
}

inline uint32 getMSTimeDiff(uint32 oldMSTime, uint32 newMSTime)
{
    // getMSTime() have limited data range and this is case when it overflow in this tick
    if (oldMSTime > newMSTime)
        return (0xFFFFFFFF - oldMSTime) + newMSTime;
    else
        return newMSTime - oldMSTime;
}

class IntervalTimer
{
    public:
        IntervalTimer() : _interval(0), _current(0) {}

        void Update(time_t diff) { _current += diff; if(_current<0) _current=0;}
        bool Passed() { return _current >= _interval; }
        void Reset() { if(_current >= _interval) _current -= _interval;  }

        void SetCurrent(time_t current) { _current = current; }
        void SetInterval(time_t interval) { _interval = interval; }
        time_t GetInterval() const { return _interval; }
        time_t GetCurrent() const { return _current; }

    private:
        time_t _interval;
        time_t _current;
};

struct TimeTracker
{
    TimeTracker(time_t expiry) : i_expiryTime(expiry) {}
    void Update(time_t diff) { i_expiryTime -= diff; }
    bool Passed(void) const { return (i_expiryTime <= 0); }
    void Reset(time_t interval) { i_expiryTime = interval; }
    time_t GetExpiry(void) const { return i_expiryTime; }
    time_t i_expiryTime;
};

struct TimeTrackerSmall
{
    TimeTrackerSmall(int32 expiry) : i_expiryTime(expiry) {}
    void Update(int32 diff) { i_expiryTime -= diff; }
    bool Passed(void) const { return (i_expiryTime <= 0); }
    void Reset(int32 interval) { i_expiryTime = interval; }
    int32 GetExpiry(void) const { return i_expiryTime; }
    int32 i_expiryTime;
};

struct PeriodicTimer
{
    PeriodicTimer(int32 period, int32 start_time) :
        i_expirity(start_time), i_period(period) {}

    bool Update(const uint32 &diff)
    {
        if((i_expirity -= diff) > 0)
            return false;

        i_expirity += i_period > diff ? i_period : diff;
        return true;
    }

    void SetPeriodic(int32 period, int32 start_time)
    {
        i_expirity=start_time, i_period=period;
    }

    // Tracker interface
    PeriodicTimer(int32 start_time) :
        i_expirity(start_time), i_period(start_time) {}

    void TUpdate(uint32 diff) { i_expirity -= diff; }
    bool TPassed() const { return i_expirity <= 0; }
    void TReset(uint32 diff, uint32 period)  { i_expirity += period > diff ? period : diff; }

    int32 i_period;
    int32 i_expirity;
};

#endif
