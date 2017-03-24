QT += widgets gui core
# CONFIG -= app_bundle

HEADERS       = mainwindow.h \
                qwidgetstyleselector.h
SOURCES       = mainwindow.cpp \
                qwidgetstyleselector.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/menus
INSTALLS += target
