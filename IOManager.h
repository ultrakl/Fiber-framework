#ifndef __IO_MANAGER__H__
#define __IO_MANAGER__H__

#include <shared_mutex>
#include "Schedule.h"
#include "Timer.h"
class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef std::shared_lock<std::shared_mutex> readLock;
    typedef std::unique_lock<std::shared_mutex> writeLock;
    /**
     * @brief IO事件，模仿epoll对于事件的定义
     * @details 只关心socket fd读写事件，其余事件都被归类为读写事件
     */
    enum  Event{
        None = 0x0,
        Read = 0x1,
        Write = 0x4
    };
private:
    /**
     * @brief socket fd上下文类, 包括fd的值，fd上的事件，以及fd的读写事件上下文
     */
    struct FdContext {
        /**
         * @brief fd的每个事件都有事件上下文，保存事件的回调函数以及执行回调函数的调度器
         */
        struct EventContext{
            Scheduler* sceuler = nullptr;
            Fiber::ptr fiber;
            std::function<void()> cb;
        };
        /**
         * @brief 获取事件上下文
         * @param[in] event 事件类型 
         * @return  返回对应事件的上下文
         */
        EventContext& getEventContext(EventContext& event);
        /**
         * @brief 重置事件上下文
         * @param[in, out] ctx 待重置的事件上下文对象 
         */
        void resetEventContext(EventContext& ctx);
        /**
         * @brief 触发事件
         * @details 根据事件类型调用对应上下文结构中的调度器去回调协程或者回调函数
         * @param[in] event 事件类型
         */
        void triggerEvent(Event event);

        /// 读事件上下文
        EventContext read;
        /// 写事件上下文
        EventContext write;
        /// 事件关联的句柄
        int fd = 0;
        // 该fd添加的事件
        Event events = None;
        std::mutex mutex;
    };

public: 
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否将调用线程包含进去
     * @param[in] name 调度器的名称
     */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "IOManager");

    ~IOManager();

    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);
    bool cancelAll(int fd);
    static IOManager* Getthis();
protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    bool stopping(uint64_t& timeout);
    void onTimerInsertedAtFront() override;
private:
    int m_epfd = 0;
    int tickleFds[2];
    std::atomic<size_t> m_pendingEventCount = {0};
    std::vector<FdContext*> m_fdContexts;
    mutable std::shared_mutex m_mutex;
 };

#endif