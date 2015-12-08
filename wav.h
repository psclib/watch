#ifndef WAV_H
#define WAV_H

#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    char     chunk_id[4];
    uint32_t chunk_size;
    char     format[4];

    char     fmtchunk_id[4];
    uint32_t fmtchunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bps;

    char     datachunk_id[4];
    uint32_t datachunk_size;
}WavHeader;

float audio_int16_to_float(int16_t i){
  float f = (float)i / 32768;
  if(f > 1){
    f = 1;
  }else if(f < -1){
    f = -1;
  }
  return f;
}

double audio_int16_to_double(int16_t i){
  double f = (double)i / 32768;
  if(f > 1){
    f = 1;
  }else if(f < -1){
    f = -1;
  }
  return f;
}

#endif
