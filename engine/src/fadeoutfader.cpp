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
    qDebug() << Q_FUNC_INFO;

    QHashIterator <FadeChannel,FadeChannel> it(fader.channels());
    QList<FadeChannel> newChannels;
    while (it.hasNext() == true)
    {
        it.next();
        FadeChannel const& fcRef = it.value();

        // fade out only intensity channels
        if (fcRef.group(m_doc) != QLCChannel::Intensity)
        {
            qDebug() << Q_FUNC_INFO << "a channel is not intensity";
            continue;
        }
        qDebug() << Q_FUNC_INFO << "a channel is intensity";

        FadeChannel fc(fcRef);

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
            // TODO understand this. is this an LTP thing ? Should this be removed ?
            fc.setFadeTime(0);
            fc.setTarget(fc.current(faderIntensity));
        }
        else
        {
            fc.setFadeTime(fadeOutTime);
            fc.setTarget(0);
        }

        newChannels << fc;
    }

    foreach(FadeChannel const& fc, newChannels)
    {
        qDebug() << Q_FUNC_INFO << "newFC loop";
        if (m_channels.contains(fc))
        {
            qDebug() << Q_FUNC_INFO << "contains";
            FadeChannel& currentFc = m_channels[fc];
            if (fadeChannelIsBigger(fc, currentFc))
                currentFc = fc;
            else if (!fadeChannelIsBigger(currentFc, fc))
            {
                tryToInsert(fc);
            }
        }
        else
        {
            qDebug() << Q_FUNC_INFO << "no contains: genericfader add";
            GenericFader::add(fc);
        }
    }
}

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

bool FadeOutFader::fadeChannelIsBigger(FadeChannel const& left, FadeChannel const& right)
{
    return qMin(left.current(), left.target()) > qMax(right.current(), right.target());
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
