#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decode.h"
#include "common.h"
#include "types.h"

/* Step 1: Read & validate command line arguments according to notes */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    // Check if stego file is provided and has .bmp extension
    if (argv[2] != NULL && strstr(argv[2], ".bmp") != NULL)
    {
        decInfo->stego_image_fname = argv[2];
    }
    else
    {
        fprintf(stderr, "ERROR: Stego image must be a .bmp file\n");
        return e_failure;
    }

    // Check if output filename is passed in argv[3]
    if (argv[3] != NULL)
    {
        // Check if user specified filename contains extension '.'
        char *ext = strrchr(argv[3], '.');
        if (ext != NULL)
        {
            strcpy(decInfo->output_fname, argv[3]);
        }
        else
        {
            // Store base name (extension will be appended after decoding extension)
            strcpy(decInfo->output_fname, argv[3]);
        }
    }
    else
    {
        // Default filename as mentioned in notes
        strcpy(decInfo->output_fname, "default");
    }

    return e_success;
}

/* Step 2: Open stego image file and seek to 54th byte */
Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "r");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open stego image %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    // Move file pointer past original 54-byte BMP header
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);

    return e_success;
}

/* Helper: Decode 1 char from 8 LSBs */
char decode_char_from_lsb(const char *image_buffer)
{
    char byte = 0;
    for (int i = 0; i < 8; i++)
    {
        byte = (byte << 1) | (image_buffer[i] & 1);
    }
    return byte;
}

/* Helper: Decode integer (32 bits) from 32 LSBs */
int decode_size_from_lsb(const char *image_buffer)
{
    int size = 0;
    for (int i = 0; i < 32; i++)
    {
        size = (size << 1) | (image_buffer[i] & 1);
    }
    return size;
}

/* Step 3 & 4: Decode Magic String and compare with "#*" */
Status decode_magic_string(DecodeInfo *decInfo)
{
    int magic_len = strlen(MAGIC_STRING); // 2 bytes for "#*"
    char magic_str[magic_len + 1];
    char buffer[8];

    for (int i = 0; i < magic_len; i++)
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)
            return e_failure;

        magic_str[i] = decode_char_from_lsb(buffer);
    }
    magic_str[magic_len] = '\0';

    // Compare decoded string with magic string "#*"
    if (strcmp(magic_str, MAGIC_STRING) == 0)
    {
        return e_success;
    }

    return e_failure;
}

/* Step 5 & 6: Decode extension size and extension string */
Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    char buffer[32];

    // Decode extension size (Take 32 bytes from BMP file)
    if (fread(buffer, 1, 32, decInfo->fptr_stego_image) != 32)
        return e_failure;

    decInfo->extn_size = decode_size_from_lsb(buffer);

    // Call decode_char_from_lsb extn_size times
    char image_buf[8];
    for (int i = 0; i < decInfo->extn_size; i++)
    {
        if (fread(image_buf, 1, 8, decInfo->fptr_stego_image) != 8)
            return e_failure;

        decInfo->extn_output_file[i] = decode_char_from_lsb(image_buf);
    }
    decInfo->extn_output_file[decInfo->extn_size] = '\0';

    return e_success;
}

/* Step 7: Decode size of secret file */
Status decode_secret_file_size(DecodeInfo *decInfo)
{
    char buffer[32];

    // Take 32 bytes from BMP file
    if (fread(buffer, 1, 32, decInfo->fptr_stego_image) != 32)
        return e_failure;

    decInfo->size_secret_file = decode_size_from_lsb(buffer);

    return e_success;
}

/* Step 8: Decode secret data & write to file */
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    // Append extension if output filename doesn't already have one
    if (strrchr(decInfo->output_fname, '.') == NULL)
    {
        strcat(decInfo->output_fname, decInfo->extn_output_file);
    }

    decInfo->fptr_output_file = fopen(decInfo->output_fname, "w");
    if (decInfo->fptr_output_file == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open output file %s\n", decInfo->output_fname);
        return e_failure;
    }

    char image_buf[8];
    for (int i = 0; i < decInfo->size_secret_file; i++)
    {
        if (fread(image_buf, 1, 8, decInfo->fptr_stego_image) != 8)
            return e_failure;

        char ch = decode_char_from_lsb(image_buf);
        fputc(ch, decInfo->fptr_output_file);
    }

    fclose(decInfo->fptr_output_file);
    return e_success;
}

/* Complete Decoding Flow */
Status do_decoding(DecodeInfo *decInfo)
{
    printf("INFO: Opening stego image file...\n");
    if (open_decode_files(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to open stego image\n");
        return e_failure;
    }
    printf("SUCCESS: Stego image file opened.\n");

    printf("INFO: Decoding Magic String...\n");
    if (decode_magic_string(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Magic string mismatch! File is not encoded properly.\n");
        return e_failure;
    }
    printf("SUCCESS: Magic string matched successfully!\n");

    printf("INFO: Decoding Secret File Extension...\n");
    if (decode_secret_file_extn(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to decode extension\n");
        return e_failure;
    }
    printf("SUCCESS: Decoded Extension = %s\n", decInfo->extn_output_file);

    printf("INFO: Decoding Secret File Size...\n");
    if (decode_secret_file_size(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to decode secret file size\n");
        return e_failure;
    }
    printf("SUCCESS: Decoded Secret File Size = %d bytes\n", decInfo->size_secret_file);

    printf("INFO: Decoding Secret File Data...\n");
    if (decode_secret_file_data(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to decode secret file data\n");
        return e_failure;
    }
    printf("SUCCESS: Decoded secret data stored in '%s'\n", decInfo->output_fname);

    fclose(decInfo->fptr_stego_image);
    return e_success;
}