#include <stdio.h>
#include <math.h>
#include <iostream>
#include <time.h>
#include <string.h>
#include <cstdio>
#include <vector>
#include <thread>
#include <pthread.h>

int systhr_create(void * (*start_func)(void *), void *arg);
void assign_cpu(int cpu);

int systhr_create(void * (*start_func)(void *), void *arg){
    int status = 0;
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    status = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    if(status == 0) status = pthread_create(&tid, &attr, start_func, arg);
    if(status != 0) status = pthread_create(&tid, 0,     start_func, arg);
    return status;
}

//タスクをこれを拡張
//これは何もしないタスクであり、noneで拒否の代わりに使える
class Task0{
    protected: bool hasResult0 = false;
    public: bool hasResult() {return hasResult0;}

};

// treq の際に new して victim（候補）の req に渡す
// 中止: 取り戻し用に requestedId いつ入手 => setTask時
class TaskBuf{

    public: Task0* task = nullptr;
    public: void setTask(Task0* task){
        this->task = task;
    }
    public: Task0* getTask(){
        return task;
    }
    public: int requestedId;
    public: TaskBuf(int requestedId){
            this->requestedId = requestedId;
    }
        
};

// ワーカ別環境
class WorkerEnv : Runnable{
    public: int nWorkers;
    public: int myId;
    public: TaskBuf *req = nullptr;
    WorkerEnv(int nWorkers, int myId){
        this->nWorkers = nWorkers;
        this->myId = myId;
    }
    // helpers

    void stealRunTask(WorkerEnv victim, int why){
        TaskBuf *tBuf = new TaskBuf(myId);
        victim.tReq(this, tBuf);
        Task0 *task = nullptr;
        while((task = tBuf->getTask()) == nullptr)
            this->refuseReq();
        task->run(this);     //タスクを実行

    }

    // victim候補(this)のロック獲得して，タスク要求．thiefスレッドで動く
    void tReq(WorkerEnv* thief, TaskBuf* tBuf){   //syncronizeを書く
        if(req != nullptr)
            tBuf->setTask(new Task0());
        else
            req = tBuf;
    }

    // 要求拒否として Task0 を返す
    void refuseReq(){       //syncronizeを書く
        if(req != NULL){
            Task0 *task0 = new Task0();
            req->setTask(task0);
            req = NULL;
        }
    }

    // タスク要求を正式に(syncronizedしてポートから除いて)受け取る.
    // 普段 volatile の req が nullか判断する。
    public: TaskBuf* acceptReq(){    //syncronizeを書く
        TaskBuf *tb = req;
        req = nullptr;
        return tb;
    }
    public: void waitResult(TaskBuf* tb){
        while(!tb->getTask().hasResult())
            this->stealRunTask(ParEnv::pe[tb->requestedId],1);
    }
    
    public: void run(){
            double r = sqrt(0.5 + myId);
            double q = sqrt(r + myId);
            for(r -= (int)r;
                true;
                r = r * 3.0 + q, r -= (int)r){
                int victimId = (int)(nWorkers * r);
                if(victimId != myId)
                    this->stealRunTask(ParEnv::pe[victimId],0);
            }
    }
};

class ParEnv{
    //static WorkerEnv pe[];
    public: static std::vector<WorkerEnv> pe;
    public: static void initParallel(int nWorkers){
        pe.resize(nWorkers);
        for (int id = 0; id < nWorkers; id++)
            pe[id] = new WorkerEnv(nWorkers, id);
            // 上記は環境ができてから複数スレッド開始
            // id 0 は current スレッド
        for (int id = 1; id < nWorkers; id++){
            std::thread t(pe[id]); 
        }
    }
};

// 共通にする
class Fib0{
    public: static int fib0(int k){
        if(k <= 2) return 1;
        return fib0(k - 1) + fib0(k - 2);
    }
};

// Task0 を拡張して
class Tfib : public Task0{
    static int thres = 20;
    int r;
    int k;

    public: Tfib(int k) { this->k = k;}
        
    public: void run(WorkerEnv thief){
            // ここで、do-handle する。ただし、ここのハンドラはタスク生成しない
            // ここで、デバッグ用に表示すると良いが書いてない
                int r = 0;
                try{
                    r = fib(thief,k);
                }catch (TaskBuf tb){
                    ;
                }
                setResult(r); // synchronized
        }
        // theifが結果をセット、結果の送り先はなく見てもらう
    public: void setResult(int r){  // synchronizedをつける必要がある
                this->r = r;
                hasResult0 = true;
            }
        // vicitm が結果を get
        // 先に hasResult()で presence 確認しておくこと（なければ取り戻し）
    public: int getResult(){    // synchronizedをつける必要がある
                return this->r;
            }

