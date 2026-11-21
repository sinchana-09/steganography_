#ifndef ENCODE_H
#define ENCODE_H

#include <stdio.h>
#include "types.h"

/* Macro: Set to 10 so full extension (.txt + '\0') fits cleanly */
#define MAX_FILE_SUFFIX 10

typedef struct _EncodeInfo
{
    /* Source Image Info */
    char *src_image_fname;
    FILE *fptr_src_image;
    uint image_capacity;

    /* Secret File Info */
    char *secret_fname;
    FILE *fptr_secret;
    char extn_secret_file[MAX_FILE_SUFFIX];
    long size_secret_file;

    /* Stego Image Info */
    char *stego_image_fname;
    FILE *fptr_stego_image;

} EncodeInfo;

/* Function Prototypes */

/* Check operation type based on CLI flag */
OperationType check_operation_type(char *argv[]);

/* Read and validate CLI arguments for encoding */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo);

/* Perform complete encoding process */
Status do_encoding(EncodeInfo *encInfo);

/* Open files for encoding */
Status open_files(EncodeInfo *encInfo);

/* Check if source image capacity is sufficient */
Status check_capacity(EncodeInfo *encInfo);

/* Get image capacity */
uint get_image_size_for_bmp(FILE *fptr_image);

/* Get secret file size */
uint get_file_size(FILE *fptr);

/* Copy 54 bytes BMP header */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image);

/* Encode Magic String */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo);

/* Encode secret file extension size and extension */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo);

/* Encode secret file size */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo);

/* Encode secret file data */
Status encode_secret_file_data(EncodeInfo *encInfo);

/* Encode byte into 8 LSBs */
Status encode_byte_to_lsb(char data, char *image_buffer);

/* Encode integer into 32 LSBs */
Status encode_size_to_lsb(int size, char *image_buffer);

/* Copy remaining bytes */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest);

#endif