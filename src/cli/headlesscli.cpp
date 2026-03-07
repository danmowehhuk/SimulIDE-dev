/***************************************************************************
 *   This file is part of SimulIDE.                                        *
 *   Released under GNU General Public License v3 or later.               *
 *   See copyright.txt at the project root for full license text.         *
 ***************************************************************************/

#include <QFileInfo>
#include <QTimer>
#include <cstdio>
#include <cstdlib>

#include "headlesscli.h"
#include "usartcapture.h"
#include "circuitwidget.h"
#include "mcu.h"

QString HeadlessCli::m_circuitPath;
QString HeadlessCli::m_firmwarePath;

bool HeadlessCli::isHeadless( int argc, char* argv[] )
{
    for( int i=1; i<argc; ++i )
        if( QString::fromLocal8Bit( argv[i] ) == "-nogui" ) return true;
    return false;
}

bool HeadlessCli::parseArgs( int argc, char* argv[] )
{
    QString circuitPath, firmwarePath, outPath, stopSignal;
    int idleTimeoutMs = 5000;

    for( int i=1; i<argc; ++i )
    {
        QString arg = QString::fromLocal8Bit( argv[i] );

        if(      arg == "-circuit"      && i+1 < argc ) circuitPath   = QString::fromLocal8Bit( argv[++i] );
        else if( arg == "-firmware"     && i+1 < argc ) firmwarePath  = QString::fromLocal8Bit( argv[++i] );
        else if( arg == "-out"          && i+1 < argc ) outPath       = QString::fromLocal8Bit( argv[++i] );
        else if( arg == "-stop-signal"  && i+1 < argc ) stopSignal    = QString::fromLocal8Bit( argv[++i] );
        else if( arg == "-idle-timeout" && i+1 < argc ) idleTimeoutMs = QString::fromLocal8Bit( argv[++i] ).toInt();
        else if( !arg.startsWith("-") )
        {
            // Positional .sim2 path (e.g. simulide -nogui circuit.sim2)
            QString path = arg;
            if( path.startsWith("file://") ) path = path.mid(7).replace("\r\n","").replace("%20"," ");
#ifdef _WIN32
            if( path.startsWith("/") ) path.remove(0,1);
#endif
            if( path.endsWith(".sim2") || path.endsWith(".sim1") ) circuitPath = path;
        }
    }

    return configure( circuitPath, firmwarePath, outPath, stopSignal, idleTimeoutMs );
}

bool HeadlessCli::configure( const QString& circuitPath,
                              const QString& firmwarePath,
                              const QString& outPath,
                              const QString& stopSignal,
                              int            idleTimeoutMs )
{
    if( circuitPath.isEmpty() )
    {
        fprintf( stderr,
            "Usage: simulide -nogui -circuit <file.sim2> [options]\n\n"
            "Options:\n"
            "  -firmware <file>      Load firmware into the first MCU\n"
            "  -out <file>           Write UART output to file (default: stdout)\n"
            "  -stop-signal <str>    Exit 0 when string appears in UART output\n"
            "  -idle-timeout <ms>    Exit 1 after N ms of silence (default: 5000)\n" );
        return false;
    }
    if( !QFileInfo::exists( circuitPath ) )
    {
        fprintf( stderr, "ERROR: circuit file not found: %s\n",
                 circuitPath.toLocal8Bit().constData() );
        return false;
    }

    if( !outPath.isEmpty() )    UsartCapture::setOutputFile( outPath );
    if( !stopSignal.isEmpty() ) UsartCapture::setSentinel( stopSignal );
    UsartCapture::setIdleTimeoutMs( idleTimeoutMs );

    m_circuitPath  = circuitPath;
    m_firmwarePath = firmwarePath;
    return true;
}

void HeadlessCli::run()
{
    CircuitWidget::self()->loadCirc( m_circuitPath );

    if( !m_firmwarePath.isEmpty() )
    {
        Mcu* mcu = Mcu::self();
        if( !mcu )
        {
            fprintf( stderr, "ERROR: no MCU found in circuit\n" );
            std::_Exit( 1 );
        }
        if( !mcu->load( m_firmwarePath ) )
        {
            fprintf( stderr, "ERROR: failed to load firmware: %s\n",
                     m_firmwarePath.toLocal8Bit().constData() );
            std::_Exit( 1 );
        }
    }

    UsartCapture::startMonitoring();
    CircuitWidget::self()->powerCircOn();

    QTimer* quitPoll = new QTimer();
    quitPoll->setInterval( 100 );
    QObject::connect( quitPoll, &QTimer::timeout,
                      [](){ UsartCapture::checkQuit(); } );
    quitPoll->start();
}
