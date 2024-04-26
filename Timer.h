#ifndef __TIMER_H__
#define __TIMER_H__

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <set>

class TimerManager;

class Timer: public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    bool cancel();
    bool refresh();
    /**
     * @brief 重置定时器时间
     * @param[in] ms 定时器执行间隔时间(毫秒) 
     * @param[in] from_now 是否从当前时间开始计算
     * @return  
     */
    bool reset(uint64_t ms, bool from_now);
private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
    Timer(uint64_t next);
private:
    /// 是否循环定时器
    bool m_recurring = false;
    /// 执行周期
    uint64_t m_ms = 0;
    /// 精确的执行时间, 即超时后的绝对时间
    uint64_t m_next = 0;
    std::function<void()> m_cb;
    TimerManager* m_manager  = nullptr;
private:
    struct Comparator{
        bool operator() (const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

class TimerManager {
friend class Timer;
public:
    TimerManager();
    virtual ~TimerManager();
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond,
                                bool recurring = false);
    uint64_t getNextTimer();
    void listExpiredCb(std::vector<std::function<void()>>& cbs);
    bool hasTimer();
protected:
    /**
     * @brief 当有新定时器插入到定时器的首部时执行该函数
     */
    virtual void onTimerInsertedAtFront() = 0;
    void addTimer(Timer::ptr val, std::unique_lock<std::shared_mutex>&);
private:
    bool detectClockRollover(uint64_t now_ms);
private:
    std::shared_mutex m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    /// 是否触发onTimerInsertAtFront
    bool m_tickled = false;
    /// 上次执行时间
    uint64_t m_previouseTime = 0;
};

#endif