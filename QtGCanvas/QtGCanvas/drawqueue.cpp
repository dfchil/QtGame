#include "../qtgcanvas.h"
#include <QDebug>

#include "../qtgdrawer.h"

#define QPARTSIZEBITS 10
#define PMASK ((1 << QPARTSIZEBITS)-1)
#define QSUBQS 512

DrawQueue::DrawQueue(QObject *parent) :
    QObject(parent)
{

    this->item_size = -1;
    this->sub_queues = (draw_base_t**)calloc(sizeof(draw_base_t*), QSUBQS);

    this->qpos = 0;
    this->part_mask = ((1 << QPARTSIZEBITS)-1);
}

static inline draw_base_t* _lookup(draw_base_t** queues, int qpos, size_t item_size){
    draw_base_t* tbl = queues[qpos >> QPARTSIZEBITS];
    return (draw_base_t*) ((size_t)tbl + (PMASK & qpos)*item_size);
}


void DrawQueue::lock()
{
//    if (!this->lock.fastTryLock()){
//        qDebug() << "Drawqueue lock contention!";
//    }
    this->mutex.lock();
}

void DrawQueue::unlock()
{
    this->mutex.unlock();
}

draw_base_t* DrawQueue::next_item(){
    // test to see if we need to grow the allocation size
    if ((this->qpos & this->part_mask) == 0){
        this->grow();
    }
    return _lookup(this->sub_queues, this->qpos++, this->item_size);
}

void DrawQueue::draw()
{
    for (int dp = 0 ; dp < qpos;){
        for (int cp = 0; dp < qpos  && cp <= PMASK ; cp++, dp++){
            draw_base_t* ti = _lookup(this->sub_queues, dp, this->item_size);
            ti->drawer->draw(ti);
        }
    }
    qpos = 0;
}

void DrawQueue::set_draw_item_size(size_t size)
{
    this->item_size = size;
}

void DrawQueue::grow()
{
    this->sub_queues[qpos >> QPARTSIZEBITS] = (draw_base_t*)calloc(sizeof(draw_base_t), 1 << QPARTSIZEBITS);
}
