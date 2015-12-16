/*
  Q Light Controller Plus
  inputoutputmap.cpp

  Copyright (c) Massimo Callegari

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

#if defined(WIN32) || defined(Q_OS_WIN)
#	include <Windows.h>
#else
#   include <unistd.h>
#endif

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSettings>
#include <QDebug>

#include "inputoutputmap.h"
#include "qlcinputchannel.h"
#include "qlcinputsource.h"
#include "qlcioplugin.h"
#include "outputpatch.h"
#include "inputpatch.h"
#include "qlcconfig.h"
#include "universe.h"
#include "qlcfile.h"
#include "doc.h"

InputOutputMap::InputOutputMap(Doc *doc, quint32 universes)
  : QObject(doc)
  , m_blackout(false)
  , m_latestUniverseId(InputOutputMap::invalidUniverse())
  , m_universeChanged(false)
{
    m_grandMaster = new GrandMaster(this);
    for (quint32 i = 0; i < universes; i++)
        addUniverse();

    connect(doc->ioPluginCache(), SIGNAL(pluginConfigurationChanged(QLCIOPlugin*)),
            this, SLOT(slotPluginConfigurationChanged(QLCIOPlugin*)));
}

InputOutputMap::~InputOutputMap()
{
    removeAllUniverses();
    delete m_grandMaster;
}

Doc* InputOutputMap::doc() const
{
    return qobject_cast<Doc*> (parent());
}

/*****************************************************************************
 * Blackout
 *****************************************************************************/

bool InputOutputMap::toggleBlackout()
{
    if (m_blackout == true)
        setBlackout(false);
    else
        setBlackout(true);

    return m_blackout;
}

void InputOutputMap::setBlackout(bool blackout)
{
    /* Don't do blackout twice */
    if (m_blackout == blackout)
        return;

    QMutexLocker locker(&m_universeMutex);
    m_blackout = blackout;

    QByteArray zeros(512, 0);
    foreach (Universe* universe, m_universeMap)
    {
        if (universe->outputPatch() != NULL)
        {
            if (blackout == true)
                universe->outputPatch()->dump(universe->id(), zeros);
            // notify the universe listeners that some channels have changed
        }
        locker.unlock();
        if (blackout == true)
            emit universesWritten(universe->id(), zeros);
        else
        {
            const QByteArray postGM = universe->postGMValues()->mid(0, universe->usedChannels());
            emit universesWritten(universe->id(), postGM);
        }
        locker.relock();
    }

    emit blackoutChanged(m_blackout);
}

bool InputOutputMap::blackout() const
{
    return m_blackout;
}

/*****************************************************************************
 * Universes
 *****************************************************************************/

quint32 InputOutputMap::invalidUniverse()
{
    return Universe::invalid();
}

bool InputOutputMap::addUniverse(quint32 id)
{
    {
        QMutexLocker locker(&m_universeMutex);
        if (id == InputOutputMap::invalidUniverse())
            id = ++m_latestUniverseId;
        else if (m_latestUniverseId == InputOutputMap::invalidUniverse()
                || id >= m_latestUniverseId)
            m_latestUniverseId = id;
        else
            id = ++m_latestUniverseId;

        m_universeMap.insert(id, new Universe(id, m_grandMaster));
    }

    emit universeAdded(id);
    return true;
}

bool InputOutputMap::removeUniverse(quint32 id)
{
    {
        QMutexLocker locker(&m_universeMutex);

        if (!m_universeMap.contains(id))
        {
            qWarning() << Q_FUNC_INFO << "Trying to remove universe" << id << "that does not exist";
            return false;
        }

        delete m_universeMap.take(id);
        if (m_universeMap.isEmpty())
            m_latestUniverseId = invalidUniverse();
    }

    emit universeRemoved(id);
    return true;
}

bool InputOutputMap::removeAllUniverses()
{
    QMutexLocker locker(&m_universeMutex);
    qDeleteAll(m_universeMap);
    m_universeMap.clear();
    m_latestUniverseId = invalidUniverse();
    return true;
}

QString InputOutputMap::getUniverseName(quint32 id)
{
    QMap<quint32, Universe*>::iterator it = m_universeMap.find(id);
    if (it == m_universeMap.end())
        return QString();
    return it.value()->name();
}

