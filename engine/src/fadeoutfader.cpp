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


void FadeOutFader::tryToInsert(FadeChannel const& fc)
{
    if (m_fadeOutChannels.contains(fc))
    {
        QList<FadeChannel>::iterator it(m_fadeOutChannels[fc].begin());
        QList<FadeChannel>::iterator end(m_fadeOutChannels[fc].end());
        while (it != end)
        {
            FadeChannel& currentFc = *it;
            if (fadeChannelIsBigger(fc, currentFc))
            {
                currentFc = fc;
                return;
            }
            if (fadeChannelIsBigger(currentFc, fc))
                return;
            ++it;
        }
    }
    m_fadeOutChannels[fc].push_back(fc);
}

void FadeOutFader::write(QList<Universe *> universes)
{
    // Advance the fades contained in genericFader
    GenericFader::write(universes);

    QMutableHashIterator <FadeChannel, QList<FadeChannel> > it(m_fadeOutChannels);
    while (it.hasNext())
    {
        QList<FadeChannel>& list(it.next().value());
        QMutableListIterator<FadeChannel> listIt(list);
        while (listIt.hasNext())
        {
            FadeChannel& fc(listIt.next());

            quint32 addr = fc.addressInUniverse();
            quint32 universe = fc.universe();

            // Calculate the next step
            uchar value = fc.nextStep(MasterTimer::tick());

            if (universe != Universe::invalid())
            {
                //qDebug() << "[GenericFader] >>> uni:" << universe << ", address:" << addr << ", value:" << value;
                universes[universe]->writeBlended(addr, value, m_blendMode);
            }

            // FadeOutFader: Remove all channels that reach their target _zero_ value.
            if (fc.current() == 0)
            {
                qDebug() << Q_FUNC_INFO << "remove" << fc.fadeTime() << "s";
                listIt.remove();
            }
        }
        if (list.size() == 0)
        {
            qDebug() << Q_FUNC_INFO << "remove a channel";
            it.remove();
        }
    }
}
