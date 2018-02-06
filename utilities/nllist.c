/*
 *
 *    Copyright (c) 2015-2018 Nest Labs, Inc.
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
 *
 *    @file
 *      List. Implemented as an array-based double-ended queue
 *      (add/remove at either the head or the tail of the list).
 *
 */

#include <nllist.h>

#include <stdlib.h>
#include <string.h>

#include <nlererror.h>

int nl_list_init(nl_list_t *aList, void **aStorage, int aNumElements)
{
    int retval = NLER_SUCCESS;

    if ((aStorage != NULL) && (aNumElements > 0))
    {
        aList->mList = aStorage;
        aList->mListSize = aNumElements;
        aList->mListEnd = 0;
    }
    else
    {
        retval = NLER_ERROR_BAD_INPUT;
    }

    return retval;
}

int nl_list_is_empty(nl_list_t *aList)
{
    return (aList->mListEnd == 0);
}

int nl_list_is_full(nl_list_t *aList)
{
    return (aList->mListEnd == aList->mListSize);
}

void *nl_list_remove_head(nl_list_t *aList)
{
    void *retval = NULL;

    if (aList->mListEnd > 0)
    {
        retval = aList->mList[0];

        aList->mListEnd--;

        if (aList->mListEnd > 0)
        {
            memmove(&aList->mList[0], &aList->mList[1], aList->mListEnd * sizeof(void *));
        }
    }

    return retval;
}

void *nl_list_remove_tail(nl_list_t *aList)
{
    void *retval = NULL;

    if (aList->mListEnd > 0)
    {
        retval = aList->mList[--aList->mListEnd];
    }

    return retval;
}

void *nl_list_remove_element(nl_list_t *aList, void *aElement)
{
    void    *retval = NULL;
    int     idx = 0;

    while (idx < aList->mListEnd)
    {
        if (aList->mList[idx] == aElement)
        {
            aList->mListEnd--;

            if (idx < aList->mListEnd)
            {
                memmove(&aList->mList[idx], &aList->mList[idx + 1], (aList->mListEnd - idx) * sizeof(void *));
            }

            retval = aElement;

            break;
        }

        idx++;
    }

    return retval;
}

void *nl_list_peek_head(nl_list_t *aList)
{
    void *retval = NULL;

    if (aList->mListEnd > 0)
    {
        retval = aList->mList[0];
    }

    return retval;
}

void *nl_list_peek_tail(nl_list_t *aList)
{
    void *retval = NULL;

    if (aList->mListEnd > 0)
    {
        retval = aList->mList[aList->mListEnd - 1];
    }

    return retval;
}

int nl_list_add_head(nl_list_t *aList, void *aElement)
{
    int retval = NLER_SUCCESS;

    if (aList->mListEnd < aList->mListSize)
    {
        if (aList->mListEnd > 0)
        {
            memmove(&aList->mList[1], &aList->mList[0], aList->mListEnd * sizeof(void *));
        }

        aList->mList[0] = aElement;
        aList->mListEnd++;
    }
    else
    {
        retval = NLER_ERROR_NO_MEMORY;
    }

    return retval;
}

int nl_list_add_tail(nl_list_t *aList, void *aElement)
{
    int retval = NLER_SUCCESS;

    if (aList->mListEnd < aList->mListSize)
    {
        aList->mList[aList->mListEnd++] = aElement;
    }
    else
    {
        retval = NLER_ERROR_NO_MEMORY;
    }

    return retval;
}

void nl_list_enumerate(nl_list_t *aList, nl_list_enumerator_t aEnumerator, void *aClosure)
{
    int idx = 0;

    while (idx < aList->mListEnd)
    {
        (aEnumerator)(idx, aList->mList[idx], aClosure);
        idx++;
    }

    (aEnumerator)(-1, NULL, aClosure);
}

int nl_list_has_element(nl_list_t *aList, void *aElement)
{
    int retval = NLER_ERROR_FAILURE;
    int idx = 0;

    while (idx < aList->mListEnd)
    {
        if (aList->mList[idx] == aElement)
        {
            retval = NLER_SUCCESS;
            break;
        }
        idx++;
    }

    return retval;
}

