QT += widgets gui core
# CONFIG -= app_bundle

QMAKE_CXXFLAGS += $$QMAKE_CXXFLAGS_CXX11

HEADERS       = qqmenu.h \
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
