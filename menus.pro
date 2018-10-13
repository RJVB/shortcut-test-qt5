QT += widgets gui core concurrent
# CONFIG -= app_bundle

QMAKE_CXXFLAGS += $$QMAKE_CXXFLAGS_CXX11

HEADERS       = qqmenu.h \
                main.h \
                mainwindow.h \
                qwidgetstyleselector.h
SOURCES       = mainwindow.cpp \
                qwidgetstyleselector.cpp \
                qqmenu.cpp \
                main.cpp

mac {
    LIBS += -framework Carbon
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/menus
INSTALLS += target
