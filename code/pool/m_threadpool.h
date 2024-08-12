#ifndef M_THREADPOOL
#define M_THREADPOOL

#include<mutex>
#include<condition_variable>
#include<queue>
#include<thread>
#include<functional>
#include<assert.h>
class ThreadPool{
public:
    explicit ThreadPool(size_t threadCount=8):pool_(std::make_shared<Pool>()){
        assert(threadCount>0);
        for(size_t i=0;i<threadCount;i++){
            /*
                实际上是在创建一个线程时，
                使用一个初始化捕获列表（initialization capture list）。
                这种语法允许你为线程函数中的某个变量提供一个初始值，
                这个初始值可以在lambda表达式中使用。
            */
            std::thread([pool=pool_]{
                std::unique_lock<std::mutex>locker(pool->mtx);
                while(true){
                    if(!pool->m_tasks.empty()){
                        auto task=std::move(pool->m_tasks.front());
                        pool->m_tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if(pool->isClosed) break;
                    else pool->m_cond.wait(locker);
                }
            }).detach();
            //从 thread 对象分离执行线程，允许执行独立地持续。
            //一旦该线程退出，则释放任何分配的资源。
        }
    }


    ThreadPool()=default;
    ThreadPool(ThreadPool&&)=default;
    ~ThreadPool(){
            if(static_cast<bool>(pool_)){
                {
                    std::lock_guard<std::mutex> locker(pool_->mtx);
                    pool_->isClosed=true;
                }
                pool_->m_cond.notify_all();
            }
    }


    template<class F>
    void AddTask(F&& task){
        {
            std::lock_guard<std::mutex>locker(pool_->mtx);
            pool_->m_tasks.emplace(std::forward<F>(task));
            //使用std::forward保留了值原来的属性，实现完美转发
            // emplace是原地构造一个元素
        }
        pool_->m_cond.notify_one();
    }
private:
    struct Pool{
        std::mutex mtx;
        std::condition_variable m_cond;
        bool isClosed;
        std::queue<std::function<void()>>m_tasks;
        // return-type:void
    };
    std::shared_ptr<Pool>pool_;
};

#endif // M_THREADPOOL