void InputOutputMap::setUniverseName(quint32 id, QString name)
{
    QMap<quint32, Universe*>::iterator it = m_universeMap.find(id);
    if (it == m_universeMap.end())
        return;
    it.value()->setName(name);
}

void InputOutputMap::setUniversePassthrough(quint32 id, bool enable)
{
    QMap<quint32, Universe*>::iterator it = m_universeMap.find(id);
    if (it == m_universeMap.end())
        return;
    it.value()->setPassthrough(enable);
}

bool InputOutputMap::getUniversePassthrough(quint32 id)
{
    QMap<quint32, Universe*>::iterator it = m_universeMap.find(id);
    if (it == m_universeMap.end())
        return false;
    return it.value()->passthrough();
}

void InputOutputMap::setUniverseMonitor(quint32 id, bool enable)
{
    QMap<quint32, Universe*>::iterator it = m_universeMap.find(id);
    if (it == m_universeMap.end())
        return;
    it.value()->setMonitor(enable);
}

bool InputOutputMap::getUniverseMonitor(quint32 id)
{
    QMap<quint32, Universe*>::iterator it = m_universeMap.find(id);
    if (it == m_universeMap.end())
        return false;
    return it.value()->monitor();
}

bool InputOutputMap::isUniversePatched(quint32 id)
{
    QMap<quint32, Universe*>::iterator it = m_universeMap.find(id);
    if (it == m_universeMap.end())
        return false;
    return it.value()->isPatched();
}

quint32 InputOutputMap::universesCount() const
{
    return m_universeMap.count();
}

quint32 InputOutputMap::getNextUniverseID(quint32 id) const
{
    QMap<quint32, Universe*>::const_iterator it = m_universeMap.constFind(id);
    if (it == m_universeMap.constEnd() || ++it == m_universeMap.constEnd())
        return invalidUniverse();
    return it.key();
}

QList<Universe *> InputOutputMap::universes() const
{
    return m_universeMap.values();
}

QList<Universe*> InputOutputMap::claimUniverses()
{
    m_universeMutex.lock();
    return m_universeMap.values();
}

void InputOutputMap::releaseUniverses(bool changed)
{
    m_universeChanged = changed;
    m_universeMutex.unlock();
}

void InputOutputMap::dumpUniverses()
{
    if (m_blackout)
        return;

    QMap<quint32, QByteArray> writtenUniverses;

    {
        QMutexLocker locker(&m_universeMutex);
        foreach(Universe* universe, m_universeMap)
        {
            QByteArray const postGM = universe->postGMValues()->mid(0, universe->usedChannels());

            if (universe->hasChanged())
            {
                writtenUniverses.insert(universe->id(), postGM);
            }

            // this is where QLC+ sends data to the output plugins
            universe->dumpOutput(postGM);
        }
    }

    // notify the universe listeners that some channels have changed
    for (QMap<quint32, QByteArray>::iterator it = writtenUniverses.begin();
            it != writtenUniverses.end(); ++it)
    {
        emit universesWritten(it.key(), it.value());
    }
}

void InputOutputMap::resetUniverses()
{
    {
        QMutexLocker locker(&m_universeMutex);
        foreach(Universe* universe, m_universeMap)
            universe->reset();
    }

    /* Reset Grand Master parameters */
    setGrandMasterValue(255);
    setGrandMasterValueMode(GrandMaster::Reduce);
    setGrandMasterChannelMode(GrandMaster::Intensity);
}

/*********************************************************************
 * Grand Master
 *********************************************************************/

void InputOutputMap::setGrandMasterChannelMode(GrandMaster::ChannelMode mode)
{
    Q_ASSERT(m_grandMaster != NULL);

    if(m_grandMaster->channelMode() != mode)
    {
        m_grandMaster->setChannelMode(mode);
        m_universeChanged = true;
    }
}

GrandMaster::ChannelMode InputOutputMap::grandMasterChannelMode()
{
    Q_ASSERT(m_grandMaster != NULL);

    return m_grandMaster->channelMode();
}

