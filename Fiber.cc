#include "Fiber.h"
#include <atomic>
#include <cassert>
#include "Schedule.h"

static Fiber::ptr t_thread_ptr = nullptr;
static Fiber* t_ptr = nullptr;
static thread_local std::atomic<uint64_t> s_fiber_num{0};
static thread_local std::atomic<uint64_t> s_fiber_cnt{0};

Fiber::ptr Fiber::Getthis() {
    if(t_ptr)
        return t_ptr->shared_from_this();
    ptr main_thread(new Fiber);
    t_thread_ptr = main_thread;
    return t_ptr->shared_from_this(); 
}

void Fiber::setthis(Fiber* f) {
    t_ptr = f;
}

Fiber::Fiber() {
    setthis(this);
    m_stacksize = 0;
    getcontext(&m_utx);
    m_id = s_fiber_num++;
    s_fiber_cnt++;
}

Fiber::Fiber(std::function<void()> cb, int stacksize, bool isRunInScheduler) 
: m_id(s_fiber_num++), m_stacksize(stacksize ? stacksize : 128 * 1024), m_cb(cb), m_runInScheduler(isRunInScheduler) 
{
    s_fiber_cnt++;
    m_stack = malloc(m_stacksize);
    getcontext(&m_utx);

    m_utx.uc_link = nullptr;
    m_utx.uc_stack.ss_sp = m_stack;
    m_utx.uc_stack.ss_size =m_stacksize;
    makecontext(&m_utx, &Fiber::MainFunc, 0);

}

Fiber::~Fiber() {
    s_fiber_cnt--;
    if(m_stack) {
        free(m_stack);
    }
    else {
        assert(!m_cb);
        assert(m_state == Running);
        Fiber* cur = t_ptr;
        if(cur == this) setthis(nullptr);
    }
}

void Fiber::resume() {
    assert(m_state != Running && m_state != Term);
    setthis(this);
    m_state = Running;
    if(m_runInScheduler) {
        swapcontext(&Scheduler::GetMainFiber()->m_utx, &m_utx);
    }
    else swapcontext(&t_thread_ptr->m_utx, &m_utx);
}

void Fiber::reset(std::function<void()> cb) {
    assert(m_stack);
    assert(m_state == Term);
    m_cb = cb;
    getcontext(&m_utx);

    m_utx.uc_link          = nullptr;
    m_utx.uc_stack.ss_sp   = m_stack;
    m_utx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_utx, &Fiber::MainFunc, 0);
    m_state = Ready;
}

void Fiber::yield() {
    assert(m_state == Running || m_state == Term);
    setthis(t_thread_ptr.get());
    if(m_state != Term) m_state = Ready;
    if(m_runInScheduler) {
        swapcontext(&m_utx, &Scheduler::GetMainFiber()->m_utx);
    }
    swapcontext(&m_utx, &t_thread_ptr->m_utx);
}
void Fiber::MainFunc() {
    ptr cur = Getthis();
    assert(cur != nullptr);
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = Term;
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->yield();
}