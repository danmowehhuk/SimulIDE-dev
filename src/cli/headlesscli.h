/***************************************************************************
 *   This file is part of SimulIDE.                                        *
 *   Released under GNU General Public License v3 or later.               *
 *   See copyright.txt at the project root for full license text.         *
 ***************************************************************************/

#pragma once

#include <QString>

// Encapsulates headless (-nogui) CLI mode.
//
// Typical call sequence from main():
//
//   g_headless = HeadlessCli::isHeadless( argc, argv );
//   ...
//   if( arg == "-nogui" ) {
//       window.hideGui(); window.hide();
//       if( !HeadlessCli::parseArgs( argc, argv ) ) return 1;
//       QTimer::singleShot( 500, [](){ HeadlessCli::run(); } );
//       break;
//   }

class HeadlessCli
{
    public:
        // Returns true if -nogui is present in argv.  Used to suppress the
        // Qt window system before QApplication is constructed.
        static bool isHeadless( int argc, char* argv[] );

        // Parse all CLI flags from argv, validate, and configure UsartCapture.
        // Prints usage / error to stderr and returns false on failure.
        static bool parseArgs( int argc, char* argv[] );

        // Load circuit, flash firmware, start monitoring, and power on.
        // Must be called from the main thread after the event loop starts.
        static void run();

    private:
        static bool configure( const QString& circuitPath,
                               const QString& firmwarePath,
                               const QString& outPath,
                               const QString& stopSignal,
                               int            idleTimeoutMs );

        static QString m_circuitPath;
        static QString m_firmwarePath;
};
