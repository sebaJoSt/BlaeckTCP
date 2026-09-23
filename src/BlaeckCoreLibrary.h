/*
        File: BlaeckCoreLibrary.h

    The one core file that differs between Blaeck libraries: it names this library's
    namespace and settings. BlaeckCore.h and BlaeckCore.cpp are identical copies.
*/

#ifndef BLAECK_CORE_LIBRARY_H
#define BLAECK_CORE_LIBRARY_H

#define BLAECK_CORE_NAMESPACE blaeck_tcp

// A sketch's own settings. See docs/configuration.md.
#if defined __has_include
  #if __has_include(<BlaeckTCPConfig.h>)
    #include <BlaeckTCPConfig.h>
  #endif
#endif

// Buffered writes
// ---------------
// On: each frame is built in RAM and sent with one write per host. The buffer is sized from
// the signals added and grows if a frame needs more.
// Off: bytes go to the network as the frame is built, and no buffer is allocated.
//
// On for every board, AVR included: with an Ethernet shield each write is a TCP send of its
// own, so an unbuffered frame leaves in many small packets. setBufferedWrites() changes it
// at runtime.
#ifndef BLAECK_BUFFERED_WRITES_DEFAULT
  #define BLAECK_BUFFERED_WRITES_DEFAULT true
#endif

#endif // BLAECK_CORE_LIBRARY_H
