/*
  Q Light Controller Plus
  chaserstep.h

  Copyright (C) 2004 Heikki Junnila
                2015 Massimo Callegari

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

#ifndef CHASERSTEP_H
#define CHASERSTEP_H

#include <QVariant>
#include "scenevalue.h"
#include "function.h"

class QXmlStreamReader;

/** @addtogroup engine_functions Functions
 * @{
 */

/**
 * A ChaserStep encapsulates a function ID with fade in, fade out and duration
 * speeds (in milliseconds). Thus, each step can optionally use step-specific
 * speeds if the user so wishes.
 */
class ChaserStep
{
    /************************************************************************
     * Initialization
     ***********************************************************************/
public:
    /** Construct a new ChaserStep with the given attributes */
    ChaserStep(quint32 aFid = Function::invalidId(),
               FunctionSpeeds const& speeds = FunctionSpeeds(0, 0, 0));

    /** Copy constructor */
    ChaserStep(const ChaserStep& cs);

    /** Comparison operator (only function IDs are compared) */
    bool operator==(const ChaserStep& cs) const;

    /** Return the actual function pointer for $fid from $doc */
    Function* resolveFunction(const Doc* doc) const;

    /************************************************************************
     * QVariant operations
     ***********************************************************************/
public:
#if 1
    /** Construct a new ChaserStep from the given QVariant */
    static ChaserStep fromVariant(const QVariant& var);

    /** Construct a QVariant from a ChaserStep */
    QVariant toVariant() const;
#endif
    /************************************************************************
     * Load & Save
     ***********************************************************************/
public:
    /** Load ChaserStep contents from $root and return step index in $stepNumber */
    bool loadXML(QXmlStreamReader &root, int& stepNumber);

    /** Save ChaserStep contents to $doc, with $stepNumber */
    bool saveXML(QXmlStreamWriter *doc, int stepNumber, bool isSequence) const;

public:
    quint32 fid;                 //! The function ID
    FunctionSpeeds speeds;     //! The speeds of this step
    QList <SceneValue> values;   //! specific DMX values for this step (chaser in sequence mode)
    QString note;

public:
    FunctionSpeeds m_overrideSpeeds;
};

/** @} */

#endif
