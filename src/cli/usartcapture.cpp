/***************************************************************************
 *   This file is part of SimulIDE.                                        *
 *   Released under GNU General Public License v3 or later.               *
 *   See copyright.txt at the project root for full license text.         *
 ***************************************************************************/

#include <QFile>
#include <QTimer>
#include <cstdio>
#include <cstdlib>

#include "usartcapture.h"
#include "circuitwidget.h"

QFile*        UsartCapture::m_file          = nullptr;
QMutex        UsartCapture::m_mutex;
QString       UsartCapture::m_sentinel;
QString       UsartCapture::m_sentinelBuf;
int           UsartCapture::m_idleTimeoutMs = 5000;
QElapsedTimer UsartCapture::m_idleTimer;
bool          UsartCapture::m_monitoring    = false;
bool          UsartCapture::m_quitPending   = false;
int           UsartCapture::m_quitCode      = 0;

void UsartCapture::setOutputFile( const QString& path )
{
    QMutexLocker lock( &m_mutex );
    if( m_file ) { m_file->close(); delete m_file; }
    m_file = new QFile( path );
    if( !m_file->open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        fprintf( stderr, "ERROR: cannot open output file: %s\n",
                 path.toLocal8Bit().constData() );
        delete m_file;
        m_file = nullptr;
    }
}

void UsartCapture::setSentinel( const QString& sentinel )
{
    QMutexLocker lock( &m_mutex );
    m_sentinel = sentinel;
    m_sentinelBuf.clear();
}

void UsartCapture::setIdleTimeoutMs( int ms )
{
    QMutexLocker lock( &m_mutex );
    m_idleTimeoutMs = ms;
}

void UsartCapture::startMonitoring()
{
    QMutexLocker lock( &m_mutex );
    m_monitoring  = true;
    m_quitPending = false;
    m_idleTimer.restart();
}

void UsartCapture::captureByteReceived( uint8_t byte )
{
    QMutexLocker lock( &m_mutex );

    if( m_file && m_file->isOpen() )
    {
        m_file->write( reinterpret_cast<const char*>( &byte ), 1 );
        m_file->flush();
    }
    else
    {
        fwrite( &byte, 1, 1, stdout );
        fflush( stdout );
    }

    if( !m_monitoring || m_quitPending ) return;

    m_idleTimer.restart();

    if( !m_sentinel.isEmpty() )
    {
        m_sentinelBuf.append( QChar( byte ) );

        // Retain only as much tail as needed to detect a match.
        int keep = m_sentinel.length() + 4;
        if( m_sentinelBuf.length() > keep )
            m_sentinelBuf.remove( 0, m_sentinelBuf.length() - keep );

        if( m_sentinelBuf.contains( m_sentinel ) )
        {
            m_quitPending = true;
            m_quitCode    = 0;
        }
    }
}

void UsartCapture::checkQuit()
{
    QMutexLocker lock( &m_mutex );

    if( !m_monitoring ) return;

    if( !m_quitPending
        && m_idleTimer.isValid()
        && m_idleTimer.elapsed() >= m_idleTimeoutMs )
    {
        m_quitPending = true;
        m_quitCode    = 1;
    }

    if( m_quitPending )
    {
        m_monitoring = false;
        closeFile();
        int code = m_quitCode;
        lock.unlock();
        QTimer::singleShot( 0, [code](){
            CircuitWidget::self()->powerCircOff();
            QTimer::singleShot( 300, [code](){ std::_Exit( code ); });
        });
    }
}

void UsartCapture::closeFile()
{
    if( m_file ) { m_file->close(); delete m_file; m_file = nullptr; }
}
