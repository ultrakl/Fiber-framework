#include <bits/stdc++.h>
using namespace std;

class ThreadPool {
private:
    int m_threads;
    mutex m_tx;
    vector<thread> threads;
    // template<typename T>
    queue<function<void()>> m_work_queue;
    condition_variable m_cv;
    bool is_stop;
public:
    ThreadPool(int threadsize) : m_threads(threadsize) {
        for(int i = 0; i < threadsize; i++) {
            threads.emplace_back([this](){
                while(true) {
                    function<void()> func;
                    {
                        unique_lock<mutex> uq(m_tx);
                        m_cv.wait(uq, [this](){return is_stop || !m_work_queue.empty();});
                        if(is_stop && m_work_queue.empty()) return;
                        func = ::move(m_work_queue.front());
                        m_work_queue.pop();
                    }
                    func();
                }
            });
        }
    }
    template <typename Func, typename... Args> 
    decltype(auto) enqueue(Func func, Args&&... args) {
        // m_work_queue.push();
        using return_type = invoke_result_t<Func, Args...>;
        auto task = make_shared<packaged_task<return_type()>>([func, args...]() -> return_type {
            return func(args...);
        });
        future<return_type> res = task->get_future();
        {
            unique_lock<mutex> uq(m_tx);
            if(is_stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            m_work_queue.emplace([task](){
                (*task)();
            });
        }
        m_cv.notify_all();
        return res;
    }
    ~ThreadPool() {
        {
            unique_lock<mutex> uq(m_tx);
            is_stop = true;
        }
        m_cv.notify_all();
        for(auto& i : threads) i.join();
    }
};