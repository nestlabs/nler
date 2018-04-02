/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements NLER build platform-independent logging
 *      interfaces.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if NLER_FEATURE_LOG_TOKENIZATION
#include <nlutilities.h>
#endif

#include "nlerassert.h"
#include "nlerlog.h"
#include "nlerlogmanager.h"

extern nl_log_printer_t       gLogger;
extern void                   *gLoggerClosure;
extern nl_log_token_printer_t gTokenLogger;
extern void                   *gTokenLoggerClosure;
extern uint8_t                gAppLogLevels[];

static uint8_t loglevels[] =
{
    nlLPDEBG,   /* lrER */
    nlLPDEBG,   /* lrERTASK */
    nlLPDEBG,   /* lrEREVENT */
    nlLPDEBG,   /* lrERINIT */
    nlLPDEBG,   /* lrERQUEUE */
    nlLPDEBG,   /* lrERTIMER */
    nlLPDEBG,   /* lrERPOOLED */
    0           /* lrERLAST */
};

#if NLER_FEATURE_LOG_TOKENIZATION
/*
 * Ensure that NL_LOG_CREATE_ENTRY's creation of nl_log_token_region_entry_t
 * objects is up-to-date.
 */
NLER_STATIC_ASSERT(offsetof(nl_log_token_region_entry_t, mToken) == 0, "nl_log_token_region_entry_t has changed");
NLER_STATIC_ASSERT(offsetof(nl_log_token_region_entry_t, mRegionId) == 4, "nl_log_token_region_entry_t has changed");
#endif

void nl_log_va_list(nl_log_region_t aRegion, const char *aFormat, va_list aArgList)
{
    if (gLogger != NULL)
    {
        if (aRegion < (lrERLAST + 1))
        {
            if (loglevels[aRegion] > nlLPNONE)
            {
                (gLogger)(gLoggerClosure, aRegion, loglevels[aRegion], aFormat, aArgList);
            }
        }
        else if (gAppLogLevels[aRegion - (lrERLAST + 1)] > nlLPNONE)
        {
            (gLogger)(gLoggerClosure, aRegion, gAppLogLevels[aRegion - (lrERLAST + 1)], aFormat, aArgList);
        }
    }
}

void nl_log(nl_log_region_t aRegion, const char *aFormat, ...)
{
    /* this comparison needs to happen against something
     * better than this baked in constant. need to build the constant
     * in from the build system.
     */

    NLER_STATIC_ASSERT(sizeof(loglevels) == (lrERLAST + 1), __FILE__ ": loglevels arrary size does not match (lrERLAST + 1) defined in nllogregion.h");

    if (gLogger != NULL)
    {
        va_list ap;

        va_start(ap, aFormat);

        nl_log_va_list(aRegion, aFormat, ap);

        va_end(ap);
    }
}

void nl_log_token(nl_log_region_t aRegion, const nl_log_token_entry_t *aFormat, ...)
{
    /* this comparison needs to happen against something
     * better than this baked in constant. need to build the constant
     * in from the build system.
     */

    NLER_STATIC_ASSERT(sizeof(loglevels) == (lrERLAST + 1), __FILE__ ": loglevels arrary size does not match (lrERLAST + 1) defined in nllogregion.h");

    if (gTokenLogger != NULL)
    {
        va_list ap;

        va_start(ap, aFormat);

        if (aRegion < (lrERLAST + 1))
        {
            if (loglevels[aRegion] > nlLPNONE)
            {
                (gTokenLogger)(gTokenLoggerClosure, aRegion, loglevels[aRegion], aFormat, ap);
            }
        }
        else if (gAppLogLevels[aRegion - (lrERLAST + 1)] > nlLPNONE)
        {
            (gTokenLogger)(gTokenLoggerClosure, aRegion, gAppLogLevels[aRegion - (lrERLAST + 1)], aFormat, ap);
        }

        va_end(ap);
    }
}

