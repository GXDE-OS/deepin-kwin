// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "recordeventmonitor.h"

#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include <X11/extensions/record.h>

#define TOUCHDOWN      (LASTEvent + 1)
#define TOUCHMOTION    (LASTEvent + 2)
#define TOUCHUP        (LASTEvent + 3)

int m_eventTime = 0;
XRecordContext m_context;
Display *m_d0;

static void handleRecordEvent(XRecordInterceptData *data, RecordEventMonitor *p) {
    if (data->category == XRecordFromServer) {
        xEvent * event = (xEvent *)data->data;
        switch (event->u.u.type) {
        case TOUCHDOWN:
            // sometimes, xrecord extend will get repeated touch down event(maybe send to the real client).
            // but, those touch down event is belong to same ancestors.
            // so, if you need, you can use (event->u.keyButtonPointer.time)
            // to filter out the repeated touh down event.
            if (m_eventTime != event->u.keyButtonPointer.time) {
                Q_EMIT p->touchDown();
                m_eventTime = event->u.keyButtonPointer.time;
            }
            break;
        case TOUCHMOTION:
            Q_EMIT p->touchMotion();
            break;
        case TOUCHUP:
            Q_EMIT p->touchUp();
            break;
        default:
            break;
        }
    }

    fflush(stdout);
    XRecordFreeData(data);
}

static void callback(XPointer trash, XRecordInterceptData *data) {
    handleRecordEvent(data, RecordEventMonitor::instance());
}

RecordEventMonitor::RecordEventMonitor(QObject *parent) : QThread(parent)
{

}

RecordEventMonitor *RecordEventMonitor::m_pRecordEventMonitor = nullptr;
RecordEventMonitor *RecordEventMonitor::instance()
{
    if (m_pRecordEventMonitor == nullptr) {
        m_pRecordEventMonitor = new RecordEventMonitor;
    }
    return m_pRecordEventMonitor;
}

void RecordEventMonitor::run()
{
    Display *pDisplay = XOpenDisplay(nullptr);
    m_d0 = pDisplay;
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
    m_context = context;
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

void RecordEventMonitor::stopRecord()
{
    XRecordDisableContext(m_d0, m_context);
    XRecordFreeContext(m_d0, m_context);
    XSync(m_d0, True);
    XCloseDisplay(m_d0);
}
