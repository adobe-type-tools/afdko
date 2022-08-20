/* Copyright 2022 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0. 
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#ifndef SHARED_INCLUDE_SMEM_H_
#define SHARED_INCLUDE_SMEM_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void *sMemNew(size_t size);
extern void *sMemResize(void *old, size_t size);
extern void sMemFree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif  // SHARED_INCLUDE_SMEM_H_