void InputOutputMap::setGrandMasterValueMode(GrandMaster::ValueMode mode)
{
    Q_ASSERT(m_grandMaster != NULL);

    if(m_grandMaster->valueMode() != mode)
    {
        m_grandMaster->setValueMode(mode);
        m_universeChanged = true;
    }

    emit grandMasterValueModeChanged(mode);
}

GrandMaster::ValueMode InputOutputMap::grandMasterValueMode()
{
    Q_ASSERT(m_grandMaster != NULL);

    return m_grandMaster->valueMode();
}

void InputOutputMap::setGrandMasterValue(uchar value)
{
    Q_ASSERT(m_grandMaster != NULL);

    if (m_grandMaster->value() != value)
    {
        m_grandMaster->setValue(value);
        m_universeChanged = true;
    }

    if (m_universeChanged == true)
        emit grandMasterValueChanged(value);
}

uchar InputOutputMap::grandMasterValue()
{
    Q_ASSERT(m_grandMaster != NULL);

    return m_grandMaster->value();
}

/*********************************************************************
 * Patch
 *********************************************************************/

void InputOutputMap::flushInputs()
{
    QMutexLocker locker(&m_universeMutex);

    foreach (Universe* universe, m_universeMap)
    {
        universe->flushInput();
    }
}

bool InputOutputMap::setInputPatch(quint32 universeID, const QString &pluginName,
                                   quint32 input, const QString &profileName)
{
    QMutexLocker locker(&m_universeMutex);

    /* Check that the universe that we're doing mapping for is valid */
    if (!m_universeMap.contains(universeID))
    {
        qWarning() << Q_FUNC_INFO << "Universe" << universeID << "out of bounds.";
        return false;
    }

    Universe* universe = m_universeMap.value(universeID);
    InputPatch *currInPatch = universe->inputPatch();
    QLCInputProfile *currProfile = NULL;
    if (currInPatch != NULL)
    {
        currProfile = currInPatch->profile();
        disconnect(currInPatch, SIGNAL(inputValueChanged(quint32,quint32,uchar,const QString&)),
                this, SIGNAL(inputValueChanged(quint32,quint32,uchar,const QString&)));
    }
    InputPatch *ip = NULL;

    if (universe->setInputPatch(
                doc()->ioPluginCache()->plugin(pluginName), input,
                profile(profileName)) == true)
    {
        ip = universe->inputPatch();
        if (ip != NULL)
            connect(ip, SIGNAL(inputValueChanged(quint32,quint32,uchar,const QString&)),
                    this, SIGNAL(inputValueChanged(quint32,quint32,uchar,const QString&)));
    }
    else
    {
        return false;
    }

    if (ip != NULL && currProfile != ip->profile())
        emit profileChanged(universeID, ip->profileName());

    return true;
}

bool InputOutputMap::setInputProfile(quint32 universe, const QString &profileName)
{
    /* Check that the universe that we're doing mapping for is valid */
    if (!m_universeMap.contains(universe))
    {
        qWarning() << Q_FUNC_INFO << "Universe" << universe << "out of bounds.";
        return false;
    }

    InputPatch *currInPatch = m_universeMap.value(universe)->inputPatch();
    if (currInPatch == NULL)
        return false;

    return currInPatch->set(profile(profileName));
}

bool InputOutputMap::setOutputPatch(quint32 universe, const QString &pluginName,
                                    quint32 output, bool isFeedback)
{
    /* Check that the universe that we're doing mapping for is valid */
    if (!m_universeMap.contains(universe))
    {
        qWarning() << Q_FUNC_INFO << "Universe" << universe << "out of bounds.";
        return false;
    }

    QMutexLocker locker(&m_universeMutex);
    if (isFeedback == false)
        return m_universeMap.value(universe)->setOutputPatch(
                    doc()->ioPluginCache()->plugin(pluginName), output);
    else
        return m_universeMap.value(universe)->setFeedbackPatch(
                    doc()->ioPluginCache()->plugin(pluginName), output);

    return false;
}

InputPatch *InputOutputMap::inputPatch(quint32 universe) const
{
    if (!m_universeMap.contains(universe))
    {
        qWarning() << Q_FUNC_INFO << "Universe" << universe << "out of bounds.";
        return NULL;
    }
    return m_universeMap.value(universe)->inputPatch();
}