    public: static void main(std::string arg[]){
            int nWorkers = stoi(arg[0]);
            int k = stoi(arg[1]);
            thres = stoi(arg[2]);
            ParEnv::initParallel(nWorkers);
            int r = 0;
            long long startTime; // nanotime
            try{
                r = fib(ParEnv::pe[],k);
            }catch (TaskBuf tb){
                ;
            }
            long long endTime; // nanotime
            printf("fib");
            printf("time: ");
            }
    
    static int fib(WorkerEnv w, int k) throw TaskBuf{
        if (k <= thres) return Fib0::fib0(k);
        {
            //ここで、do-handleし、fib(k-1)実行中にfib(k-2)をspawn可能に
            // 
            // PPoPP 2009以降なら問題ない
            int r0 = 0, r1 = 0;
            TaskBuf *tb1 = nullptr; //ここで面倒見る
            try{
                //POLL
                if (w.req != NULL){
                    TaskBuf *tb = w.acceptReq();
                    if (true) throw tb; //hcallの代わり
                    if (tb->getTask() == nullptr) //
                        tb->setTask(new Task0());
                }
                {{ r0 = fib(w,k - 1);}}
            }catch (TaskBuf* tb){
                if (tb1 == nullptr){
                    if (true) throw tb;  //hcallの代わり // 呼び出し元で
                    if (tb->getTask() == NULL) // 呼び出し元で面倒みてない
                        (tb = tb1)->setTask(new Tfib(k -2));
                }
            }
            if (tb1 == NULL)
                {{ r1 = fib(w, k - 2);}}
            else {
                w.waitResult(tb1);
                {{ r1 = ((Tfib)(tb1->getTask())).getResult();}}
            }
            return r0 + r1;
        }
    }
};

class TfibL : public Task0{
    static int thres = 20;
    int r;
    int k;

    public: TfibL(int k) {this->k = k;}

    public: void run(WorkerEnv* thief) override{
            //ここでdo-handleする。ただし、ここのハンドラはタスク生成しない
            //入れ子関数の場合

    }

    // theifが結果をセット．結果の送り先はなくみてもらう
    public: void setResult(int r){  //syncronizeをかく
            this->r = r;
            this->hasResult0 = true;
    }

        // victimが結果をget
        // 先にhasResult()でpresence確認しておく
    public: int getResult(){    //syncornizeをかく
            return this->r;
    }

        // fib: 別クラスにせず、ここにstaticでかく
    
    static int fib(WorkerEnv w, int k){
        if (k <= thres) return Fib0::fib0(k);
        {
            int r0 = 0, r1 = 0;
            std::vector<TaskBuf*> tb1(1);
            {
                //ラムダ式の処理

                TaskBuf* bk1 = [](std::vector<TaskBuf*> tb){
                    if (tb1.at(0) == nullptr){   //まだ面倒をみてない
                        
                        if (tb->getTask() == nullptr) //呼び出し元で面倒みてない
                            //ここで面倒を見る
                            (tb1[0] = tb)->setTask(new TfibL(k - 2));
                    }
                };

                if (w.req != nullptr){
                    TaskBuf* tb = w.acceptReq();
                    bk1.accept(tb);
                    if (tb->getTask() == nullptr){
                        tb->setTask(new Task0());
                    }
                }
                {{ r0 = fib(w, bk1, k - 1);}}
            }
            if (tb[0] == nullptr){
                {{ r1 = fib(w, bk0, k - 2);}}
            }else {
                w.waitResult(tb1[0]);
                {{ r1 = ((Tfib)(tb1[0]->getTask())).getResult(); }}
            }

        }
    }

    public: static void main(std::string arg[]){
        int nWorkers = stoi(arg[0]);
        int k = stoi(arg[1]);
        thres = stoi(arg[2]);
        ParEnv::initParallel(nWorkers);
        int r = 0;
        //long long StartTime = 
        r = fib(ParEnv::pe[0], k);
    }
};

/*
class TfibSC : public Task0{
    static class Frame_FIB : public Frame{

    }

};
*/

