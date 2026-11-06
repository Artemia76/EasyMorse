/****************************************************************************
**
** Copyright (C) 2026 Gianni Peschiutta
** Contact: https://github.com/Artemia76/EasyMorse
**
** EasyMorse is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** EasyMorse is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** The license is as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this software. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
****************************************************************************/

/****************************************************************************
* qmltypes.h is part of EasyMorse project
*
* Exposes the core objects to QML without making the core depend on QML:
* the application creates them, QML sees them as singletons.
* **************************************************************************/

#ifndef QMLTYPES_H
#define QMLTYPES_H

#include <QJSEngine>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include "keyer/keyerinput.h"
#include "settings/settings.h"
#include "trainer.h"

///
/// \brief Singleton returning an object created by main()
///
template <typename T>
struct ExistingInstance
{
    inline static T* s_instance = nullptr;

    static T* create(QQmlEngine*, QJSEngine*)
    {
        Q_ASSERT(s_instance);
        // QML must not delete an object it did not create
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }
};

struct TrainerForeign : ExistingInstance<Trainer>
{
    Q_GADGET
    QML_FOREIGN(Trainer)
    QML_NAMED_ELEMENT(Trainer)
    QML_SINGLETON
};

struct SettingsForeign : ExistingInstance<Settings>
{
    Q_GADGET
    QML_FOREIGN(Settings)
    QML_NAMED_ELEMENT(MorseSettings)
    QML_SINGLETON
};

struct KeyerInputForeign
{
    Q_GADGET
    QML_FOREIGN(KeyerInput)
    QML_ANONYMOUS
};

struct ManualKeyerForeign
{
    Q_GADGET
    QML_FOREIGN(ManualKeyer)
    QML_ANONYMOUS
};

#endif // QMLTYPES_H
