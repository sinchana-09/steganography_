#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

typedef struct _DecodeInfo
{
    /* Stego Image Info */
    char *stego_image_fname;
    FILE *fptr_stego_image;

    /* Output Secret File Info */
    char output_fname[100];
    FILE *fptr_output_file;
    char extn_output_file[10];
    int extn_size;
    int size_secret_file;

} DecodeInfo;

/* Function Prototypes */

/* 1. Read & validate command line arguments */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* 2. Open input stego image file */
Status open_decode_files(DecodeInfo *decInfo);

/* 3. Helper: Decode 1 char/byte from 8 image bytes */
char decode_char_from_lsb(const char *image_buffer);

/* 4. Helper: Decode 1 integer (32-bit) from 32 image bytes */
int decode_size_from_lsb(const char *image_buffer);

/* 5. Decode Magic String and compare with "#*" */
Status decode_magic_string(DecodeInfo *decInfo);

/* 6. Decode secret file extension size & extension string */
Status decode_secret_file_extn(DecodeInfo *decInfo);

/* 7. Decode secret file size */
Status decode_secret_file_size(DecodeInfo *decInfo);

/* 8. Decode secret file data and write to output file */
Status decode_secret_file_data(DecodeInfo *decInfo);

/* Complete Decoding Pipeline */
Status do_decoding(DecodeInfo *decInfo);

#endif