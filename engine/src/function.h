/*
  Q Light Controller Plus
  function.h

  Copyright (C) 2004 Heikki Junnila
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

#ifndef FUNCTION_H
#define FUNCTION_H

#include <QWaitCondition>
#include <QObject>
#include <QString>
#include <QMutex>
#include <QList>
#include <QIcon>

#include "universe.h"
#include "functionparent.h"
#include "functionspeeds.h"

class QXmlStreamReader;

class GenericFader;
class MasterTimer;
class Function;
class Doc;

class FunctionUiState;

/** @addtogroup engine_functions Functions
 * @{
 */

#define KXMLQLCFunction "Function"
#define KXMLQLCFunctionName "Name"
#define KXMLQLCFunctionID "ID"
#define KXMLQLCFunctionType "Type"
#define KXMLQLCFunctionData "Data"
#define KXMLQLCFunctionPath "Path"
#define KXMLQLCFunctionBlendMode "BlendMode"

#define KXMLQLCFunctionValue "Value"
#define KXMLQLCFunctionValueType "Type"
#define KXMLQLCFunctionChannel "Channel"

#define KXMLQLCFunctionStep "Step"
#define KXMLQLCFunctionNumber "Number"

#define KXMLQLCFunctionDirection "Direction"
#define KXMLQLCFunctionRunOrder "RunOrder"

#define KXMLQLCFunctionEnabled "Enabled"


typedef struct
{
    QString name;
    qreal value;
} Attribute;

class Function : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Function)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(quint32 id READ id CONSTANT)
    Q_PROPERTY(Type type READ type CONSTANT)

public:
    /**
     * All known function types.
     * This is a bit mask to facilitate easy AND-mode type filtering
     */
    enum Type
    {
        Undefined  = 0,
        Scene      = 1 << 0,
        Chaser     = 1 << 1,
        EFX        = 1 << 2,
        Collection = 1 << 3,
        Script     = 1 << 4,
        RGBMatrix  = 1 << 5,
        Show       = 1 << 6,
        Audio      = 1 << 7
#if QT_VERSION >= 0x050000
        , Video    = 1 << 8
#endif
    };
    Q_ENUMS(Type)

    /**
     * Common attributes
     */
    enum Attr
    {
        Intensity = 0
    };

    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    /** Create a new function instance with the given QObject parent. */
    Function(QObject* parent = 0);

    /**
     * Create a new function
     *
     * @param doc The parent object that owns this function (Doc)
     * @param t The function type (see enum Type)
     */
    Function(Doc* doc, Type t);

    /**
     * Destroy this function
     */
    virtual ~Function();

    /** Get the parent Doc object */
    Doc* doc() const;

signals:
    /** Signal telling that the contents of this function have changed */
    void changed(quint32 fid);

    /*********************************************************************
     * Copying
     *********************************************************************/
public:
    /**
     * Create a copy of a function to the given doc object and return the
     * newly-created function, unless an error has occurred.
     *
     * @param doc The QLC Doc object that owns all functions
     * @param addToDoc enable/disable addition of the function copy to Doc
     * @return The newly-created function or NULL in case of an error
     */
    virtual Function* createCopy(Doc* doc, bool addToDoc = true);

    /**
     * Copy this function's contents from the given function. Finally emits
     * the changed() signal so don't emit it from subclasses' copyFrom()
     * implementations, but call this one just before return.
     *
     * @param function The function to copy from
     * @return true if successful, otherwise false
     */
    virtual bool copyFrom(const Function* function);

    /*********************************************************************
     * ID
     *********************************************************************/
public:
    /**
     * Set this function's unique ID
     *
     * @param id This function's unique ID
     */
    void setID(quint32 id);

    /**
     * Get this function's unique ID
     */
    quint32 id() const;

    /**
     * Get the value for an invalid function ID (for comparison etc.)
     */
    static quint32 invalidId();

private:
    quint32 m_id;

    /*********************************************************************
     * Name
     *********************************************************************/
