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
 *      List. Defined as an array-based double-ended queue (add/remove
 *      at either the head or the tail of the list).
 *
 */

#ifndef NL_ER_UTILITIES_LIST_H
#define NL_ER_UTILITIES_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#define NLER_LIST_AS_ARRAY 1

#ifdef NLER_LIST_AS_ARRAY

typedef struct
{
    void    **mList;
    int     mListSize;
    int     mListEnd;
} nl_list_t;

#else /* NLER_LIST_AS_ARRAY */

typedef struct
{
    void **mHead;
    void **mTail;
} nl_list_t;

#endif /* NLER_LIST_AS_ARRAY */

/* supply this callback to the list enumeration function.
 * aPosition is a counter that will increase by one for each
 * element in the list. the callback will be called one more
 * time when enumeration is complete with aPosition equal to
 * -1.
 * aElement an element in the list
 * aClosure is a pointer to an object that you supplied when
 * beginning the enumeration.
 */

typedef void (*nl_list_enumerator_t)(int aPosition, void *aElement, void *aClosure);

int nl_list_init(nl_list_t *aList, void **aStorage, int aNumElements);
int nl_list_is_empty(nl_list_t *aList);
int nl_list_is_full(nl_list_t *aList);
void *nl_list_remove_head(nl_list_t *aList);
void *nl_list_remove_tail(nl_list_t *aList);
void *nl_list_remove_element(nl_list_t *aList, void *aElement);
void *nl_list_peek_head(nl_list_t *aList);
void *nl_list_peek_tail(nl_list_t *aList);
int nl_list_add_head(nl_list_t *aList, void *aElement);
int nl_list_add_tail(nl_list_t *aList, void *aElement);
int nl_list_has_element(nl_list_t *aList, void *aElement);
void nl_list_enumerate(nl_list_t *aList, nl_list_enumerator_t aEnumerator, void *aClosure);

#ifdef __cplusplus
}
#endif

#endif /* NL_ER_UTILITIES_LIST_H */
