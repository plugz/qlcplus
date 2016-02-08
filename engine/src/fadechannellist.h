/*
  Q Light Controller Plus
  fadechannellist.h

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

#ifndef FADECHANNELLIST_H
#define FADECHANNELLIST_H

#include <QtGlobal>

#include "qlcchannel.h"
#include "fixture.h"
#include "doc.h"

/** @addtogroup engine Engine
 * @{
 */

/**
 * This is a list of fadechannels.
 * This is used to get the current value from all the running fades,
 * because so may be faster than others.
 */
class FadeChannelList
{
    /************************************************************************
     * Initialization
     ************************************************************************/
public:
    /** Create a new FadeChannel with empty/invalid values */
    FadeChannelList();

    /** Non-virtual destructor: do not inherit */
    ~FadeChannelList();

    /************************************************************************
     * Values
     ************************************************************************/
public:
    /**
     * Increment elapsed() by $ms milliseconds, calculate the next step and
     * return the new current() value.
     */
    uchar nextStep(uint ms);

    void add(FadeChannel const& channel);

    // void remove(FadeChannel const& channel);

private:
    QList<FadeChannel> m_fadeChannels;
};

/** @} */

#endif
