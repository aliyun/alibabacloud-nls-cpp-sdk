/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2007             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

function: LPC low level routines
last mod: $Id: lpc.h 16037 2009-05-26 21:10:58Z xiphmont $

 ********************************************************************/

#ifndef ALIBABA_NLS_LPC_H_
#define ALIBABA_NLS_LPC_H_

#include <stdint.h>

float vorbis_lpc_from_data(float *data, float *lpci, int n, int m, int stride);

void vorbis_lpc_predict(float *coeff, float *prime, int m,
                        float *data, int64_t n, int stride);

#endif  // ALIBABA_NLS_LPC_H_

