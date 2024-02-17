// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KWINEFFECTSEX_H
#define KWINEFFECTSEX_H

#include <deepin_kwineffects.h>
#include <deepin_kwinconfig.h>
#include <deepin_kwineffects_export.h>
#include <deepin_kwinglobals.h>


/**
 * Logging category to be used inside the KWin effects.
 * Do not use in this library.
 **/
//Q_DECLARE_LOGGING_CATEGORY(KWINEFFECTS)

namespace KWin
{

class EffectWindow;

#define KWIN_EFFECT_API_MAKE_VERSION( major, minor ) (( major ) << 8 | ( minor ))
#define KWIN_EFFECT_API_VERSION_MAJOR 0
#define KWIN_EFFECT_API_VERSION_MINOR 227
#define KWIN_EFFECT_API_VERSION KWIN_EFFECT_API_MAKE_VERSION( \
        KWIN_EFFECT_API_VERSION_MAJOR, KWIN_EFFECT_API_VERSION_MINOR )


class KWINEFFECTS_EXPORT EffectsHandlerEx : public EffectsHandler
{
    Q_OBJECT
public:
    explicit EffectsHandlerEx(CompositingType type);
    virtual ~EffectsHandlerEx();

    virtual QString getActiveColor() = 0;
    virtual void setKeepAbove(KWin::EffectWindow *c, bool) = 0;
    virtual void requestLock() = 0;
    virtual void sendPointer(Qt::MouseButton) = 0;
    virtual void setCursorPos(const QPoint& pos) = 0;
    virtual void changeBlurState(bool) = 0;
    virtual EffectWindowList getChildWinList(KWin::EffectWindow *w) = 0;
    virtual bool isTransientWin(KWin::EffectWindow *w) = 0;
    virtual int getCurrentPaintingScreen() = 0;
    virtual bool isShortcuts(QKeyEvent *event) = 0;
    virtual void setActiveMultitasking(bool isActive) = 0;

    virtual bool checkWindowAllowToSplit(KWin::EffectWindow *c) = 0;
    virtual int getSplitMode(int desktop, QString screen) = 0;
    virtual void setSplitWindow(KWin::EffectWindow* c, int mode, bool isShowPreview = false) = 0;
    virtual SwipeDirection desktopChangedDirection() const = 0;
    virtual void resetSplitOutlinePos(QString screen, int desktop) = 0;
    virtual void setShowLine(bool flag, QRect rt = QRect()) = 0;
    virtual KWin::EffectWindow *findSplitGroupSecondaryClient(int desktop, QString screen) = 0;
    virtual void setSplitOutlinePos(int pos, bool isLeftRight = true) = 0;
    virtual void raiseActiveWindow(int desktop, QString screen) = 0;

    virtual bool getOutlineState() = 0;
    virtual void getOutlineRect(QString screen, QRect &hRect, QRect &vRect) = 0;
    virtual void getSplitList(QSet<KWin::EffectWindow *> &list, int &location, int desktop, QString screen) = 0;
    virtual void getActiveSplitList(QSet<KWin::EffectWindow *> &list, int desktop, QString screen) = 0;
    virtual void switchSplitWorkspace(int srcdesktop, int dstdesktop) = 0;
    virtual void getStockSplitList(QSet<KWin::EffectWindow *> &list, int desktop, QString screen) = 0;
    virtual QRect getSplitArea(int mode, QRect rect, QRect availableArea, QString screen, int desktop, bool isUseTmp = false) = 0;
    virtual QString getScreenWithSplit() = 0;

Q_SIGNALS:
    void windowQuickTileModeChanged(KWin::EffectWindow *w);
    void showSplitScreenPreview(KWin::EffectWindow *w);
    void signalSplitScreenStartShowMasking(QString screen, bool isTopDownMove);
    void signalSplitScreenResizeMasking(int pos);
    void signalSplitScreenExitMasking();
    void splitEventActive();
    //void swapSplitWin(KWin::EffectWindow *w, int index);
    void swapSplitWin(QSet<KWin::EffectWindow *> list, int index, int direction);

};

/**
 * Pointer to the global EffectsHandlerEx object.
 **/
extern KWINEFFECTS_EXPORT EffectsHandlerEx* effectsEx;


} // namespace

#endif // KWINEFFECTSEX_H
