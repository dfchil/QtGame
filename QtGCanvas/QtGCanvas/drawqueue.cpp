#include "../qtgcanvas.h"
#include <QDebug>
#include "../qtgdrawer.h"

#define QPARTSIZEBITS 10
#define PMASK ((1 << QPARTSIZEBITS)-1)
#define QSUBQS 512

QtgDrawQueue::QtgDrawQueue(size_t item_size)
{
    this->item_size = item_size;
    this->sub_queues = (draw_base_t**)calloc(sizeof(draw_base_t*), QSUBQS);

    this->qpos = 0;
    this->part_mask = ((1 << QPARTSIZEBITS)-1);
}

static inline draw_base_t* _lookup(draw_base_t** queues, int qpos, size_t item_size){
    draw_base_t* tbl = queues[qpos >> QPARTSIZEBITS];
    return (draw_base_t*) ((size_t)tbl + (PMASK & qpos)*item_size);
}

void QtgDrawQueue::lock()
{
    if (!this->mutex.tryLock()){
        qDebug() << "Drawqueue lock contention!";
    }
}
void QtgDrawQueue::unlock()

{
    this->mutex.unlock();
}

draw_base_t* QtgDrawQueue::next_item(){
    this->next_lock.lock();
    // test to see if we need to grow the allocation size
    if ((this->qpos & this->part_mask) == 0){
        this->grow();
    }
    draw_base_t*  outp = _lookup(this->sub_queues, this->qpos++, this->item_size);
    this->next_lock.unlock();
    return  outp;
}

void QtgDrawQueue::draw_frame(){
    this->lock();
    for (int dp = 0 ; dp < qpos;){
        for (int cp = 0; dp < qpos  && cp <= PMASK ; cp++, dp++){
            draw_base_t* ti = _lookup(this->sub_queues, dp, this->item_size);
            ti->drawer->draw(ti);
        }
    }
    qpos = 0;
    this->unlock();
}

void QtgDrawQueue::grow()
{
    this->sub_queues[qpos >> QPARTSIZEBITS] = (draw_base_t*)calloc(sizeof(draw_base_t), 1 << QPARTSIZEBITS);
}