public:
    /**
     * Set a name for this function
     *
     * @param name The function's new name
     */
    void setName(const QString& name);

    /**
     * Return the name of this function
     */
    QString name() const;

signals:
    /** Signal telling that the name of this function have changed */
    void nameChanged(quint32 fid);

private:
    QString m_name;

    /*********************************************************************
     * Type
     *********************************************************************/
public:
    /**
     * Return the type of this function (see the enum above)
     */
    Type type() const;

    /**
     * Return the type of this function as a string
     */
    QString typeString() const;

    /**
     * Convert a type to string
     *
     * @param type The type to convert
     */
    static QString typeToString(Function::Type type);

    /**
     * Convert a string to Type
     *
     * @param str The string to convert
     */
    static Type stringToType(const QString& str);

    /**
     * Convert a type to an icon
     *
     * @param type The type to convert
     */
    static QIcon typeToIcon(Function::Type type);

private:
    Type m_type;

    /*********************************************************************
     * Path
     *********************************************************************/
public:
    /** Set the function path for folder grouping */
    void setPath(QString path);

    /** Retrieve the currently set path */
    QString path(bool simplified = false) const;

private:
    QString m_path;

    /*********************************************************************
     * Common XML
     *********************************************************************/
protected:
    /** Save function's common attributes in $doc, under $root */
    bool saveXMLCommon(QXmlStreamWriter *doc) const;

    /*********************************************************************
     * Running order
     *********************************************************************/
public:
    enum RunOrder { Loop = 0, SingleShot, PingPong, Random };
    Q_ENUMS(RunOrder)

public:
    /**
     * Set this function's running order
     *
     * @param dir This function's running order
     */
    void setRunOrder(const Function::RunOrder& order);

    /**
     * Get this function's running order
     */
    Function::RunOrder runOrder() const;

    /**
     * Convert a RunOrder to string
     *
     * @param order RunOrder to convert
     */
    static QString runOrderToString(const Function::RunOrder& order);

    /**
     * Convert a string to RunOrder
     *
     * @param str The string to convert
     */
    static Function::RunOrder stringToRunOrder(const QString& str);

protected:
    /** Save function's running order in $doc */
    bool saveXMLRunOrder(QXmlStreamWriter *doc) const;

    /** Load function's direction from $root */
    bool loadXMLRunOrder(QXmlStreamReader &root);

private:
    RunOrder m_runOrder;

    /*********************************************************************
     * Direction
     *********************************************************************/
public:
    enum Direction { Forward = 0, Backward };
    Q_ENUMS(Direction)

public:
    /**
     * Set this function's direction
     *
     * @param dir This function's direction
     */
    void setDirection(const Function::Direction& dir);

    /**
     * Get this function's direction
     */
    Function::Direction direction() const;

    /**
     * Convert a Direction to a string
     *
     * @param dir Direction to convert
     */
    static QString directionToString(const Function::Direction& dir);

    /**
     * Convert a string to Direction
     *
     * @param str The string to convert
     */
    static Function::Direction stringToDirection(const QString& str);

protected:
    /** Save function's direction in $doc */
    bool saveXMLDirection(QXmlStreamWriter *doc) const;

    /** Load function's direction from $root */
    bool loadXMLDirection(QXmlStreamReader &root);

private:
    Direction m_direction;

    /**
     * This slot is connected to the Master Timer and it is invoked
     * when this Function is in 'Beats' tempo type and the BPM
     * number changed. Subclasses should reimplement this.
     */
    virtual void slotBPMChanged(int bpmNumber);

private:
    bool m_beatResyncNeeded;

    /*********************************************************************
     * Speed
     *********************************************************************/
public:
    enum SpeedsType { FadeIn = 0, Hold, FadeOut, Duration };
    Q_ENUMS(SpeedsType)

