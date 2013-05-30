/*  Copyright (C) 2008 e_k (e_k@users.sourceforge.net)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QLayout>
#include <QBoxLayout>
#include <QLayoutItem>
#include <QSizePolicy>
#include "SearchBar.h"
#include "qtermwidget.h"
#include "ColorTables.h"

#include "Session.h"
#include "Screen.h"
#include "ScreenWindow.h"
#include "Emulation.h"
#include "History.h"
#include "TerminalDisplay.h"
#include "KeyboardTranslator.h"
#include "ColorScheme.h"
#include "SearchBar.h"

#define STEP_ZOOM 3

using namespace Konsole;

void *createTermWidget(int startnow, void *parent)
{
    return (void*) new QTermWidget(startnow, (QWidget*)parent);
}

struct TermWidgetImpl {
    TermWidgetImpl(QWidget* parent = 0);

    TerminalDisplay *m_terminalDisplay;
    Session *m_session;

    Session* createSession();
    TerminalDisplay* createTerminalDisplay(Session *session, QWidget* parent);
};

TermWidgetImpl::TermWidgetImpl(QWidget* parent)
{
    this->m_session = createSession();
    this->m_terminalDisplay = createTerminalDisplay(this->m_session, parent);
}


Session *TermWidgetImpl::createSession()
{
    Session *session = new Session();

    session->setTitle(Session::NameRole, "QTermWidget");

    /* Thats a freaking bad idea!!!!
     * /bin/bash is not there on every system
     * better set it to the current $SHELL
     * Maybe you can also make a list available and then let the widget-owner decide what to use.
     * By setting it to $SHELL right away we actually make the first filecheck obsolete.
     * But as iam not sure if you want to do anything else ill just let both checks in and set this to $SHELL anyway.
     */
    //session->setProgram("/bin/bash");

    session->setProgram(getenv("SHELL"));


    QStringList args("");
    session->setArguments(args);
    session->setAutoClose(true);

    session->setCodec(QTextCodec::codecForName("UTF-8"));

    session->setFlowControlEnabled(true);
    session->setHistoryType(HistoryTypeBuffer(1000));

    session->setDarkBackground(true);

    session->setKeyBindings("");
    return session;
}

TerminalDisplay *TermWidgetImpl::createTerminalDisplay(Session *session, QWidget* parent)
{
//    TerminalDisplay* display = new TerminalDisplay(this);
    TerminalDisplay* display = new TerminalDisplay(parent);

    display->setBellMode(TerminalDisplay::NotifyBell);
    display->setTerminalSizeHint(true);
    display->setTripleClickMode(TerminalDisplay::SelectWholeLine);
    display->setTerminalSizeStartup(true);

    display->setRandomSeed(session->sessionId() * 31);

    return display;
}


QTermWidget::QTermWidget(int startnow, QWidget *parent)
    : QWidget(parent)
{
    init(startnow);
}

QTermWidget::QTermWidget(QWidget *parent)
    : QWidget(parent)
{
    init(1);
}

void QTermWidget::selectionChanged(bool textSelected)
{
    emit copyAvailable(textSelected);
}

void QTermWidget::find()
{
    search(true, false);
}

void QTermWidget::findNext()
{
    search(true, true);
}

void QTermWidget::findPrevious()
{
    search(false, false);
}

void QTermWidget::search(bool forwards, bool next)
{
    int startColumn, startLine;
    
    if (next) // search from just after current selection
    {
        m_impl->m_terminalDisplay->screenWindow()->screen()->getSelectionEnd(startColumn, startLine);
        startColumn++;
    }
    else // search from start of current selection
    {
        m_impl->m_terminalDisplay->screenWindow()->screen()->getSelectionStart(startColumn, startLine);
    }
   
    qDebug() << "current selection starts at: " << startColumn << startLine;
    qDebug() << "current cursor position: " << m_impl->m_terminalDisplay->screenWindow()->cursorPosition(); 

    QRegExp regExp(m_searchBar->searchText());
    regExp.setPatternSyntax(m_searchBar->useRegularExpression() ? QRegExp::RegExp : QRegExp::FixedString);
    regExp.setCaseSensitivity(m_searchBar->matchCase() ? Qt::CaseSensitive : Qt::CaseInsensitive);

    HistorySearch *historySearch = 
            new HistorySearch(m_impl->m_session->emulation(), regExp, forwards, startColumn, startLine, this);
    connect(historySearch, SIGNAL(matchFound(int, int, int, int)), this, SLOT(matchFound(int, int, int, int)));
    connect(historySearch, SIGNAL(noMatchFound()), this, SLOT(noMatchFound()));
    connect(historySearch, SIGNAL(noMatchFound()), m_searchBar, SLOT(noMatchFound()));
    historySearch->search();
}