OutputPatch *InputOutputMap::outputPatch(quint32 universe) const
{
    if (!m_universeMap.contains(universe))
    {
        qWarning() << Q_FUNC_INFO << "Universe" << universe << "out of bounds.";
        return NULL;
    }
    return m_universeMap.value(universe)->outputPatch();
}

OutputPatch *InputOutputMap::feedbackPatch(quint32 universe) const
{
    if (!m_universeMap.contains(universe))
    {
        qWarning() << Q_FUNC_INFO << "Universe" << universe << "out of bounds.";
        return NULL;
    }
    return m_universeMap.value(universe)->feedbackPatch();
}

QMap<quint32, QString> InputOutputMap::universeNames() const
{
    QMap<quint32, QString> map;
    foreach(Universe* universe, m_universeMap)
        map.insert(universe->id(), universe->name());

    return map;
}

quint32 InputOutputMap::inputMapping(const QString &pluginName, quint32 input) const
{
    foreach (Universe* universe, m_universeMap)
    {
        const InputPatch* p = universe->inputPatch();
        if (p != NULL && p->pluginName() == pluginName && p->input() == input)
            return universe->id();
    }

    return invalidUniverse();
}

quint32 InputOutputMap::outputMapping(const QString &pluginName, quint32 output) const
{
    foreach (Universe* universe, m_universeMap)
    {
        const OutputPatch* p = universe->outputPatch();
        if (p != NULL && p->pluginName() == pluginName && p->output() == output)
            return universe->id();
    }

    return invalidUniverse();
}

/*****************************************************************************
 * Plugins
 *****************************************************************************/

QString InputOutputMap::pluginDescription(const QString &pluginName)
{
    QLCIOPlugin* plugin = NULL;

    if (pluginName.isEmpty() == false)
        plugin = doc()->ioPluginCache()->plugin(pluginName);

    if (plugin != NULL)
    {
        return plugin->pluginInfo();
    }
    else
        return "";
}

QStringList InputOutputMap::inputPluginNames()
{
    QStringList list;
    QListIterator <QLCIOPlugin*> it(doc()->ioPluginCache()->plugins());
    while (it.hasNext() == true)
    {
        QLCIOPlugin* plg(it.next());
        if (plg->capabilities() & QLCIOPlugin::Input)
            list << plg->name();
    }
    return list;
}

QStringList InputOutputMap::outputPluginNames()
{
    QStringList list;
    QListIterator <QLCIOPlugin*> it(doc()->ioPluginCache()->plugins());
    while (it.hasNext() == true)
    {
        QLCIOPlugin* plg(it.next());
        if (plg->capabilities() & QLCIOPlugin::Output)
            list << plg->name();
    }
    return list;
}

QStringList InputOutputMap::pluginInputs(const QString& pluginName)
{
    QLCIOPlugin* ip = doc()->ioPluginCache()->plugin(pluginName);
    if (ip == NULL)
        return QStringList();
    else
        return ip->inputs();
}

QStringList InputOutputMap::pluginOutputs(const QString& pluginName)
{
    QLCIOPlugin* op = doc()->ioPluginCache()->plugin(pluginName);
    if (op == NULL)
        return QStringList();
    else
        return op->outputs();
}

bool InputOutputMap::pluginSupportsFeedback(const QString& pluginName)
{
    QLCIOPlugin* outputPlugin = doc()->ioPluginCache()->plugin(pluginName);
    if (outputPlugin != NULL)
        return (outputPlugin->capabilities() & QLCIOPlugin::Feedback) > 0;
    else
        return false;
}

void InputOutputMap::configurePlugin(const QString& pluginName)
{
    QLCIOPlugin* outputPlugin = doc()->ioPluginCache()->plugin(pluginName);
    if (outputPlugin != NULL)
        outputPlugin->configure();
}

bool InputOutputMap::canConfigurePlugin(const QString& pluginName)
{
    QLCIOPlugin* outputPlugin = doc()->ioPluginCache()->plugin(pluginName);
    if (outputPlugin != NULL)
        return outputPlugin->canConfigure();
    else
        return false;
}