public:
    /*********************************************************************
     * Speeds
     *********************************************************************/
    void emitChanged();

    class FunctionSpeedsEditProxy
    {
      private:
        FunctionSpeeds& m_speeds;
        Function& m_function;
        bool m_changed;

      public:
        FunctionSpeedsEditProxy(FunctionSpeeds &speeds, Function &function)
            : m_speeds(speeds), m_function(function), m_changed(false) {}
        ~FunctionSpeedsEditProxy()
        {
            if (m_changed)
                m_function.emitChanged();
        }
        FunctionSpeedsEditProxy& setFadeIn(quint32 ms)
        {
            m_speeds.setFadeIn(ms);
            m_changed = true;
            return *this;
        }
        FunctionSpeedsEditProxy& setFadeOut(quint32 ms)
        {
            m_speeds.setFadeOut(ms);
            m_changed = true;
            return *this;
        }
        FunctionSpeedsEditProxy& setHold(quint32 ms)
        {
            m_speeds.setHold(ms);
            m_changed = true;
            return *this;
        }
        FunctionSpeedsEditProxy& setDuration(quint32 ms)
        {
            m_speeds.setDuration(ms);
            m_changed = true;
            return *this;
        }
    };

public:
    void setSpeeds(FunctionSpeeds const& speeds);
    FunctionSpeeds const& speeds() const;
    FunctionSpeedsEditProxy speedsEdit();

    void setOverrideSpeeds(FunctionSpeeds const& speeds);
    FunctionSpeeds const& overrideSpeeds() const;
    FunctionSpeeds& overrideSpeeds();

    /** Tell the function that it has been "tapped". Default implementation does nothing. */
    virtual void tap();

    // TODO alternate speeds
    // replace overrideblabla by alternate.
    // Alternate idx0 is the "global" speed of the function.
    // virtual quint32 getNum__SUBSPEEDPH__SpeedsCount() const;
    // virtual void set__SUBSPEEDPH__Speeds(quint32 alternateIdx, FunctionSpeeds const& speeds);
    // virtual void set__SUBSPEEDPH__FadeIn(quint32 alternateIdx, quint32 ms);
    // virtual void set__SUBSPEEDPH__Hold(quint32 alternateIdx, quint32 ms);
    // virtual void set__SUBSPEEDPH__FadeOut(quint32 alternateIdx, quint32 ms);
    // virtual void set__SUBSPEEDPH__Duration(quint32 alternateIdx, quint32 ms);
    // virtual void get__SUBSPEEDPH__String(quint32 alternateIdx) const;

protected:
    FunctionSpeeds m_speeds;
    FunctionSpeeds m_overrideSpeeds;

    /*********************************************************************
     * UI State
     *********************************************************************/
public:
    FunctionUiState * uiState();
    const FunctionUiState * uiState() const;

private:
    virtual FunctionUiState * createUiState();

private:
    FunctionUiState * m_uiState;

    /*********************************************************************
     * Fixtures
     *********************************************************************/
public slots:
    /** Slot that captures Doc::fixtureRemoved signals */
    virtual void slotFixtureRemoved(quint32 fxi_id);

    /*********************************************************************
     * Load & Save
     *********************************************************************/
public:
    /**
     * Save this function to an XML document
     *
     * @param doc The XML document to save to
     */
    virtual bool saveXML(QXmlStreamWriter *doc);

    /**
     * Read this function's contents from an XML document
     *
     * @param doc An XML document to load from
     * @param root An XML root element of a function
     */
    virtual bool loadXML(QXmlStreamReader &root);

    /**
     * Load a new function from an XML tag and add it to the given doc
     * object, if loading was successful.
     *
     * @param root An XML root element of a function
     * @param doc The QLC document object, that owns all functions
     * @return true if successful, otherwise false
     */
    static bool loader(QXmlStreamReader &root, Doc* doc);

    /**
     * Called for each Function-based object after everything has been loaded.
     * Do any post-load cleanup, function mappings etc. if needed. Default
     * implementation does nothing.
     */
    virtual void postLoad();

    /*********************************************************************
     * Flash
     *********************************************************************/