void QTermWidget::matchFound(int startColumn, int startLine, int endColumn, int endLine)
{
    ScreenWindow* sw = m_impl->m_terminalDisplay->screenWindow();
    qDebug() << "Scroll to" << startLine;
    sw->scrollTo(startLine);
    sw->setTrackOutput(false);
    sw->notifyOutputChanged();
    sw->setSelectionStart(startColumn, startLine - sw->currentLine(), false);
    sw->setSelectionEnd(endColumn, endLine - sw->currentLine());
}

void QTermWidget::noMatchFound() 
{
        m_impl->m_terminalDisplay->screenWindow()->clearSelection();
}

int QTermWidget::getShellPID()
{
    return m_impl->m_session->processId();
}

void QTermWidget::changeDir(const QString & dir)
{
    /*
       this is a very hackish way of trying to determine if the shell is in
       the foreground before attempting to change the directory.  It may not
       be portable to anything other than Linux.
    */
    QString strCmd;
    strCmd.setNum(getShellPID());
    strCmd.prepend("ps -j ");
    strCmd.append(" | tail -1 | awk '{ print $5 }' | grep -q \\+");
    int retval = system(strCmd.toStdString().c_str());

    if (!retval) {
        QString cmd = "cd " + dir + "\n";
        sendText(cmd);
    }
}

QSize QTermWidget::sizeHint() const
{
    QSize size = m_impl->m_terminalDisplay->sizeHint();
    size.rheight() = 150;
    return size;
}

void QTermWidget::startShellProgram()
{
    if ( m_impl->m_session->isRunning() ) {
        return;
    }

    m_impl->m_session->run();
}

void QTermWidget::init(int startnow)
{
    m_layout = new QVBoxLayout();
    m_layout->setMargin(0);
    setLayout(m_layout);
    
    m_impl = new TermWidgetImpl(this);
    m_impl->m_terminalDisplay->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_layout->addWidget(m_impl->m_terminalDisplay);

    m_searchBar = new SearchBar(this);
    m_searchBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    connect(m_searchBar, SIGNAL(searchCriteriaChanged()), this, SLOT(find()));
    connect(m_searchBar, SIGNAL(findNext()), this, SLOT(findNext()));
    connect(m_searchBar, SIGNAL(findPrevious()), this, SLOT(findPrevious()));
    m_layout->addWidget(m_searchBar);
    m_searchBar->hide();

    if (startnow && m_impl->m_session) {
        m_impl->m_session->run();
    }

    this->setFocus( Qt::OtherFocusReason );
    this->setFocusPolicy( Qt::WheelFocus );
    m_impl->m_terminalDisplay->resize(this->size());

    this->setFocusProxy(m_impl->m_terminalDisplay);
    connect(m_impl->m_terminalDisplay, SIGNAL(copyAvailable(bool)),
            this, SLOT(selectionChanged(bool)));
    connect(m_impl->m_terminalDisplay, SIGNAL(termGetFocus()),
            this, SIGNAL(termGetFocus()));
    connect(m_impl->m_terminalDisplay, SIGNAL(termLostFocus()),
            this, SIGNAL(termLostFocus()));
    connect(m_impl->m_terminalDisplay, SIGNAL(keyPressedSignal(QKeyEvent *)),
            this, SIGNAL(termKeyPressed(QKeyEvent *)));

    QFont font = QApplication::font();
    font.setFamily("Monospace");
    font.setPointSize(10);
    font.setStyleHint(QFont::TypeWriter);
    setTerminalFont(font);
    m_searchBar->setFont(font);

    setScrollBarPosition(NoScrollBar);

    m_impl->m_session->addView(m_impl->m_terminalDisplay);
    connect( m_impl->m_terminalDisplay, SIGNAL(updated(QRect)), this, SIGNAL(updated(QRect)) );
    connect(m_impl->m_session, SIGNAL(finished()), this, SLOT(sessionFinished()));
}