QString InputOutputMap::inputPluginStatus(const QString& pluginName, quint32 input)
{
    QLCIOPlugin* inputPlugin = NULL;
    QString info;

    if (pluginName.isEmpty() == false)
        inputPlugin = doc()->ioPluginCache()->plugin(pluginName);

    if (inputPlugin != NULL)
    {
        info = inputPlugin->inputInfo(input);
    }
    else
    {
        /* Nothing selected */
        info += QString("<HTML><HEAD></HEAD><BODY>");
        info += QString("<H3>%1</H3>").arg(tr("Nothing selected"));
        info += QString("</BODY></HTML>");
    }

    return info;
}

QString InputOutputMap::outputPluginStatus(const QString& pluginName, quint32 output)
{
    QLCIOPlugin* outputPlugin = doc()->ioPluginCache()->plugin(pluginName);
    if (outputPlugin != NULL)
    {
        return outputPlugin->outputInfo(output);
    }
    else
    {
        QString info;
        info += QString("<HTML><HEAD></HEAD><BODY>");
        info += QString("<H3>%1</H3>").arg(tr("Nothing selected"));
        info += QString("</BODY></HTML>");
        return info;
    }
}

bool InputOutputMap::sendFeedBack(quint32 universe, quint32 channel, uchar value, const QString& key)
{
    if (!m_universeMap.contains(universe))
        return false;

    OutputPatch* patch = m_universeMap.value(universe)->feedbackPatch();

    if (patch != NULL && patch->isPatched())
    {
        patch->plugin()->sendFeedBack(universe, patch->output(), channel, value, key);
        return true;
    }
    else
    {
        return false;
    }
}

void InputOutputMap::slotPluginConfigurationChanged(QLCIOPlugin* plugin)
{
    QMutexLocker locker(&m_universeMutex);
    bool success = true;
    foreach (Universe* universe, m_universeMap)
    {
        OutputPatch* op = universe->outputPatch();

        if (op != NULL && op->plugin() == plugin)
        {
            /*success = */ op->reconnect();
        }

        InputPatch* ip = universe->inputPatch();

        if (ip != NULL && ip->plugin() == plugin)
        {
            /*success = */ ip->reconnect();
        }
    }
    locker.unlock();

    emit pluginConfigurationChanged(plugin->name(), success);
}

/*****************************************************************************
 * Profiles
 *****************************************************************************/

void InputOutputMap::loadProfiles(const QDir& dir)
{
    if (dir.exists() == false || dir.isReadable() == false)
        return;

    /* Go thru all found file entries and attempt to load an input
       profile from each of them. */
    QStringListIterator it(dir.entryList());
    while (it.hasNext() == true)
    {
        QLCInputProfile* prof;
        QString path;

        path = dir.absoluteFilePath(it.next());
        prof = QLCInputProfile::loader(path);
        if (prof != NULL)
        {
            /* Check for duplicates */
            if (profile(prof->name()) == NULL)
                addProfile(prof);
            else
                delete prof;
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unable to find an input profile from" << path;
        }
    }
}

QStringList InputOutputMap::profileNames()
{
    QStringList list;
    QListIterator <QLCInputProfile*> it(m_profiles);
    while (it.hasNext() == true)
        list << it.next()->name();
    return list;
}

QLCInputProfile* InputOutputMap::profile(const QString& name)
{
    QListIterator <QLCInputProfile*> it(m_profiles);
    while (it.hasNext() == true)
    {
        QLCInputProfile* profile = it.next();
        if (profile->name() == name)
            return profile;
    }

    return NULL;
}

bool InputOutputMap::addProfile(QLCInputProfile* profile)
{
    Q_ASSERT(profile != NULL);

    /* Don't add the same profile twice */
    if (m_profiles.contains(profile) == false)
    {
        m_profiles.append(profile);
        return true;
    }
    else
    {
        return false;
    }
}

bool InputOutputMap::removeProfile(const QString& name)
{
    QLCInputProfile* profile;
    QMutableListIterator <QLCInputProfile*> it(m_profiles);
    while (it.hasNext() == true)
    {
        profile = it.next();
        if (profile->name() == name)
        {
            it.remove();
            delete profile;
            return true;
        }
    }

    return false;
}