public:
    /** Flash the function */
    virtual void flash(MasterTimer* timer);

    /** UnFlash the function */
    virtual void unFlash(MasterTimer* timer);

    /** Check, whether the function is flashing */
    virtual bool flashing() const;

signals:
    /**
     * Tells listeners that this function is flashing or that is just
     * stopped flashing.
     *
     * @param fid The flashing function's ID
     * @param state true if the function flashing, false if the function
     *              just stopped flashing
     */
    void flashing(quint32 fid, bool state);

private:
    bool m_flashing;

    /*********************************************************************
     * Running
     *********************************************************************/
public:
    /**
     * Called by MasterTimer when the function is started. MasterTimer's
     * function list mutex is locked during this call, so functions must
     * not attempt to start/stop additional functions from their preRun()
     * methods because it would result in a deadlock.
     *
     * @param timer The MasterTimer instance that takes care of running
     *              the function in correct intervals.
     */
    virtual void preRun(MasterTimer* timer);

    /**
     * Write next values to universes. This method is called periodically
     * by the MasterTimer instance that the
     * function has been set to run in, to write the next values to the
     * given DMX universe buffer. This method is called for each function
     * in the order that they were set to run. When this method has been
     * called for each running function, the buffer is written to OutputMap.
     *
     * MasterTimer calls this method for each function to get their DMX
     * data for the given array of universes. This method will be called
     * for each running function until Function::stopped() returns true.
     *
     * @param timer The MasterTimer that is running the function
     * @param universes The DMX universe buffer to write values into
     */
    virtual void write(MasterTimer* timer, QList<Universe*> universes);

    /**
     * Called by MasterTimer when the function is stopped. No more write()
     * calls will arrive to the function after this call. The function may
     * still write its last data packet to $universes during this call or
     * hand over channel zero-fading to $timer's generic fader.
     *
     * MasterTimer's function list mutex is locked during this call, so
     * functions must not attempt to start/stop additional functions from
     * their postRun() method as it would result in a deadlock situation.
     *
     * @param timer The MasterTimer that has stopped running the function
     * @param universes Universe buffer to write the function's exit data
     */
    virtual void postRun(MasterTimer* timer, QList<Universe*> universes);

signals:
    /**
     * Emitted when a function is started (i.e. added to MasterTimer's
     * list of running functions).
     *
     * @param id The ID of the started function
     */
    void running(quint32 id);

    /**
     * Emitted when a function is really finished (i.e. removed from
     * MasterTimer's list of running functions).
     *
     * @param id The ID of the stopped function
     */
    void stopped(quint32 id);

    /*********************************************************************
     * Elapsed
     *********************************************************************/
public:
    /**
     * Get number of elapsed ticks for this function (0 unless the function
     * is running).
     *
     * @return Number of elapsed timer ticks since the function was started
     */
    quint32 elapsed() const;

    quint32 elapsedBeats() const;

protected:
    /** Reset elapsed timer ticks to zero */
    void resetElapsed();

    /** Increment the elapsed timer ticks by one */
    void incrementElapsed();

    /** Increment the elapsed beats by one */
    void incrementElapsedBeats();

    void roundElapsed(quint32 roundTime);

private:
    /* The elapsed time in ms when tempoType is Time */
    quint32 m_elapsed;
    /* The elapsed beats when tempoType is Beats */
    quint32 m_elapsedBeats;

    /*********************************************************************
     * Start & Stop
     *********************************************************************/
