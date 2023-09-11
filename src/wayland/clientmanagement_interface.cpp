/********************************************************************
Copyright 2020  wugang <wugang@uniontech.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "clientmanagement_interface.h"
#include "display.h"
#include "surface_interface.h"
#include "utils.h"
#include "shmclientbuffer.h"

#include <qwayland-server-wayland.h>
#include "qwayland-server-com-deepin-client-management.h"

#define MAX_WINDOWS 100

namespace KWaylandServer
{

static const quint32 s_version = 1;

class ClientManagementInterfacePrivate: public QtWaylandServer::com_deepin_client_management
{
public:

    ClientManagementInterfacePrivate(ClientManagementInterface *q, Display *d);
    ClientManagementInterface *q;

    void updateWindowStates();
    void getWindowStates();
    void sendWindowStates(wl_resource *resource);
    void sendWindowCaption(int windowId, bool succeed, wl_resource *buffer);
    void sendWindowFromPoint(uint32_t wid);
    void getWindowFromPoint();
    void showSplitMenu(const QRect &botton_rect, uint32_t wid);
    void hideSplitMenu(bool delay);

    ClientManagementInterface::WindowState m_windowStates[MAX_WINDOWS];
    uint32_t m_windowCount;

protected:
    void com_deepin_client_management_get_window_states(Resource *resource) override;
    void com_deepin_client_management_get_window_from_point(Resource *resource) override;
    void com_deepin_client_management_show_split_menu(Resource *resource,
            int32_t x, int32_t y, int32_t width, int32_t height, uint32_t wid) override;
    void com_deepin_client_management_hide_split_menu(Resource *resource, int32_t delay) override;
private:
    QString m_splitUuid;
    int     m_splitable = 0;
};

ClientManagementInterfacePrivate::ClientManagementInterfacePrivate(ClientManagementInterface *q, Display *d)
    : QtWaylandServer::com_deepin_client_management(*d, s_version)
    , q(q)
{
}

void ClientManagementInterfacePrivate::com_deepin_client_management_get_window_states(Resource *resource)
{
    Q_UNUSED(resource);

    getWindowStates();
}

void ClientManagementInterfacePrivate::com_deepin_client_management_get_window_from_point(Resource *resource)
{
    Q_UNUSED(resource);

    getWindowFromPoint();
}

void ClientManagementInterfacePrivate::com_deepin_client_management_show_split_menu(Resource *resource,
        int32_t x, int32_t y, int32_t width, int32_t height, uint32_t wid)
{
    Q_UNUSED(resource);

    QRect botton_rect = QRect(x, y, width, height);
    if (botton_rect.isValid())
        showSplitMenu(botton_rect, wid);
}

void ClientManagementInterfacePrivate::com_deepin_client_management_hide_split_menu(Resource *resource, int32_t delay)
{
    Q_UNUSED(resource);

    hideSplitMenu(delay);
}

void ClientManagementInterfacePrivate::getWindowStates()
{
    Q_EMIT q->windowStatesRequest();
}

void ClientManagementInterfacePrivate::getWindowFromPoint()
{
    Q_EMIT q->windowFromPointRequest();
}

void ClientManagementInterfacePrivate::showSplitMenu(const QRect &botton_rect, uint32_t wid)
{
    Q_EMIT q->showSplitMenuRequest(botton_rect, wid);
}

void ClientManagementInterfacePrivate::hideSplitMenu(bool delay)
{
    Q_EMIT q->hideSplitMenuRequest(delay);
}

void ClientManagementInterfacePrivate::sendWindowStates(wl_resource *resource)
{
    struct wl_array data;
    auto fillArray = [this](const ClientManagementInterface::WindowState *origin, wl_array *dest) {
        wl_array_init(dest);
        const size_t memLength = sizeof(struct ClientManagementInterface::WindowState) * m_windowCount;
        void *s = wl_array_add(dest, memLength);
        memcpy(s, origin, memLength);
    };
    fillArray(m_windowStates, &data);
    com_deepin_client_management_send_window_states(resource, m_windowCount, &data);
    wl_array_release(&data);
}

void ClientManagementInterfacePrivate::updateWindowStates()
{
    const auto clientResources = resourceMap();
    for (Resource *resource : clientResources) {
        sendWindowStates(resource->handle);
    }
}

void ClientManagementInterfacePrivate::sendWindowCaption(int windowId, bool succeed, wl_resource *buffer)
{
    const auto clientResources = resourceMap();
    for (Resource *resource : clientResources) {
        qWarning() << __func__ << ":" << __LINE__ << "ut-gfx-capture-sendWindowCaption: windowId " << windowId << " resource" << resource->handle;
        // com_deepin_client_management_send_capture_callback(resource->handle, windowId, succeed, buffer);
    }
}

void ClientManagementInterfacePrivate::sendWindowFromPoint(uint32_t wid)
{
    const auto clientResources = resourceMap();
    for (Resource *resource : clientResources)
        com_deepin_client_management_send_window_from_point(resource->handle, wid);
}

ClientManagementInterface::ClientManagementInterface(Display *display, QObject *parent)
    : QObject(parent)
    , d(new ClientManagementInterfacePrivate(this, display))
{
    connect(this, &ClientManagementInterface::windowStatesChanged, this, [this] { this->d->updateWindowStates(); });
}

ClientManagementInterface::~ClientManagementInterface() = default;

ClientManagementInterface *ClientManagementInterface::get(wl_resource* native)
{
    if (auto interfacePrivate = resource_cast<ClientManagementInterfacePrivate *>(native)) {
        return interfacePrivate->q;
    }
    return nullptr;
}

void ClientManagementInterface::setWindowStates(QList<WindowState*> &windowStates)
{
    int i = 0;
    for (auto it = windowStates.begin();
        it != windowStates.end() && i < MAX_WINDOWS;
        ++it) {
        memcpy(&d->m_windowStates[i++], *it, sizeof(WindowState));
        d->m_windowCount = i;
    }
    Q_EMIT windowStatesChanged();
}

void ClientManagementInterface::sendWindowFromPoint(uint32_t wid)
{
    d->sendWindowFromPoint(wid);
}

}