bool InputOutputMap::inputSourceNames(const QLCInputSource *src,
                                QString& uniName, QString& chName) const
{
    if (src == NULL || src->isValid() == false)
        return false;

    if (!m_universeMap.contains(src->universe()))
        return false;

    InputPatch* pat = m_universeMap.value(src->universe())->inputPatch();
    if (pat == NULL)
    {
        /* There is no patch for the given universe */
        uniName = QString("%1 -UNPATCHED-").arg(src->universe() + 1);

        ushort page = src->page();
        ushort channel = (src->channel() & 0x0000FFFF) + 1;

        if (page != 0)
            chName = QString("%1: ? (Page %2)").arg(channel).arg(page + 1);
        else
            chName = QString("%1: ?").arg(channel);
        return true;
    }

    QLCInputProfile* profile = pat->profile();
    if (profile == NULL)
    {
        /* There is no profile. Display plugin name and channel number. */
        if (pat->plugin() != NULL)
            uniName = QString("%1: %2").arg(src->universe() + 1).arg(pat->plugin()->name());
        else
            uniName = QString("%1: ??").arg(src->universe() + 1);

        ushort page = src->page();
        ushort channel = (src->channel() & 0x0000FFFF) + 1;

        if (page != 0)
            chName = QString("%1: ? (Page %2)").arg(channel).arg(page + 1);
        else
            chName = QString("%1: ?").arg(channel);
    }
    else
    {
        QLCInputChannel* ich;
        QString name;

        /* Display profile name for universe */
        uniName = QString("%1: %2").arg(src->universe() + 1).arg(profile->name());

        /* User can input the channel number by hand, so put something
           rational to the channel name in those cases as well. */
        ushort page = src->page();
        ushort channel = (src->channel() & 0x0000FFFF);

        ich = profile->channel(channel);
        if (ich != NULL)
            name = ich->name();
        else
            name = QString("?");

        /* Display channel name */
        if (page != 0)
            chName = QString("%1: %2 (Page %3)").arg(channel + 1).arg(name).arg(page + 1);
        else
            chName = QString("%1: %2").arg(channel + 1).arg(name);
    }

    return true;
}

bool InputOutputMap::inputSourceNames(QSharedPointer<QLCInputSource> const& src,
                                QString& uniName, QString& chName) const
{
    return inputSourceNames(src.data(), uniName, chName);
}

QDir InputOutputMap::systemProfileDirectory()
{
    return QLCFile::systemDirectory(QString(INPUTPROFILEDIR), QString(KExtInputProfile));
}

QDir InputOutputMap::userProfileDirectory()
{
    return QLCFile::userDirectory(QString(USERINPUTPROFILEDIR), QString(INPUTPROFILEDIR),
                                  QStringList() << QString("*%1").arg(KExtInputProfile));
}

/*********************************************************************
 * Defaults - !! FALLBACK !!
 *********************************************************************/

void InputOutputMap::loadDefaults()
{
    /* ************************ INPUT *********************************** */
    QSettings settings;
    QString plugin;
    QString input;
    QString key;

    for (quint32 i = 0; i < universesCount(); i++)
    {
        QString profileName;
        bool passthrough;

        /* Plugin name */
        key = QString("/inputmap/universe%1/plugin/").arg(i);
        plugin = settings.value(key).toString();

        /* Plugin input */
        key = QString("/inputmap/universe%1/input/").arg(i);
        input = settings.value(key).toString();

        /* Input profile */
        key = QString("/inputmap/universe%1/profile/").arg(i);
        profileName = settings.value(key).toString();

        key = QString("/inputmap/universe%1/passthrough/").arg(i);
        passthrough = settings.value(key).toBool();
        if (passthrough == true)
            m_universeMap.value(i)->setPassthrough(passthrough);

        /* Do the mapping */
        if (plugin != KInputNone && input != KInputNone)
            setInputPatch(i, plugin, input.toUInt(), profileName);
    }

    /* ************************ OUTPUT *********************************** */
    QString output;
    QString fb_plugin;
    QString feedback;

    for (quint32 i = 0; i < universesCount(); i++)
    {
        /* Plugin name */
        key = QString("/outputmap/universe%1/plugin/").arg(i);
        plugin = settings.value(key).toString();

        /* Plugin output */
        key = QString("/outputmap/universe%1/output/").arg(i);
        output = settings.value(key).toString();

        /* Feedback plugin name */
        key = QString("/outputmap/universe%1/feedbackplugin/").arg(i);
        fb_plugin = settings.value(key).toString();

        /* Feedback line */
        key = QString("/outputmap/universe%1/feedback/").arg(i);
        feedback = settings.value(key).toString();

        if (plugin != KOutputNone && output != KOutputNone)
            setOutputPatch(i, plugin, output.toUInt());

        if (fb_plugin != KOutputNone && feedback != KOutputNone)
            setOutputPatch(i, fb_plugin, feedback.toUInt(), true);
    }
}

