#pragma once

// Writes a minidump (Windows) when the process crashes, so field crashes can
// be analyzed after the fact. No-op on other platforms.
namespace CrashHandler
{
// Call once, before QApplication is created.
void install();
}
