#include "recordeventmonitor.h"
#include <X11/Xlibint.h>

# define TOUCHDOWN      (LASTEvent + 1)
# define TOUCHMOTION    (LASTEvent + 2)
# define TOUCHUP        (LASTEvent + 3)

RecordEventMonitor::RecordEventMonitor(QObject *parent) : QThread(parent)
{

}

void RecordEventMonitor::run()
{
    Display *pDisplay = XOpenDisplay(nullptr);
    if (pDisplay == nullptr)
        return;

    XRecordClientSpec clients = XRecordAllClients;
    XRecordRange *pRange = XRecordAllocRange();
    if (pRange == nullptr)
        return;

    // In this, we just get touch event in a range.
    memset(pRange, 0, sizeof(XRecordRange));
    pRange->device_events.first = TOUCHDOWN;
    pRange->device_events.last  = TOUCHUP;

    XRecordContext context = XRecordCreateContext(pDisplay, 0, &clients, 1, &pRange, 1);
    if (context == 0)
        return;

    XFree(pRange);
    XSync(pDisplay, True);

    Display* displayDatalink = XOpenDisplay(nullptr);
    if (displayDatalink == nullptr)
        return;

    if (!XRecordEnableContext(displayDatalink, context,  callback, (XPointer) this))
        return;
}

void RecordEventMonitor::callback(XPointer ptr, XRecordInterceptData *data)
{
    ((RecordEventMonitor *) ptr)->handleRecordEvent(data);
}

void RecordEventMonitor::handleRecordEvent(XRecordInterceptData* data)
{
    if (data->category == XRecordFromServer) {
        xEvent * event = (xEvent *)data->data;
        switch (event->u.u.type) {
        case TOUCHDOWN:
            // sometimes, xrecord extend will get repeated touch down event(maybe send to the real client).
            // but, those touch down event is belong to same ancestors.
            // so, if you need, you can use (event->u.keyButtonPointer.time)
            // to filter out the repeated touh down event.
            emit touchDown();
            break;
        case TOUCHMOTION:
            emit touchMotion();
            break;
        case TOUCHUP:
            emit touchUp();
            break;
        default:
            break;
        }
    }

    fflush(stdout);
    XRecordFreeData(data);
}