void InputOutputMap::saveDefaults()
{
    /* ************************ INPUT *********************************** */
    QSettings settings;
    QString key;

    for (quint32 i = 0; i < universesCount(); i++)
    {
        InputPatch* inPatch = inputPatch(i);

        /* Plugin name */
        key = QString("/inputmap/universe%1/plugin/").arg(i);
        if (inPatch != NULL)
            settings.setValue(key, inPatch->pluginName());
        else
            settings.setValue(key, KInputNone);

        /* Plugin input */
        key = QString("/inputmap/universe%1/input/").arg(i);
        if (inPatch != NULL)
            settings.setValue(key, QString::number(inPatch->input()));
        else
            settings.setValue(key, KInputNone);

        /* Input profile */
        key = QString("/inputmap/universe%1/profile/").arg(i);
        if (inPatch != NULL)
            settings.setValue(key, inPatch->profileName());
        else
            settings.setValue(key, KInputNone);

        /* Passthrough */
        key = QString("/inputmap/universe%1/passthrough/").arg(i);
        bool passthrough = m_universeMap.value(i)->passthrough();
        if (passthrough == true)
            settings.setValue(key, passthrough);
        else
            settings.remove(key);
    }

    /* ************************ OUTPUT *********************************** */

    for (quint32 i = 0; i < universesCount(); i++)
    {
        OutputPatch* outPatch = outputPatch(i);
        OutputPatch* fbPatch = feedbackPatch(i);

        key = QString("/outputmap/universe%1/plugin/").arg(i);

        /* Plugin name */
        if (outPatch != NULL)
            settings.setValue(key, outPatch->pluginName());
        else
            settings.setValue(key, KOutputNone);

        /* Plugin output */
        key = QString("/outputmap/universe%1/output/").arg(i);
        if (outPatch != NULL)
            settings.setValue(key, outPatch->output());
        else
            settings.setValue(key, KOutputNone);

        key = QString("/outputmap/universe%1/feedbackplugin/").arg(i);

        /* Feedback plugin name */
        if (fbPatch != NULL)
            settings.setValue(key, fbPatch->pluginName());
        else
            settings.setValue(key, KOutputNone);

        /* Feedback plugin output */
        key = QString("/outputmap/universe%1/feedback/").arg(i);
        if (fbPatch != NULL)
            settings.setValue(key, QString::number(fbPatch->output()));
        else
            settings.setValue(key, KOutputNone);
    }
}

bool InputOutputMap::loadXML(QXmlStreamReader &root)
{
    if (root.name() != KXMLIOMap)
    {
        qWarning() << Q_FUNC_INFO << "InputOutputMap node not found";
        return false;
    }

    /** Reset the current universe list and read the new one */
    removeAllUniverses();

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCUniverse)
        {
            quint32 id = InputOutputMap::invalidUniverse();
            if (root.attributes().hasAttribute(KXMLQLCUniverseID))
                id = root.attributes().value(KXMLQLCUniverseID).toString().toUInt();
            addUniverse(id);
            Universe *uni = m_universeMap.value(id);
            uni->loadXML(root, this);
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unknown IO Map tag:" << root.name();
            root.skipCurrentElement();
        }
    }

    return true;
}

bool InputOutputMap::saveXML(QXmlStreamWriter *doc) const
{
    Q_ASSERT(doc != NULL);

    /* IO Map Instance entry */
    doc->writeStartElement(KXMLIOMap);

    foreach(Universe *uni, m_universeMap)
        uni->saveXML(doc);

    doc->writeEndElement();

    return true;
}


