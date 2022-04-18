#include <stdio.h>
#include <math.h>
#include <iostream>
#include <time.h>
#include <string.h>
#include <cstdio>
#include <vector>
#include <pthread.h>
#include <functional>

// int systhr_create(void * (*start_func)(void *), void *arg);
// void assign_cpu(int cpu);


// systhr.c より

int systhr_create(void * (*start_func)(void *), void *arg) {
    int status = 0;
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    status = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    if(status == 0) status = pthread_create(&tid, &attr, start_func, arg);
    if(status != 0) status = pthread_create(&tid, 0,     start_func, arg);
    return status;
}

#if 0

import java.util.function.Consumer;

// タスクはこれを拡張．
// これは何もしないタスクであり．noneで拒否の代わりに使える
class Task0 {
    protected boolean hasResult0 = false;
    public synchronized boolean hasResult() { return hasResult0; }
    // 結果を返す動作があるときは run の中で (以下にはないが Overrideで)
    public void run(WorkerEnv w) { ; /* 何もしない */  }
};

#endif

class WorkerEnv;
class ParEnv{
public:
  static WorkerEnv* pe;
  static void initParallel(int nWorkers);
};


// タスクはこれを拡張．
// これは何もしないタスクであり．noneで拒否の代わりに使える
class Task0 {
public:
  bool hasResult() {
    bool r;
    pthread_mutex_lock(&lk);
    r =  hasResult0;
    pthread_mutex_unlock(&lk);
    return r;
  }
  virtual void run(WorkerEnv* w) { ; /* 何もしない */  }
protected:
  bool hasResult0 = false;
  pthread_mutex_t lk = PTHREAD_MUTEX_INITIALIZER; 
};

#if 0
  
// treq の際に new して victim (候補) の req に渡す
// 中止: 取り戻し用に requesterId はいつ入手 => setTask時
// 取り戻し用に requesterId は public で読めることに
// Task0 (noneの代わり) のときは requesterId 使わない
class TaskBuf extends Exception {
    public int requesterId; // for steal back
    Task0 task = null;
    public TaskBuf(int requesterId) {
        this.requesterId = requesterId;
    }
    synchronized void setTask(Task0 task) {
        this.task = task;
    }
    synchronized Task0 getTask() {
        return task; // なかったら nullが返る
    }
}

#endif

// treq の際に new して victim (候補) の req に渡す
// 取り戻し用に requesterId は public で読めることに
// Task0 (noneの代わり) のときは requesterId 使わない
class TaskBuf {
public: 
  int requesterId;
  TaskBuf(int requesterId) {
    this->requesterId = requesterId;
  }
  void setTask(Task0* task) {
    pthread_mutex_lock(&lk);
    this->task = task;
    pthread_mutex_unlock(&lk);
  }
  Task0* getTask() {
    Task0* task;
    pthread_mutex_lock(&lk);
    task = this->task;
    pthread_mutex_unlock(&lk);
    return task;
  }
private:
  Task0* task = nullptr;
  pthread_mutex_t lk = PTHREAD_MUTEX_INITIALIZER; 
};
  
#if 0

// ワーカ別環境
class WorkerEnv implements Runnable {
    int nWorkers;
    int myId;
    volatile TaskBuf req = null;
    WorkerEnv(int nWorkers, int myId) {
        this.nWorkers = nWorkers;
        this.myId = myId;
    }
    // helpers
    public void run() {
        double r = Math.sqrt(0.5 + myId), q = Math.sqrt(r + myId);
        for (r -= (int)r;
             true;
             r = r * 3.0 + q, r -= (int)r) {
            int victimId = (int)(nWorkers * r);
            if (victimId != myId)
                this.stealRunTask(ParEnv.pe[victimId], 0);
        }
    }
    void stealRunTask(WorkerEnv victim, int why) {
        TaskBuf tBuf = new TaskBuf(myId);
        victim.tReq(this, tBuf);
        Task0 task = null;
        while ((task = tBuf.getTask()) == null) // タスク待ち，拒否しながら
            this.refuseReq();
        task.run(this); // タスク実行，Task0 でなく Overrideされていること多い
    }
    // victim候補(this)のロック獲得して，タスク要求．thiefスレッドで動く
    synchronized void tReq(WorkerEnv thief, TaskBuf tBuf) {
        // いまのところは thief 不使用
        if (req != null)
            tBuf.setTask(new Task0()); // この方法は前と違う．デッドロック心配
        else
            req = tBuf; // task request (tBuf): theif => victim
    }
    // 要求拒否として Task0 を返す
    synchronized void refuseReq() {
        if (req != null) {
            req.setTask(new Task0());
            req = null;
        }
    }
    // タスク要求を正式に(synchronizedしてポートから除いて)受け取る．
    // 普段は volatile の req が nullかで判断する
    synchronized TaskBuf acceptReq() {
        TaskBuf tb = req;
        req = null;
        return tb;
    }
    void waitResult(TaskBuf tb) {
        while (!tb.getTask().hasResult())
            this.stealRunTask(ParEnv.pe[tb.requesterId], 1);
    }
}

