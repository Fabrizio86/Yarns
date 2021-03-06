//
// Created by Fabrizio Paino on 2020-08-16.
//

#include "Fiber.h"
#include "Scheduler.h"

#include <utility>

namespace YarnBall {

    using namespace std;

    sITask Fiber::stealWork() {
        // if we got less than 2 items, don't steal
        if (this->queueSize() < 3) return nullptr;

        sITask task = this->queue.back();
        this->queue.pop_back();
        return task;
    }

    void Fiber::work() {
        while (this->getState() != State::Aborting) {
            if (this->queue.empty()) {
                Scheduler::instance()->getWork(this);
                this->wait();
            }

            // thread could have been stopped after waiting, lets just exit
            if (this->getState() == State::Aborting) {
                Scheduler::instance()->cleanup(this);
                return;
            }

            sITask task = this->queue.front();
            this->queue.pop_front();

            try {
                if (task != nullptr) {
                    task->run();
                }
            }
            catch (...) {
                task->exception(current_exception());
            }
        }
    }

    void Fiber::addWork(sITask work) {
        this->queue.push_back(work);
    }

    size_t Fiber::queueSize() {
        return this->queue.size();
    }

    void Fiber::clearQueue() {
        this->queue.clear();
    }

    void Fiber::join() {
        if (!this->isDetached())
            this->thread.join();
    }

    ThreadId Fiber::id() {
        return this->thread.get_id();
    }

    void Fiber::stop() {
        // if we have already exited, no need to go further
        if (this->state == State::Aborting) return;

        this->clearQueue();
        this->state = State::Aborting;
        this->condition.notify_one();
    }

    State Fiber::getState() {
        return this->state;
    }

    void Fiber::detach() {
        this->thread.detach();
    }

    Workload Fiber::getWorkload() {
        size_t size = this->queueSize();

        return size >= this->upperLimit ? Workload::Exhausted
                                        : size >= this->queueThreshold && size < this->upperLimit
                                          ? Workload::Taxed : Workload::Normal;
    }

    void Fiber::wait() {
        if (this->state == State::Aborting) return;

        Locker lk(this->mu);

        if (this->temp) {
            bool expired = !this->condition.wait_for(lk, 2s, [this] { return conditional(); });
            if (expired) {
                this->state = State::Aborting;
            }
        } else {
            this->condition.wait(lk, [this] { return conditional(); });
        }
    }

    bool Fiber::conditional() {
        auto empty = this->queueSize() != 0;
        auto aborting = this->getState() == State::Aborting;
        return aborting || empty;
    }

    Fiber::~Fiber() {
        this->stop();
        this->join();
    }

    Fiber::Fiber(uint upperLimit, bool temp) : upperLimit(upperLimit), state(State::Running), temp{temp} {
        this->queueThreshold = this->upperLimit * 0.5;
        this->thread = std::thread(&Fiber::work, this);
    }

    bool Fiber::isDetached() const {
        return !this->thread.joinable();
    }
}