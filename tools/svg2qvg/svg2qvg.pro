QSK_ROOT = $${PWD}/../..
QSK_OUT_ROOT = $${OUT_PWD}/../..

TEMPLATE     = app
TARGET = svg2qvg

QT += svg

CONFIG += standalone
CONFIG -= app_bundle

QSK_DIRS = \
    $${QSK_ROOT}/src/common \
    $${QSK_ROOT}/src/graphic

INCLUDEPATH *= $${QSK_DIRS}
DEPENDPATH  += $${QSK_DIRS}

DESTDIR      = $${QSK_OUT_ROOT}/tools/bin

standalone {

    # We only need a very small subset of QSkinny and by including the 
    # necessary cpp files svg2qvg becomes independent from the library

    DEFINES += QSK_STANDALONE

}
else {

    qskAddLibrary( $${QSK_OUT_ROOT}/lib, qskinny )

    contains(QSK_CONFIG, QskDll) {
        DEFINES    += QSK_DLL
    }
}

SOURCES += \
    main.cpp

target.path    = $${QSK_INSTALL_BINS}
INSTALLS       = target
