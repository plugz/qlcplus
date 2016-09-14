/*
  Q Light Controller Plus
  cue.cpp

  Copyright (c) Heikki Junnila
                Massimo Callegari

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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

#include "cue.h"

Cue::Cue(const QString& name)
    : m_name(name)
    , m_fadeIn(0)
    , m_fadeOut(0)
    , m_duration(0)
{
}

Cue::Cue(const QHash <uint,uchar> values)
    : m_name(QString())
    , m_values(values)
    , m_fadeIn(0)
    , m_fadeOut(0)
    , m_duration(0)
{
}

Cue::Cue(const Cue& cue)
    : m_name(cue.name())
    , m_values(cue.values())
    , m_fadeIn(cue.fadeIn())
    , m_fadeOut(cue.fadeOut())
    , m_duration(cue.duration())
{
}

Cue::~Cue()
{
}

/****************************************************************************
 * Name
 ****************************************************************************/

void Cue::setName(const QString& str)
{
    m_name = str;
}

QString Cue::name() const
{
    return m_name;
}

/****************************************************************************
 * Values
 ****************************************************************************/

void Cue::setValue(uint channel, uchar value)
{
    m_values[channel] = value;
}

void Cue::unsetValue(uint channel)
{
    if (m_values.contains(channel) == true)
        m_values.remove(channel);
}

uchar Cue::value(uint channel) const
{
    if (m_values.contains(channel) == true)
        return m_values[channel];
    else
        return 0;
}

QHash <uint,uchar> Cue::values() const
{
    return m_values;
}

/****************************************************************************
 * Speed
 ****************************************************************************/

void Cue::setFadeIn(uint ms)
{
    m_fadeIn = ms;
}

uint Cue::fadeIn() const
{
    return m_fadeIn;
}

void Cue::setFadeOut(uint ms)
{
    m_fadeOut = ms;
}

uint Cue::fadeOut() const
{
    return m_fadeOut;
}

void Cue::setDuration(uint ms)
{
    m_duration = ms;
}

uint Cue::duration() const
{
    return m_duration;
}

/****************************************************************************
 * Load & Save
 ****************************************************************************/

bool Cue::loadXML(QXmlStreamReader &root)
{
    qDebug() << Q_FUNC_INFO;

    if (root.name() != KXMLQLCCue)
    {
        qWarning() << Q_FUNC_INFO << "Cue node not found";
        return false;
    }

    setName(root.attributes().value(KXMLQLCCueName).toString());

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCCueValue)
        {
            QString ch = root.attributes().value(KXMLQLCCueValueChannel).toString();
            QString val = root.readElementText();
            if (ch.isEmpty() == false && val.isEmpty() == false)
                setValue(ch.toUInt(), uchar(val.toUInt()));
        }
        else if (root.name() == KXMLQLCCueSpeed)
        {
            loadXMLSpeed(root);
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unrecognized Cue tag:" << root.name();
            root.skipCurrentElement();
        }
    }

    return true;
}

bool Cue::saveXML(QXmlStreamWriter *doc) const
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT(doc != NULL);

    doc->writeStartElement(KXMLQLCCue);
    doc->writeAttribute(KXMLQLCCueName, name());

    QHashIterator <uint,uchar> it(values());
    while (it.hasNext() == true)
    {
        it.next();
        doc->writeStartElement(KXMLQLCCueValue);
        doc->writeAttribute(KXMLQLCCueValueChannel, QString::number(it.key()));
        doc->writeCharacters(QString::number(it.value()));
        doc->writeEndElement();
    }

    saveXMLSpeed(doc);

    /* End the <Cue> tag */
    doc->writeEndElement();

    return true;
}

bool Cue::loadXMLSpeed(QXmlStreamReader &speedRoot)
{
    if (speedRoot.name() != KXMLQLCCueSpeed)
        return false;

    m_fadeIn = speedRoot.attributes().value(KXMLQLCCueSpeedFadeIn).toString().toUInt();
    m_fadeOut = speedRoot.attributes().value(KXMLQLCCueSpeedFadeOut).toString().toUInt();
    m_duration = speedRoot.attributes().value(KXMLQLCCueSpeedDuration).toString().toUInt();
    speedRoot.skipCurrentElement();

    return true;
}

bool Cue::saveXMLSpeed(QXmlStreamWriter *doc) const
{
    doc->writeStartElement(KXMLQLCCueSpeed);
    doc->writeAttribute(KXMLQLCCueSpeedFadeIn, QString::number(fadeIn()));
    doc->writeAttribute(KXMLQLCCueSpeedFadeOut, QString::number(fadeOut()));
    doc->writeAttribute(KXMLQLCCueSpeedDuration, QString::number(duration()));
    doc->writeEndElement();

    return true;
}
