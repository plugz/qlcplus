/*
  Q Light Controller Plus
  functionparent.h

  Copyright (C) 2016 Massimo Callegari
                     David Garyga

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef FUNCTIONPARENT_H
#define FUNCTIONPARENT_H

#include "functiontimings.h"

/** @addtogroup engine_functions Functions
 * @{
 */

struct Attribute
{
    QString name;
    qreal value;
    // TODO add Type (LTP/HTP)
};

/**
 * Start/Stop source
 */
class FunctionParent
{
public:
    // The type of the FunctionParent has 2 purposes:
    //
    // 1. It allows to differentiate the ID of a VCWidget
    // and the ID of a Function, because they could overlap
    // otherwise
    //
    // 2. It allows to define a special behavior for some
    // types. Example: a Master FunctionParent can stop any
    // function, regardless of what started it.
    //
    // AutoVCWidget and ManualVCWidget are separated.
    // In order to keep some parts of the current behavior,
    // ManualVCWidget acts like the "Master" type and can stop a
    // running function when the user uses a manual VCWidget.
    enum Type
    {
        // Another function (Chaser, Collection...)
        Function = 0,
        // An automatic VC widget (VCAudioTriggers)
        AutoVCWidget,
        // A manual VC widget (Button, Slider...)
        ManualVCWidget,
        // Override anything (MasterTimer, test facilities...)
        Master = 0xffffffff,
    };

private:
    quint64 m_id;
    QList<Attribute> m_attributes;
    FunctionTimings m_timings;

public:
    explicit FunctionParent(Type type, quint32 id);
    explicit FunctionParent(Type type, quint32 id, QList<Attribute> const& attributes);
    explicit FunctionParent(Type type, quint32 id, FunctionTimings const& timings);
    explicit FunctionParent(Type type, quint32 id, QList<Attribute> const& attributes, FunctionTimings const& timings);

    void setAttributes(QList<Attribute> const& attributes);
    void adjustAttribute(qreal fraction, int attributeIndex);
    qreal getAttributeValue(int attributeIndex) const;

    void setTimings(FunctionTimings const& timings);
    void setFadeIn(quint32 fadeIn);
    void setHold(quint32 hold);
    void setFadeOut(quint32 fadeOut);
    void setDuration(quint32 duration);

    bool operator ==(FunctionParent const& right) const;
    quint32 type() const;
    quint32 id() const;

    static FunctionParent master();
};

/** @} */

#endif
