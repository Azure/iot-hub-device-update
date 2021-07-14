/**
 * @file handler_function_map.hpp
 * @brief Defines handler creation function map.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_HANDLER_FUNCTION_MAP_HPP
#define ADUC_HANDLER_FUNCTION_MAP_HPP

#include <functional>
#include <stdio.h>

#if ADUC_SWUPDATE_HANDLER

#    if !ADUC_SIMULATOR_MODE
#        include "aduc/swupdate_handler.hpp"
#    else
#        include "aduc/swupdate_simulator_handler.hpp"
#    endif

#endif

#if ADUC_APT_HANDLER

#    if !ADUC_SIMULATOR_MODE
#        include "aduc/apt_handler.hpp"
#    else
#        include "aduc/apt_simulator_handler.hpp"
#    endif

#endif

#if ADUC_PVCONTROL_HANDLER

#    if !ADUC_SIMULATOR_MODE
#        include "aduc/pvcontrol_handler.hpp"
#    else
#        include "aduc/pvcontrol_simulator_handler.hpp"
#    endif

#endif

typedef std::function<std::unique_ptr<ContentHandler>(const ContentHandlerCreateData&)> CreateFuncType;

typedef struct tagTypeFuncMap
{
    const char* UpdateType;
    CreateFuncType CreateFunc;
} TypeFuncMap;

#define CONCAT(U, V) #U "/" #V
#if ADUC_SIMULATOR_MODE
#    define FUNCMAPENTRY(VENDOR, TYPE)                                   \
        {                                                                \
            CONCAT(VENDOR, TYPE), VENDOR##_##TYPE##_Simulator_CreateFunc \
        }
#else
#    define FUNCMAPENTRY(VENDOR, TYPE)                         \
        {                                                      \
            CONCAT(VENDOR, TYPE), VENDOR##_##TYPE##_CreateFunc \
        }
#endif

const TypeFuncMap handlerCreateFuncs[] = {
// NOTE: When adding a new content handler, add a new FUNCMAPENTRY here.
#if ADUC_APT_HANDLER
    FUNCMAPENTRY(microsoft, apt),
#endif
#if ADUC_SWUPDATE_HANDLER
    FUNCMAPENTRY(microsoft, swupdate),
#endif
#if ADUC_PVCONTROL_HANDLER
    FUNCMAPENTRY(pantacor, pvcontrol),
#endif
};

#endif // ADUC_HANDLER_FUNCTION_MAP_HPP