QTermWidget::~QTermWidget()
{
    emit destroyed();
}


void QTermWidget::setTerminalFont(QFont &font)
{
    if (!m_impl->m_terminalDisplay)
        return;
    m_impl->m_terminalDisplay->setVTFont(font);
}

QFont QTermWidget::getTerminalFont()
{
    if (!m_impl->m_terminalDisplay)
        return QFont();
    return m_impl->m_terminalDisplay->getVTFont();
}

void QTermWidget::setTerminalOpacity(qreal level)
{
    if (!m_impl->m_terminalDisplay)
        return;

    m_impl->m_terminalDisplay->setOpacity(level);
}

void QTermWidget::setShellProgram(const QString &progname)
{
    if (!m_impl->m_session)
        return;
    m_impl->m_session->setProgram(progname);
}

QString QTermWidget::getShellProgram()
{
    if (!m_impl->m_session)
        return QString();
    return m_impl->m_session->program();
}

void QTermWidget::setWorkingDirectory(const QString& dir)
{
    if (!m_impl->m_session)
        return;
    m_impl->m_session->setInitialWorkingDirectory(dir);
}

QString QTermWidget::getWorkingDirectory()
{
    if (!m_impl->m_session)
        return QString();
    return m_impl->m_session->initialWorkingDirectory();
}

void QTermWidget::setArgs(QStringList &args)
{
    if (!m_impl->m_session)
        return;
    m_impl->m_session->setArguments(args);
}

QStringList QTermWidget::getArgs()
{
    if (!m_impl->m_session)
        return QStringList();
    return m_impl->m_session->arguments();
}

void QTermWidget::setTextCodec(QTextCodec *codec)
{
    if (!m_impl->m_session)
        return;
    m_impl->m_session->setCodec(codec);
}

void QTermWidget::setTextCodec(QString codecName)
{
    if (!m_impl->m_session)
        return;
    QTextCodec *codec = QTextCodec::codecForName(codecName.toStdString().c_str());
    m_impl->m_session->setCodec(codec);
}

QString QTermWidget::getTextCodec()
{
    if (!m_impl->m_session)
        return QString();
    return m_impl->m_session->codec();
}

void QTermWidget::setColorScheme(const QString & name)
{
    const ColorScheme *cs;
    // avoid legacy (int) solution
    if (!availableColorSchemes().contains(name))
        cs = ColorSchemeManager::instance()->defaultColorScheme();
    else
        cs = ColorSchemeManager::instance()->findColorScheme(name);

    if (! cs)
    {
        QMessageBox::information(this,
                                 tr("Color Scheme Error"),
                                 tr("Cannot load color scheme: %1").arg(name));
        return;
    }
    ColorEntry table[TABLE_COLORS];
    cs->getColorTable(table);
    m_impl->m_terminalDisplay->setColorTable(table);
}

QStringList QTermWidget::availableColorSchemes()
{
    QStringList ret;
    foreach (const ColorScheme* cs, ColorSchemeManager::instance()->allColorSchemes())
        ret.append(cs->name());
    return ret;
}

void QTermWidget::setSize(int h, int v)
{
    if (!m_impl->m_terminalDisplay)
        return;
    m_impl->m_terminalDisplay->setSize(h, v);
}

int QTermWidget::columns()
{
    if (!m_impl->m_terminalDisplay)
        return 0;
    return m_impl->m_terminalDisplay->columns();
}

int QTermWidget::rows()
{
    if (!m_impl->m_terminalDisplay)
        return 0;
    return m_impl->m_terminalDisplay->lines();
}

void QTermWidget::setTerminalSizeHint(bool onoff)
{
    if (!m_impl->m_terminalDisplay)
        return;
    m_impl->m_terminalDisplay->setTerminalSizeHint(onoff);
}

bool QTermWidget::terminalSizeHint()
{
    if (!m_impl->m_terminalDisplay)
        return false;
    return m_impl->m_terminalDisplay->terminalSizeHint();
}

void QTermWidget::setHistorySize(int lines)
{
    if (lines < 0)
        m_impl->m_session->setHistoryType(HistoryTypeFile());
    else
        m_impl->m_session->setHistoryType(HistoryTypeBuffer(lines));
}

