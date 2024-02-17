// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "chameleonbutton.h"
#include "chameleon.h"
#include "kwinutils.h"
#include "chameleonwindowtheme.h"

#include <KDecoration2/DecoratedClient>
#include <KDecoration2/Decoration>
#include <QTimer>
#include <QHoverEvent>
#include <QPainter>
#include <QDebug>
#include <QRect>
#include <QPoint>
#include <QX11Info>
#include <qnamespace.h>

#include "workspace.h"

#define LONG_PRESS_TIME     300
#define OUT_RELEASE_EVENT   100

ChameleonButton::ChameleonButton(KDecoration2::DecorationButtonType type, const QPointer<KDecoration2::Decoration> &decoration, QObject *parent)
    : KDecoration2::DecorationButton(type, decoration, parent)
{
    auto c = decoration->client().data();
    qDebug()<<__FUNCTION__<<__LINE__<<"windowId: "<<c->windowId();

    m_type = type;

    switch (type) {
    case KDecoration2::DecorationButtonType::Menu:
        break;
    case KDecoration2::DecorationButtonType::Minimize:
        setVisible(c->isMinimizeable());
        connect(c, &KDecoration2::DecoratedClient::minimizeableChanged, this, &ChameleonButton::setVisible);
        break;
    case KDecoration2::DecorationButtonType::Maximize:
        setVisible(c->isMaximizeable());
        connect(c, &KDecoration2::DecoratedClient::maximizeableChanged, this, &ChameleonButton::setVisible);
        break;
    case KDecoration2::DecorationButtonType::Close:
        setVisible(c->isCloseable());
        connect(c, &KDecoration2::DecoratedClient::closeableChanged, this, &ChameleonButton::setVisible);
        break;
    default: // 隐藏不支持的按钮
        setVisible(false);
        break;
    }
    if (m_type == KDecoration2::DecorationButtonType::Maximize) {
        connect(KWinUtils::compositor(), SIGNAL(compositingToggled(bool)), this, SLOT(onCompositorChanged(bool)));
        connect(KWinUtils::workspace(), SIGNAL(clientAreaUpdate()), this, SLOT(onClientAreaUpdate()));
    }
}

ChameleonButton::~ChameleonButton()
{
    if (m_pSplitMenu) {
        delete m_pSplitMenu;
        m_pSplitMenu = nullptr;
    }
}

KDecoration2::DecorationButton *ChameleonButton::create(KDecoration2::DecorationButtonType type, KDecoration2::Decoration *decoration, QObject *parent)
{
    return new ChameleonButton(type, decoration, parent);
}

void ChameleonButton::paint(QPainter *painter, const QRect &repaintRegion)
{
    Q_UNUSED(repaintRegion)

    Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());

    if (!decoration)
        return;

    const QRect &rect = geometry().toRect();

    painter->save();

    auto c = decoration->client().data();

    QIcon::Mode state = QIcon::Normal;

    if (!isEnabled()) {
        state = QIcon::Disabled;
    } else if (isPressed()) {
        state = QIcon::Selected;
    } else if (isHovered()) {
        state = QIcon::Active;
    }

    auto config = decoration->theme().titlebarConfig;

    auto drawButton  = [painter, decoration](const QIcon &icon, const QSize &btnSize, const QRect &rect, QIcon::Mode mode) {
        QPixmap pixmap = icon.pixmap(std::max(btnSize.width(), btnSize.height()) * decoration->getScaleFactor(), mode);
        painter->drawPixmap(rect, pixmap, QRect(QPoint(0, pixmap.height() - btnSize.height() * decoration->getScaleFactor()) / 2, btnSize * decoration->getScaleFactor()));
    };

    switch (type()) {
    case KDecoration2::DecorationButtonType::Menu: {
        drawButton(c->icon(), {config.menuBtn.width, config.menuBtn.height}, rect, state);
        break;
    }
    case KDecoration2::DecorationButtonType::ApplicationMenu: {
        drawButton(decoration->menuIcon(), {config.menuBtn.width, config.menuBtn.height}, rect, state);
        break;
    }
    case KDecoration2::DecorationButtonType::Minimize: {
        drawButton(decoration->minimizeIcon(), {config.minimizeBtn.width, config.minimizeBtn.height}, rect, state);
        break;
    }
    case KDecoration2::DecorationButtonType::Maximize: {
        if (isChecked())
            drawButton(decoration->unmaximizeIcon(), {config.unmaximizeBtn.width, config.unmaximizeBtn.height}, rect, state);
        else
            drawButton(decoration->maximizeIcon(), {config.maximizeBtn.width, config.maximizeBtn.height}, rect, state);
        break;
    }
    case KDecoration2::DecorationButtonType::Close: {
        drawButton(decoration->closeIcon(), {config.closeBtn.width, config.closeBtn.height}, rect, state);
        break;
    }
    default:
        break;
    }

    painter->restore();
}

