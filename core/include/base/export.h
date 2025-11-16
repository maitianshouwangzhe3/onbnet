#pragma once

// 定义导出宏
#ifdef _WIN32
    #ifdef MYCLASS_EXPORTS
        #define ONBNET_API __declspec(dllexport)
    #else
        #define ONBNET_API __declspec(dllimport)
    #endif
#else
    #define ONBNET_API __attribute__((visibility("default")))
#endif