void nl_set_log_priority(nl_log_region_t aRegion, int aPri)
{
    if (aRegion < (lrERLAST + 1))
    {
        loglevels[aRegion] = (uint8_t)aPri;
    }
    else if (gAppLogLevels != NULL)
    {
        gAppLogLevels[aRegion - (lrERLAST + 1)] = (uint8_t)aPri;
    }
}

int nl_get_log_priority(nl_log_region_t aRegion)
{
    int retval = nlLPNONE;

    if (aRegion < (lrERLAST + 1))
    {
        retval = loglevels[aRegion];
    }
    else if (gAppLogLevels != NULL)
    {
        retval = gAppLogLevels[aRegion - (lrERLAST + 1)];
    }

    return retval;
}

#if NLER_FEATURE_LOG_TOKENIZATION
void nl_log_send_tokenized(void (*aOutputCharFunc)(uint8_t c, void *context),
                           nl_time_ms_t aTimeMs,
                           nl_log_utc_ms_t aUtcTimeMs,
                           const nl_log_token_entry_t *aFormat,
                           va_list ap)
{
    bool done = false;
    nl_log_header_t header;
    uint32_t compressedFormat = aFormat->mFormat;

    nl_base64_stream_enc_state_t enc_state;
    nl_base64_stream_enc_start(&enc_state, aOutputCharFunc, NULL);

    // Output sentinel bytes to identify tokenized log lines
    aOutputCharFunc(NL_LOG_TOKEN_SENTINEL1, NULL);
    aOutputCharFunc(NL_LOG_TOKEN_SENTINEL2, NULL);
    aOutputCharFunc('\t', NULL);

    // Output log header, which includes the token.
    header.mHeaderVersion = NL_LOG_HEADER_VERSION_1;
    memcpy(&header.mBuildId, &gBuildId, sizeof(nl_build_id_t));
    header.mToken = aFormat->mToken;
    header.mTimeMs = (uint32_t) aTimeMs;
    header.mUtcTimeMs = aUtcTimeMs;
    nl_base64_stream_enc_more((const uint8_t *)&header,
                              sizeof(nl_log_header_t),
                              &enc_state);

    // Output format arguments
    while (!done)
    {
        unsigned int arg_type = compressedFormat & NL_ER_LOG_ARG_FIELD_MASK;
        NLER_STATIC_ASSERT(NL_ER_LOG_ARG_FIELD_MASK == 3, __FILE__ ": NL_ER_LOG_ARG_FIELD_MASK has changed.  Verify switch covers all cases.");

        // The following loop's exit condition depends on kArgType_Invalid
        // having the value of zero.  Because compressedFormat is unsigned,
        // the arg type will eventually have to be 0.
        NLER_STATIC_ASSERT(kArgType_Invalid == 0, __FILE__ ": Log tokenization invalid argument type enumerator must be zero.");

        switch (arg_type)
        {
        case kArgType_Invalid:
            done = true;
            break;
        case kArgType_Numeric32:
            {
                uint32_t arg = va_arg(ap, uint32_t);
                nl_base64_stream_enc_more((const uint8_t *)&arg, sizeof(arg),
                                          &enc_state);
            }
            break;
        case kArgType_Numeric64:
            {
                uint64_t arg = va_arg(ap, uint64_t);
                nl_base64_stream_enc_more((const uint8_t *)&arg, sizeof(arg),
                                          &enc_state);
            }
            break;
        case kArgType_String:
            {
                char * arg = va_arg(ap, char *);
                size_t len = strlen(arg) + 1;
                nl_base64_stream_enc_more((const uint8_t *)arg, len,
                                          &enc_state);
            }
            break;
        }

        compressedFormat >>= NL_ER_LOG_ARG_FIELD_WIDTH;
    }
    nl_base64_stream_enc_finish(/*pad*/true, &enc_state);
    aOutputCharFunc('\n', NULL);
}
#endif
