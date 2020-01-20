#pragma once

#ifdef XENGINE_EXPORTS
#define XENGINEAPI _declspec(dllexport)
#else
#define XENGINEAPI _declspec(dllimport)
#endif