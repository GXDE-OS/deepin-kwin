// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "deepin_kwineffectsex.h"

namespace KWin
{

EffectsHandlerEx::EffectsHandlerEx(CompositingType type)
    : EffectsHandler (type)
{
    KWin::effectsEx = this;
}

EffectsHandlerEx::~EffectsHandlerEx()
{
    KWin::effectsEx = nullptr;
}

EffectsHandlerEx* effectsEx = nullptr;
}
