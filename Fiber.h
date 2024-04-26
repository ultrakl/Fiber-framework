#ifndef  __Fiber__H
#define  __Fiber__H
#include <functional>
#include <iostream>
#include <memory>
#include <ucontext.h> // Include the ucontext.h header file

class Fiber : std::enable_shared_from_this<Fiber>{
public:
    using ptr = std::shared_ptr<Fiber>;
    static ptr Getthis();
    static void setthis(Fiber*);
    static void MainFunc();
    Fiber();
    Fiber(std::function<void()>, int stacksize = 0, bool isRunInScheduler = false); 
    ~Fiber();
    void resume();
    void reset(std::function<void()>);
    void yield();
    enum State {
        Ready = 0,
        Running,
        Term
    };
    State getState() const{return m_state;}
private:
    State m_state = Ready;
    ucontext_t m_utx;
    int m_id;
    int m_stacksize;
    void* m_stack = nullptr;
    std::function<void()> m_cb;
    bool m_runInScheduler;
    // State m_state = READY;
};


#endif
