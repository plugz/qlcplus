/*
  Q Light Controller Plus
  fadechannellist.cpp

  Copyright (c) Heikki Junnila
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

#include <QDebug>
#include <cmath>

#include "fadechannellist.h"
#include "fadechannel.h"
#include "qlcmacros.h"

FadeChannelList::FadeChannelList(FadeChannel const& fadeChannel)
    : m_fadeChannel(fadeChannel)
{
    add(fadeChannel);
}

FadeChannelList::~FadeChannelList()
{
}

void FadeChannelList::add(FadeChannel const& channel)
{
    Q_ASSERT(channel == m_fadeChannel);

    foreach(FadeChannel const& curChannel, m_fadeChannels)
    {
        if (MIN(channel.current(), channel.target())
                > MAX(curChannel.current(), curChannel.target()))
            continue;

        // If the new channel is lower than any current channel, abandon it
        return;
    }
    // If the new channel may be at some time bigger than any current channel, add it
    m_fadeChannels.push_back(channel);
}

uchar FadeChannelList::nextStep(uint ms)
{
    // TODO handle LTP ?
    uchar res = 0;
    {
        // first loop: get val
        QListIterator it(m_fadeChannels);
        while (it.hasNext())
        {
            FadeChannel& fadeChannel = it.next();
            uchar val = fadeChannel.nextStep(ms);
            if (val > res)
                res = val;
        }
    }
    {
        // second loop: cleanup
        QMutableListIterator it(m_fadeChannels);
        while (it.hasNext())
        {
            FadeChannel& fadeChannel = it.next();
            if (fadeChannel.elapsed() >= fadeChannel.fadeTime())
            {
                uchar currentMin = 0;
                QListIterator currentIt(m_fadeChannels);
                while (currentIt.hasNext())
                {
                    FadeChannel const& currentFc = currentIt.next();
                    if (&currentFc != &fadeChannel)
                        currentMin = MAX(currentMin,
                                MIN(currentFc.current(), currentFc.target()));
                }
                if (fadeChannel.current() <= currentMin)
                    it.remove();
            }
        }
    }
    return res;
}
