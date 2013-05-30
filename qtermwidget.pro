TEMPLATE = lib
DEFINES += HAVE_POSIX_OPENPT=1

DEFINES += KB_LAYOUT_DIR=\\\"/usr/local/share/qtermwidget/kb-layouts/\\\"
kblayout.path = /usr/local/share/qtermwidget/kb-layouts
kblayout.files = lib/kb-layouts/default.keytab  lib/kb-layouts/linux.keytab  lib/kb-layouts/macbook.keytab  lib/kb-layouts/solaris.keytab  lib/kb-layouts/vt420pc.keytab

DEFINES += COLORSCHEMES_DIR=\\\"/usr/local/share/qtermwidget/color-schemes/\\\"
colorschemes.path = /usr/local/share/qtermwidget/color-schemes
colorschemes.files = lib/color-schemes/BlackOnLightYellow.schema lib/color-schemes/BlackOnWhite.schema lib/color-schemes/GreenOnBlack.colorscheme lib/color-schemes/WhiteOnBlack.schema lib/color-schemes/BlackOnRandomLight.colorscheme lib/color-schemes/DarkPastels.colorscheme lib/color-schemes/Linux.colorscheme

QT += widgets
TARGET = qtermwidget

HEADERS = lib/BlockArray.h \
lib/CharacterColor.h \
lib/Character.h \
lib/ColorScheme.h \
lib/ColorTables.h \
lib/DefaultTranslatorText.h \
lib/Emulation.h \
lib/ExtendedDefaultTranslator.h \
lib/Filter.h \
lib/History.h \
lib/HistorySearch.h \
lib/KeyboardTranslator.h \
lib/konsole_wcwidth.h \
lib/kprocess.h \
lib/kptydevice.h \
lib/kpty.h \
lib/kpty_p.h \
lib/kptyprocess.h \
lib/LineFont.h \
lib/Pty.h \
lib/Screen.h \
lib/ScreenWindow.h \
lib/SearchBar.h \
lib/Session.h \
lib/ShellCommand.h \
lib/TerminalCharacterDecoder.h \
lib/TerminalDisplay.h \
lib/tools.h \
lib/Vt102Emulation.h \
lib/qtermwidget.h


SOURCES = lib/BlockArray.cpp \
lib/ColorScheme.cpp \
lib/Emulation.cpp \
lib/Filter.cpp \
lib/History.cpp \
lib/HistorySearch.cpp \
lib/KeyboardTranslator.cpp \
lib/konsole_wcwidth.cpp \
lib/kprocess.cpp \
lib/kpty.cpp \
lib/kptydevice.cpp \
lib/kptyprocess.cpp \
lib/Pty.cpp \
lib/qtermwidget.cpp \
lib/Screen.cpp \
lib/ScreenWindow.cpp \
lib/SearchBar.cpp \
lib/Session.cpp \
lib/ShellCommand.cpp \
lib/TerminalCharacterDecoder.cpp \
lib/TerminalDisplay.cpp \
lib/tools.cpp \
lib/Vt102Emulation.cpp

FORMS = lib/SearchBar.ui

headers.files = lib/qtermwidget.h
headers.path = /usr/local/include

target.path = /usr/local/lib

INSTALLS += target headers kblayout colorschemes