public:
    /**
     * Start running the function in the given MasterTimer instance.
     *
     * @param timer The MasterTimer that should run the function
     * @param child Use true if called from another function
     * @param overrideSpeeds Override the function's default speeds
     * @param overrideTempoType Override the tempo type of the function
     */
    void start(MasterTimer* timer, FunctionParent parent, quint32 startTime = 0,
               FunctionSpeeds const& overrideSpeeds = FunctionSpeeds());

    /**
     * Pause a running Function. Subclasses should check the paused state
     * immediately in the write call and, in case, return, to avoid performing
     * any action and moreover to increment the elapsed time.
     * This is declared as virtual for those subclasses where the actual
     * Function progress is handled in a separate thread, like multimedia
     * functions, or where the Function handles a few children Functions.
     */
    virtual void setPause(bool enable);

    /**
     * Mark the function to be stopped ASAP. MasterTimer will stop running
     * the function on the next pass after this method has been called.
     * There is no way to cancel it, but the function can be started again
     * normally.
     */
    void stop(FunctionParent parent);

    /**
     * Check, whether the function should be stopped ASAP. Functions can use this
     * to check, whether they should continue running or bail out.
     *
     * @return true if the function should be stopped, otherwise false.
     */
    bool stopped() const;

    /**
     * Mark the function to be stopped and block the calling thread until it is
     * actually stopped. To prevent deadlocks the function only waits for 2s.
     *
     * @return true if the function was stopped. false if the function did not
     *              stop withing two seconds
     */
    bool stopAndWait();

    /**
     * Check, whether the function is currently running (preRun() has been run)
     * or not (postRun() has been completed). This should be used only from MasterTimer.
     *
     * @return true if function is running, false if not
     */
    bool isRunning() const;

    /**
     * Check if the function is currently in a paused state. This is invoked
     * by subclasses to understand if they have to do something during the
     * write call
     */
    bool isPaused() const;

    bool startedAsChild() const;

private:
    /** Running state flags. The rules are:
     *  - m_stop resets also m_paused
     *  - m_paused and m_running can be both true
     *  - if m_paused is true, a start(...) call will just reset it to false
     *  These are private to prevent Functions from modifying them. */
    bool m_stop;
    bool m_running;
    bool m_paused;

    QList<FunctionParent> m_sources;
    QMutex m_sourcesMutex;

    QMutex m_stopMutex;
    QWaitCondition m_functionStopped;

    /*************************************************************************
     * Attributes
     *************************************************************************/
public:
    /**
     * Register a new attribute for this function.
     * If the attribute already exists, it will be overwritten.
     *
     * @param name The attribute name
     * @param value The attribute initial value
     */
    int registerAttribute(QString name, qreal value = 1.0);

    /**
     * Unregister a previously created attribute for this function.
     * If the attribute doesn't exist, false will be returned.
     *
     * @param name The attribute name
     */
    bool unregisterAttribute(QString name);

    /**
     * Rename an existing atribute
     *
     * @param idx the attribute index
     * @param newName the new name for the attribute
     * @return true on success, false on error
     */
    bool renameAttribute(int idx, QString newName);

    /**
     * Adjust the intensity of the function by a fraction.
     *
     * @param fraction Intensity as a fraction (0.0 - 1.0)
     */
    virtual void adjustAttribute(qreal fraction, int attributeIndex);

    /**
     * Reset intensity to the default value (1.0).
     */
    void resetAttributes();

    /**
     * Get a specific function attribute by index
     *
     * @param attributeIndex the attribute index
     * @return the requested attribute value (on error return 0.0)
     */
    qreal getAttributeValue(int attributeIndex) const;

    /**
     * Get a specific function attribute by name
     *
     * @param name the attribute name
     * @return the requested attribute index
     */
    int getAttributeIndex(QString name) const;

    /**
     * Get the function's attributes
     *
     * @return a list of Attributes
     */
    QList <Attribute> attributes();

signals:
    /** Informs that an attribute of the function has changed */
    void attributeChanged(int index, qreal fraction);

private:
    QList <Attribute> m_attributes;

public:
    virtual bool contains(quint32 functionId) const;

    /*************************************************************************
     * Blend mode
     *************************************************************************/
public:
    /**
     * Set a specific blend mode to be used by this Function
     * For now this is used only by RGBMatrix but it's been
     * placed here for generic usage
     */
    virtual void setBlendMode(Universe::BlendMode mode);

    /**
     * Return the blend mode set on this Function
     */
    virtual Universe::BlendMode blendMode() const;

private:
    Universe::BlendMode m_blendMode;
};

/** @} */

#endif