#endif

class WorkerEnv {
public:
    TaskBuf* volatile req;
    WorkerEnv(int nWorkers, int myId){
        this->nWorkers = nWorkers;
        this->myId = myId;
    }
    
    void run() {
        double r = sqrt(0.5 + myId), q = sqrt(r + myId);
        for(r -= (int)r ;
            true;
            r = r * 3.0 + q, r -= (int)r){
                int victimId = (int)(nWorkers * r);
                if (victimId != myId){
                    this->stealRunTask(ParEnv::pe[victimId],0);
                }
            } 
    }

    TaskBuf* acceptReq() {
        pthread_mutex_lock(&lk);
        TaskBuf *tb = req;
        req = nullptr;
        pthread_mutex_unlock(&lk);    
        return tb;
    }

    void waitResult(TaskBuf tb) {
        while(!tb.getTask()->hasResult()){
            this->stealRunTask(ParEnv::pe[tb.requesterId],1);
        }
    }
private:
    int nWorkers;
    int myId;

    void stealRunTask(WorkerEnv victim, int why) {
        TaskBuf *tBuf;
        victim.tReq(this, tBuf);
        Task0 *task;
        while((task = tBuf->getTask()) == nullptr)
            this->refuseReq();
        //task->run(this);
    } 

    void tReq(WorkerEnv* victim, TaskBuf* tBuf) {
        pthread_mutex_lock(&lk);
        if(req != nullptr)
            tBuf;
        else
            req = tBuf;
        pthread_mutex_unlock(&lk);
    }

    void refuseReq() {
        pthread_mutex_lock(&lk);
        if(req != nullptr){
            req->setTask(new Task0());
            req = nullptr;
        }
        pthread_mutex_unlock(&lk);
    }

    pthread_mutex_t lk = PTHREAD_MUTEX_INITIALIZER; 
};

#if 0

// 並列実行(複数スレッド)環境
class ParEnv {
    static WorkerEnv[] pe; // 並列に対応する大域的配列．要素はワーカ別環境
    static void initParallel(int nWorkers) {
        pe = new WorkerEnv[nWorkers];
        for (int id = 0; id < nWorkers; id++)
            pe[id] = new WorkerEnv(nWorkers, id);
        // 上記環境できてから複数スレッド開始
        // id 0 は current スレッド
        for (int id = 1; id < nWorkers; id++) {   
            Thread t = new Thread(pe[id]); // helpers
            t.setDaemon(true); t.start();
        }
        // バリア同期はしないことに
    }
}

#endif

void ParEnv::initParallel(int nWorkers) {
    ParEnv::pe = (WorkerEnv *)malloc(sizeof(int) * nWorkers);
    for(int id = 0; id < nWorkers;id++) {
        WorkerEnv *w = new WorkerEnv(nWorkers, id);
        pe[id];
    }
    for(int id = 1; id < nWorkers;id++) {

    }
}

#if 0

// 共通にする
class Fib0 {
    static int fib0(int k) {
        if (k <= 2) return 1;
        return fib0(k - 1) + fib0(k - 2);
    }
}

#endif

class Fib0 {
public:
    static int fib0(int k) {
        if (k <= 2) return 1;
        return fib0(k - 1) + fib0(k - 2);
    }
};

#if 0


