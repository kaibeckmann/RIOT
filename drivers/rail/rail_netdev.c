
#include "net/netdev2.h"


#include "rail_drv.h"
#include "rail_netdev.h"
#include "rail_internal.h"
#include "rail_registers.h"


#define ENABLE_DEBUG (1)
#include "debug.h"

// local declaration of driver methodes
static int _send(netdev2_t* netdev, const struct iovec* vector, unsigned count);
static int _recv(netdev2_t* netdev, void* buf, size_t len, void* info);
static int _init(netdev2_t* netdev);
static void _isr(netdev2_t* netdev);
static int _get(netdev2_t* netdev, netopt_t opt, void* val, size_t max_len);
static int _set(netdev2_t* netdev, netopt_t opt, void* val, size_t len);


const netdev2_driver_t rail_driver = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = _set,
};


static void _lowlevel_irq_handler(void* arg)
{
    netdev2_t* dev = (netdev2_t*) arg;

    // todo where to set what happend?

    // delegate irq handling to device thread
    if (dev->event_callback) {
        dev->event_callback(dev, NETDEV2_EVENT_ISR);
    }
}

static int _init(netdev2_t* netdev) {

    void (*foo)(void*) = _lowlevel_irq_handler;
    
    foo =foo;
    
    DEBUG("rail_netdev->init called\n");

    return 0;
}


static int _send(netdev2_t* netdev, const struct iovec* vector, unsigned count) 
{
    DEBUG("rail_netdev->send called\b");
    return 0;
}

static int _recv(netdev2_t* netdev, void* buf, size_t len, void* info)
{
    DEBUG("rail_netdev->recv called\b");
    return 0;
}

static void _isr(netdev2_t* netdev) 
{
   DEBUG("rail_netdev->isr called\n");
}
static int _get(netdev2_t* netdev, netopt_t opt, void* val, size_t max_len) 
{
    DEBUG("rail_netdev->get called \n");
    return 0;
}
static int _set(netdev2_t* netdev, netopt_t opt, void* val, size_t len)
{
    DEBUG("rail_netdev->set called\n");
    return 0;
}