int QTermWidget::getHistorySize()
{
    if (!m_impl->m_session)
        return 0;

    if( m_impl->m_session->historyType().isUnlimited() )
        return -1;
    if( ! m_impl->m_session->historyType().isEnabled() )
        return 0;
    return m_impl->m_session->historyType().maximumLineCount();
}

void QTermWidget::setScrollBarPosition(ScrollBarPosition pos)
{
    if (!m_impl->m_terminalDisplay)
        return;
    m_impl->m_terminalDisplay->setScrollBarPosition((TerminalDisplay::ScrollBarPosition)pos);
}

QTermWidget::ScrollBarPosition QTermWidget::getScrollBarPosition()
{
    if (!m_impl->m_terminalDisplay)
        return QTermWidget::ScrollBarRight;
    return (QTermWidget::ScrollBarPosition)m_impl->m_terminalDisplay->scrollBarPosition();
}

void QTermWidget::scrollToEnd()
{
    if (!m_impl->m_terminalDisplay)
        return;
    m_impl->m_terminalDisplay->scrollToEnd();
}

void QTermWidget::sendText(QString &text)
{
    m_impl->m_session->sendText(text);
}

void QTermWidget::sendKey(QKeyEvent *event)
{
    m_impl->m_terminalDisplay->sendKeyEvent(event);
    //m_impl->m_session->sendKey(event);
}

void QTermWidget::focus(bool toggle)
{
    m_impl->m_terminalDisplay->focus(toggle);
}

void QTermWidget::resizeEvent(QResizeEvent*)
{
//qDebug("global window resizing...with %d %d", this->size().width(), this->size().height());
    m_impl->m_terminalDisplay->resize(this->size());
}


void QTermWidget::sessionFinished()
{
    emit finished();
}


void QTermWidget::copyClipboard()
{
    m_impl->m_terminalDisplay->copyClipboard();
}

void QTermWidget::pasteClipboard()
{
    m_impl->m_terminalDisplay->pasteClipboard();
}

void QTermWidget::pasteSelection()
{
    m_impl->m_terminalDisplay->pasteSelection();
}

void QTermWidget::setZoom(int step)
{
    if (!m_impl->m_terminalDisplay)
        return;
    
    QFont font = m_impl->m_terminalDisplay->getVTFont();
    
    font.setPointSize(font.pointSize() + step);
    setTerminalFont(font);
}

void QTermWidget::zoomIn()
{
    setZoom(STEP_ZOOM);
}

void QTermWidget::zoomOut()
{
    setZoom(-STEP_ZOOM);
}

void QTermWidget::setKeyBindings(const QString & kb)
{
    m_impl->m_session->setKeyBindings(kb);
}

void QTermWidget::clear()
{
    m_impl->m_session->emulation()->reset();
    m_impl->m_session->refresh();
    m_impl->m_session->clearHistory();
}

void QTermWidget::setFlowControlEnabled(bool enabled)
{
    m_impl->m_session->setFlowControlEnabled(enabled);
}

QStringList QTermWidget::availableKeyBindings()
{
    return KeyboardTranslatorManager::instance()->allTranslators();
}

QString QTermWidget::keyBindings()
{
    return m_impl->m_session->keyBindings();
}

void QTermWidget::toggleShowSearchBar()
{
    m_searchBar->isHidden() ? m_searchBar->show() : m_searchBar->hide();
}

bool QTermWidget::flowControlEnabled(void)
{
    return m_impl->m_session->flowControlEnabled();
}

void QTermWidget::setFlowControlWarningEnabled(bool enabled)
{
    if (flowControlEnabled()) {
        // Do not show warning label if flow control is disabled
        m_impl->m_terminalDisplay->setFlowControlWarningEnabled(enabled);
    }
}

void QTermWidget::setEnvironment(const QStringList& environment)
{
    m_impl->m_session->setEnvironment(environment);
}

QStringList QTermWidget::getEnvironment()
{
    return m_impl->m_session->environment();
}

void QTermWidget::setMotionAfterPasting(int action)
{
    m_impl->m_terminalDisplay->setMotionAfterPasting((Konsole::MotionAfterPasting) action);
}
