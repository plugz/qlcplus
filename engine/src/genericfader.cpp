/*
  Q Light Controller
  genericfader.cpp

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

#include "genericfader.h"
#include "fadechannel.h"
#include "doc.h"

GenericFader::GenericFader(Doc* doc)
    : m_intensity(1)
    , m_blendMode(Universe::NormalBlend)
    , m_doc(doc)
{
    Q_ASSERT(doc != NULL);
}

GenericFader::~GenericFader()
{
}


namespace {
static bool fadeChannelIsBigger(FadeChannel const& left, FadeChannel const& right)
{
    return qMin(left.current(), left.target()) > qMax(right.current(), right.target());
}
}

void GenericFader::add(const FadeChannel& ch)
{
    QHash<FadeChannel, QList<FadeChannel> >::iterator hashIt = m_channels.find(ch);
    if (hashIt != m_channels.end())
    {
        QMutableListIterator<FadeChannel> listIt(hashIt.value());
        while (listIt.hasNext())
        {
            FadeChannel& currentCh = listIt.next();
            if (fadeChannelIsBigger(ch, currentCh))
            {
                // The new channel is bigger than the current one during its whole life:
                // remove the current one
                listIt.remove();
            }
            else if (fadeChannelIsBigger(currentCh, ch))
            {
                // The current channel is bigger than the new one during its whole life:
                // don't add the new one
                return;
            }
        }
        hashIt.value().push_back(ch);
    }
    else
        m_channels[ch].push_back(ch);
}

void GenericFader::fadeOut(GenericFader const& fader, qreal faderIntensity, uint fadeOutTime)
{
    qDebug() << Q_FUNC_INFO;

    QHashIterator<FadeChannel, QList<FadeChannel> > hashIt(fader.channels());
    while (hashIt.hasNext())
    {
        QListIterator<FadeChannel> listIt(hashIt.next().value());
        while (listIt.hasNext())
        {
            FadeChannel const& fcRef = listIt.next();

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

            add(fc);
        }
    }
}

void GenericFader::forceAdd(const FadeChannel &ch)
{
    m_channels[ch].append(ch);
}

void GenericFader::remove(const FadeChannel& ch)
{
    m_channels.remove(ch);
}

void GenericFader::removeAll()
{
    m_channels.clear();
}

const QHash <FadeChannel,QList<FadeChannel> >& GenericFader::channels() const
{
    return m_channels;
}

bool GenericFader::getCurrentValue(FadeChannel const& ch, uchar& value) const
{
    QHash<FadeChannel,QList<FadeChannel> >::const_iterator hashIt = m_channels.find(ch);
    if (hashIt == m_channels.end())
        return false;
    // TODO LTP: get only last value ?
    QList<FadeChannel> const& list(hashIt.value());
    value = 0;
    for (QList<FadeChannel>::const_iterator it = list.begin(), ite = list.end(); it != ite; ++it)
    {
        if (it->current() > value)
            value = it->current();
    }
    return true;
}

bool GenericFader::getCurrentValues(FadeChannel const& ch, uchar& current, uchar& target, int& elapsed) const
{
    QHash<FadeChannel,QList<FadeChannel> >::const_iterator hashIt = m_channels.find(ch);
    if (hashIt == m_channels.end())
        return false;
    // TODO LTP: get only last value ?
    QList<FadeChannel> const& list(hashIt.value());
    current = 0;
    target = 0;
    elapsed = 0;
    for (QList<FadeChannel>::const_iterator it = list.begin(), ite = list.end(); it != ite; ++it)
    {
        // TODO basing all on current is not ideal
        if (it->current() > current)
        {
            current = it->current();
            elapsed = it->elapsed();
            target = it->target();
        }
    }
    return true;
}

void GenericFader::write(QList<Universe*> ua)
{
    QMutableHashIterator<FadeChannel,QList<FadeChannel> > hashIt(m_channels);
    while (hashIt.hasNext())
    {
        QList<FadeChannel>& list(hashIt.next().value());
        QMutableListIterator<FadeChannel> listIt(list);
        while (listIt.hasNext())
        {
            FadeChannel& ch(listIt.next());
            QLCChannel::Group grp = ch.group(m_doc);
            quint32 addr = ch.addressInUniverse();
            quint32 universe = ch.universe();
            bool canFade = ch.canFade(m_doc);

            // Calculate the next step
            uchar value = ch.nextStep(MasterTimer::tick());

            // Apply intensity to HTP channels
            if (grp == QLCChannel::Intensity && canFade)
                value = ch.current(intensity());

            if (universe != Universe::invalid())
            {
                //qDebug() << "[GenericFader] >>> uni:" << universe << ", address:" << addr << ", value:" << value;
                ua[universe]->writeBlended(addr, value, m_blendMode);
            }

            if (grp == QLCChannel::Intensity && m_blendMode == Universe::NormalBlend)
            {
                // Remove all HTP channels that reach their target _zero_ value.
                // They have no effect either way so removing them saves CPU a bit.
                if (ch.current() == 0 && ch.target() == 0)
                    listIt.remove();
            }
            if (ch.isFlashing())
                listIt.remove();
        }
        if (list.size() == 0)
            hashIt.remove();
    }
}

void GenericFader::adjustIntensity(qreal fraction)
{
    m_intensity = fraction;
}

qreal GenericFader::intensity() const
{
    return m_intensity;
}

void GenericFader::setBlendMode(Universe::BlendMode mode)
{
    m_blendMode = mode;
}
