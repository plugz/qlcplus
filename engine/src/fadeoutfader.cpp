/*
  Q Light Controller
  fadeoutfader.cpp

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

#include <cmath>
#include <QDebug>

#include "fadeoutfader.h"
#include "fadechannel.h"

FadeOutFader::FadeOutFader(Doc* doc)
    : GenericFader(doc)
{
}

FadeOutFader::~FadeOutFader()
{
}

void FadeOutFader::add(GenericFader const& fader, qreal faderIntensity, uint fadeOutTime)
{
    QHashIterator <FadeChannel,FadeChannel> it(fader.channels());
    QList<FadeChannel> newChannels;
    while (it.hasNext() == true)
    {
        it.next();
        FadeChannel fc = it.value();

        // fade out only intensity channels
        if (fc.group(m_doc) != QLCChannel::Intensity)
            continue;

        bool canFade = true;
        Fixture *fixture = m_doc->fixture(fc.fixture());
        if (fixture != NULL)
            canFade = fixture->channelCanFade(fc.channel());
        fc.setStart(fc.current(faderIntensity));
        fc.setCurrent(fc.current(faderIntensity));

        fc.setElapsed(0);
        fc.setReady(false);
        if (canFade == false)
        {
            fc.setFadeTime(0);
            fc.setTarget(fc.current(faderIntensity)); // is this an LTP thing ?
        }
        else
        {
            if (fc.fadeTime() > fadeOutTime)
                fc.setFadeTime(fadeOutTime);
            fc.setTarget(0);
        }
        newChannels << fc;
    }

    foreach(FadeChannel const& fc, newChannels)
    {
        // TODO complicated stuff
        GenericFader::add(fc);
    }
}

void FadeOutFader::write(QList<Universe *> universes)
{
    // TODO complicated stuff
    return GenericFader::write(universes);
}
