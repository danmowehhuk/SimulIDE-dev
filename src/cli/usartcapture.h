/***************************************************************************
 *   This file is part of SimulIDE.                                        *
 *   Released under GNU General Public License v3 or later.               *
 *   See copyright.txt at the project root for full license text.         *
 ***************************************************************************/

#pragma once

#include <QMutex>
#include <QString>
#include <QElapsedTimer>

class QFile;

// Captures bytes from the UART transmit path and monitors two exit conditions
// for headless (-nogui) runs:
//
//   1. Sentinel  — a configurable string appears in the output stream.
//                  Exits with code 0 (expected termination).
//
//   2. Idle timeout — no bytes received for idleTimeoutMs after monitoring
//                     started.  Exits with code 1 (hang / firmware failure).
//
// Usage:
//   1. Call setOutputFile / setSentinel / setIdleTimeoutMs before powerCircOn.
//   2. Call startMonitoring() right after powerCircOn().
//   3. Drive checkQuit() from a QTimer on the main thread (~100 ms interval).

class UsartCapture
{
    public:
        static void setOutputFile( const QString& path );
        static void setSentinel( const QString& sentinel );
        static void setIdleTimeoutMs( int ms );

        // Called from the simulator thread for every transmitted byte.
        static void captureByteReceived( uint8_t byte );

        // Reset the idle timer. Call right after powerCircOn().
        static void startMonitoring();

        // Called periodically from the main thread. Triggers exit when a
        // quit condition is met.
        static void checkQuit();

        static void closeFile();

    private:
        static QFile*        m_file;
        static QMutex        m_mutex;
        static QString       m_sentinel;
        static QString       m_sentinelBuf;
        static int           m_idleTimeoutMs;
        static QElapsedTimer m_idleTimer;
        static bool          m_monitoring;
        static bool          m_quitPending;
        static int           m_quitCode;
};