// Task0 を拡張して
class TfibL extends Task0 {
    static int thres = 20;
    int r;
    int k;
    public TfibL(int k) { this.k = k; }
    @Override
    public void run(WorkerEnv thief) {
        // ここで do-handle する．ただし，ここのハンドラはタスク生成しない
        // ここでデバッグ用に表示するとよいが書いていない
        // 入れ子関数の場合．
        Consumer<TaskBuf> bk = tb -> {};
        r = fib(thief, bk, this.k);
        setResult(r); // synchronized．
    }
    // thiefが結果をセット．結果の送り先はなく見てもらう
    public synchronized void setResult(int r) {
        this.r = r;
        this.hasResult0 = true;
    }
    // victimが結果をget
    // 先に hasResult()でpresence確認しておくこと（なければ取り戻しを）
    public synchronized int getResult() {
        return this.r;
    }
    // fib: 別クラスにせず，ここに static で書くことに
    static int fib(WorkerEnv w, Consumer<TaskBuf> bk0,  int k) {
        if (k <= thres) return Fib0.fib0(k);
        {
            // ここで do-handleし，fib(k-1)実行中にfib(k-2)をspawn可能に
            // 2002.1.28のCファイルではfib(k-2)実行中分けられてなく古い
            // PPoPP 2009以降なら問題ない
            int r0 = 0, r1 = 0;
            // ここで面倒みる treq (兼TaskBuf)
            final TaskBuf[] tb1 = new TaskBuf[1];
            {
                Consumer<TaskBuf> bk1 = tb -> {
                    if (tb1[0] == null) { // まだ面倒みてない
                        bk0.accept(tb); // hcallの代わり // 呼び出し元で
                        if (tb.getTask() == null) // 呼び出し元で面倒みてない
                            // ここで面倒をみる
                            (tb1[0] = tb).setTask(new TfibL(k - 2));
                    }
                };
                // POLL
                if (w.req != null) {
                    TaskBuf tb = w.acceptReq();
                    bk1.accept(tb); // hcallの代わり
                    if (tb.getTask() == null) // 呼び出し元で面倒みてない
                        // ここで面倒をみる
                        tb.setTask(new Task0());
                }
                {{ r0 = fib(w, bk1, k - 1); }}
            }
            if (tb1[0] == null)
                {{ r1 = fib(w, bk0, k - 2); }}
            else {
                w.waitResult(tb1[0]);
                {{ r1 = ((TfibL)(tb1[0].getTask())).getResult(); }}
            }
            return r0 + r1;
        }
    }
    // main: 別クラスにせず，ここに static で書くことに
    public static void main(String[] arg) {
        int nWorkers = Integer.parseInt(arg[0]);
        int k = Integer.parseInt(arg[1]);
        thres = Integer.parseInt(arg[2]);
        ParEnv.initParallel(nWorkers);
        int r = 0;
        long startTime = System.nanoTime();
        Consumer<TaskBuf> bk = tb -> {};
        r = fib(ParEnv.pe[0], bk, k);
        long endTime = System.nanoTime();
        System.out.println("fib(" + k + ") = " + r);
        System.out.println("time: " + (endTime - startTime) * 1e-9);
    }
}

#endif

class TfibL : public Task0{
public:
    TfibL(int k) {this->k = k;}

    void setResult(int r) {
        pthread_mutex_lock(&lk);
        this->r = r;
        this->hasResult0 = true;
        pthread_mutex_unlock(&lk);
    }

    int getResult() {
        pthread_mutex_lock(&lk);
        int x = this->r;
        pthread_mutex_unlock(&lk);
        return x;
    }

    static void main(int argc, char const *argv[]) {
        int nWorkers = atoi(argv[0]);
        int k = atoi(argv[1]);
        //thres = atoi(argv[2]);
        ParEnv::initParallel(nWorkers);
        int r = 0;
        clock_t startTime = clock();

        clock_t endTime = clock();
        double time = (endTime - startTime) / CLOCKS_PER_SEC * 1000.0;
        printf("fib(%d) = %d",k,r);
        printf("time: %lf",time);
    }

private:
    static const int thres = 20;
    int k;
    int r;
    static int fib(WorkerEnv w, std::function<TaskBuf(TaskBuf)> bk0, int k) {
        if (k <= thres) { return Fib0::fib0(k); }
        {
            int r0 = 0, r1 = 0;
        }
    }

    pthread_mutex_t lk = PTHREAD_MUTEX_INITIALIZER; 
};

#if 0

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

#endif

/*
class TfibSC : public Task0{
    static class Frame_FIB : public Frame{

    }

};
*/

