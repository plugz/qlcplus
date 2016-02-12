/*
  Q Light Controller
  fadeoutfader.h

  Copyright (c) Heikki Junnila

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

#ifndef FADEOUTFADER
#define FADEOUTFADER

#include <QList>
#include <QHash>

#include "genericfader.h"

/** @addtogroup engine Engine
 * @{
 */

class FadeOutFader : public GenericFader
{
public:
    explicit FadeOutFader(Doc* doc);
    virtual ~FadeOutFader();

    void add(GenericFader const& fader, qreal faderIntensity, uint fadeOutTime);

    virtual void write(QList<Universe *> universes);
};

/** @} */

#endif