void ChameleonButton::hoverEnterEvent(QHoverEvent *event)
{
    if (!m_isMaxAvailble && !QX11Info::isPlatformX11())
        return;

    m_wlHoverStatus = true;

    if (KWinUtils::instance()->isCompositing()) {
        Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());
        if (decoration) {
            effect = decoration->effect();
            if (effect && !effect->isUserMove()) {
                KDecoration2::DecorationButton::hoverEnterEvent(event);

                if (!contains(event->posF())) {
                    return;
                }
                if (m_type == KDecoration2::DecorationButtonType::Maximize) {
                    if (KWinUtils::instance()->isCompositing()) {
                        if (!m_pSplitMenu && KWinUtils::isShowSplitMenu()) {
                            QObject *client = nullptr;
                            if (QX11Info::isPlatformX11()) {
                                client = KWinUtils::findClient(KWinUtils::Predicate::WindowMatch, decoration->client().data()->windowId());
                            } else {
                                client = KWinUtils::findObjectByDecorationClient(decoration->client().data());
                            }
                            bool isSupportFourSplit = KWinUtils::Window::checkSupportFourSplit(client);
                            m_pSplitMenu = new ChameleonSplitMenu(nullptr, isSupportFourSplit);
                            m_pSplitMenu->setEffect(client);
                        }
                        if (m_pSplitMenu) {
                            m_pSplitMenu->stopTime();
                            m_pSplitMenu->Hide();
                        }
                        m_backgroundColor = decoration->getBackgroundColor();
                        if (!max_hover_timer) {
                            max_hover_timer = new QTimer();
                            max_hover_timer->setSingleShot(true);

                            connect(max_hover_timer, &QTimer::timeout, [this]{
                                if (m_pSplitMenu) {
                                    showSplitMenu();
                                }
                            });
                            max_hover_timer->start(500);
                            m_mousePosX = event->pos().x();
                        }
                        else {
                            max_hover_timer->start(500);
                            m_mousePosX = event->pos().x();
                        }
                    }
                    decoration->requestHideToolTip();
                }
            }
        }
    } else {
        KDecoration2::DecorationButton::hoverEnterEvent(event);
    }
}

void ChameleonButton::hoverLeaveEvent(QHoverEvent *event)
{
    if (!m_wlHoverStatus && !QX11Info::isPlatformX11())
        return;

    m_wlHoverStatus = false;
    if (KWinUtils::instance()->isCompositing()) {
        Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());
        if (decoration) {
            effect = decoration->effect();
            if (max_hover_timer && m_type == KDecoration2::DecorationButtonType::Maximize) {
                max_hover_timer->stop();
            }
            if (effect && !effect->isUserMove()) {
                KDecoration2::DecorationButton::hoverLeaveEvent(event);
                if (m_pSplitMenu && m_type == KDecoration2::DecorationButtonType::Maximize) {
                    m_pSplitMenu->setShowSt(false);
                    m_pSplitMenu->startTime();
                }
            }
        }
    } else {
        KDecoration2::DecorationButton::hoverLeaveEvent(event);
    }
}

void ChameleonButton::mousePressEvent(QMouseEvent *event)
{
    KDecoration2::DecorationButton::mousePressEvent(event);
    if (m_type == KDecoration2::DecorationButtonType::Maximize) {
        if (!max_timer) {
            max_timer = new QTimer();
            max_timer->setSingleShot(true);
            connect(max_timer, &QTimer::timeout, [this] {
                if (m_isMaxAvailble) {
                    m_isMaxAvailble = false;
                    Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());
                    if (decoration) {
                        effect = decoration->effect();
                        if (m_pSplitMenu && effect) {
                            m_wlHoverStatus = false;
                            showSplitMenu();
                        }
                    }
                }
            });
            max_timer->start(LONG_PRESS_TIME);
            m_mousePosX = event->pos().x();
        } else {
            max_timer->start(LONG_PRESS_TIME);
            m_mousePosX = event->pos().x();
        }
    }
}

void ChameleonButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_type == KDecoration2::DecorationButtonType::Maximize) {
        if (max_timer) {
            max_timer->stop();
        }
        if (!m_isMaxAvailble) {
            event->setLocalPos(QPointF(event->localPos().x() - OUT_RELEASE_EVENT, event->localPos().y()));
        }
        if (m_pSplitMenu) {
            m_pSplitMenu->setShowSt(false);
            // m_pSplitMenu->Hide();
            // m_pSplitMenu->startTime(500);
        }
    }
    KDecoration2::DecorationButton::mouseReleaseEvent(event);
    m_isMaxAvailble = true;
}

void ChameleonButton::onCompositorChanged(bool active)
{
    if (!active && m_pSplitMenu) {
        m_pSplitMenu->Hide();
    }
}

void ChameleonButton::onClientAreaUpdate()
{
    if (m_pSplitMenu) {
        Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());
        if (decoration) {
            if (KWinUtils::instance()->isCompositing()) {
                QObject *client = nullptr;
                if (QX11Info::isPlatformX11()) {
                    client = KWinUtils::findClient(KWinUtils::Predicate::WindowMatch, decoration->client().data()->windowId());
                } else {
                    client = KWinUtils::findObjectByDecorationClient(decoration->client().data());
                }
                bool isSupportFourSplit = KWinUtils::Window::checkSupportFourSplit(client);
                if (m_pSplitMenu->getSupportFourSplit() != isSupportFourSplit) {
                    delete m_pSplitMenu;
                    m_pSplitMenu = new ChameleonSplitMenu(nullptr, isSupportFourSplit);
                    m_pSplitMenu->setEffect(client);
                }
            }
        }
    }
}

void ChameleonButton::showSplitMenu()
{
    qreal x = effect->geometry().x();
    qreal y = effect->geometry().y();
    QPoint p(x + geometry().x(), y + geometry().height());
    QPoint mousePos = QPoint(x + m_mousePosX, y + geometry().height());
    QPoint point;
    QRect screenGeometry;
    for (auto effectScreen : KWin::effects->screens()) {
        screenGeometry = effectScreen->geometry();
        if (screenGeometry.contains(p)) {
            point = p;
            break;
        } else if (screenGeometry.contains(mousePos)) {
            point = mousePos;
            break;
        }
    }
    m_pSplitMenu->setShowSt(true);
    m_pSplitMenu->stopTime();
    m_pSplitMenu->Show(screenGeometry, point, m_backgroundColor);
}
