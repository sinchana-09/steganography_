#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encode.h"
#include "common.h"
#include "types.h"

/* Get image capacity (width * height * 3 bytes per pixel) */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    fseek(fptr_image, 18, SEEK_SET);

    fread(&width, sizeof(int), 1, fptr_image);
    fread(&height, sizeof(int), 1, fptr_image);

    return width * height * 3;
}

/* Get file size in bytes */
uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    uint size = ftell(fptr);
    rewind(fptr);
    return size;
}

/* Check operation type based on CLI flag */
OperationType check_operation_type(char *argv[])
{
    if (argv[1] == NULL)
    {
        return e_unsupported;
    }
    if (strcmp(argv[1], "-e") == 0)
    {
        return e_encode;
    }
    else if (strcmp(argv[1], "-d") == 0)
    {
        return e_decode;
    }
    return e_unsupported;
}

/* Read and validate Encode args from argv with extension validation */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    // 1. Validate Source Image (.bmp)
    if (argv[2] != NULL && strstr(argv[2], ".bmp") != NULL)
    {
        encInfo->src_image_fname = argv[2];
    }
    else
    {
        fprintf(stderr, "ERROR: Source image must be a .bmp file\n");
        return e_failure;
    }

    // 2. Validate Secret File Extension (Check if '.' is present)
    if (argv[3] != NULL)
    {
        char *ext = strrchr(argv[3], '.');
        if (ext != NULL) // Contains '.' -> Has valid extension
        {
            encInfo->secret_fname = argv[3];
            strncpy(encInfo->extn_secret_file, ext, MAX_FILE_SUFFIX - 1);
            encInfo->extn_secret_file[MAX_FILE_SUFFIX - 1] = '\0';
        }
        else
        {
            fprintf(stderr, "ERROR: Secret file '%s' does not contain an extension ('.' missing)\n", argv[3]);
            return e_failure;
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Secret file argument missing\n");
        return e_failure;
    }

    // 3. Validate / Assign Stego Image Name
    if (argv[4] != NULL)
    {
        if (strstr(argv[4], ".bmp") != NULL)
        {
            encInfo->stego_image_fname = argv[4];
        }
        else
        {
            fprintf(stderr, "ERROR: Output stego file must be a .bmp file\n");
            return e_failure;
        }
    }
    else
    {
        encInfo->stego_image_fname = "stego.bmp";
    }

    return e_success;
}

/* Open files safely with error handling */
Status open_files(EncodeInfo *encInfo)
{
    // Source Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);
        return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);
        return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);
        return e_failure;
    }

    return e_success;
}

/* Check if the source image capacity is sufficient */
Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    // Header size (54) + ( MagicString + SecretExtnSize + SecretExtn + SecretFileSize + SecretData ) * 8 bits
    long required_capacity = 54 + (strlen(MAGIC_STRING) + 4 + strlen(encInfo->extn_secret_file) + 4 + encInfo->size_secret_file) * 8;

    if (encInfo->image_capacity > required_capacity)
    {
        return e_success;
    }
    return e_failure;
}

/* Copy original BMP 54-byte header */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char header[54];
    fseek(fptr_src_image, 0, SEEK_SET);

    if (fread(header, 1, 54, fptr_src_image) != 54)
    {
        return e_failure;
    }

    if (fwrite(header, 1, 54, fptr_dest_image) != 54)
    {
        return e_failure;
    }

    return e_success;
}

/* Encode a single byte into 8 LSBs of image buffer */
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        // Clear LSB bit of image byte and set it with corresponding bit of data byte
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((data >> (7 - i)) & 1);
    }
    return e_success;
}

/* Encode a 32-bit integer into 32 LSBs of image buffer */
Status encode_size_to_lsb(int size, char *image_buffer)
{
    for (int i = 0; i < 32; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((size >> (31 - i)) & 1);
    }
    return e_success;
}

/* Encode string/data array into stego image */
Status encode_data_to_image(const char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char image_buffer[8];

    for (int i = 0; i < size; i++)
    {
        if (fread(image_buffer, 1, 8, fptr_src_image) != 8)
        {
            return e_failure;
        }

        encode_byte_to_lsb(data[i], image_buffer);

        if (fwrite(image_buffer, 1, 8, fptr_stego_image) != 8)
        {
            return e_failure;
        }
    }
    return e_success;
}

/* Encode Magic String */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    return encode_data_to_image(magic_string, strlen(magic_string), encInfo->fptr_src_image, encInfo->fptr_stego_image);
}

/* Encode Secret File Extension */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    int extn_size = strlen(file_extn);
    char buffer[32];

    // 1. Encode extension length (int - 32 bits)
    if (fread(buffer, 1, 32, encInfo->fptr_src_image) != 32)
        return e_failure;
    
    encode_size_to_lsb(extn_size, buffer);
    
    if (fwrite(buffer, 1, 32, encInfo->fptr_stego_image) != 32)
        return e_failure;

    // 2. Encode extension string
    return encode_data_to_image(file_extn, extn_size, encInfo->fptr_src_image, encInfo->fptr_stego_image);
}

/* Encode Secret File Size */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char buffer[32];

    if (fread(buffer, 1, 32, encInfo->fptr_src_image) != 32)
        return e_failure;

    encode_size_to_lsb((int)file_size, buffer);

    if (fwrite(buffer, 1, 32, encInfo->fptr_stego_image) != 32)
        return e_failure;

    return e_success;
}

/* Encode Secret File Data */
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char ch;
    char image_buffer[8];

    rewind(encInfo->fptr_secret);

    while (fread(&ch, 1, 1, encInfo->fptr_secret) > 0)
    {
        if (fread(image_buffer, 1, 8, encInfo->fptr_src_image) != 8)
            return e_failure;

        encode_byte_to_lsb(ch, image_buffer);

        if (fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
            return e_failure;
    }

    return e_success;
}

/* Copy remaining un-encoded bytes from source BMP to output BMP */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fptr_src)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, fptr_dest) != bytes_read)
        {
            return e_failure;
        }
    }
    return e_success;
}

/* High level function to run all encoding steps sequentially */
Status do_encoding(EncodeInfo *encInfo)
{
    printf("INFO: Opening required files...\n");
    if (open_files(encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Opening files failed\n");
        return e_failure;
    }
    printf("SUCCESS: Opened files successfully.\n");

    printf("INFO: Checking capacity...\n");
    if (check_capacity(encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Image size insufficient for hiding data\n");
        return e_failure;
    }
    printf("SUCCESS: Capacity check passed.\n");

    printf("INFO: Copying BMP Header...\n");
    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to copy BMP header\n");
        return e_failure;
    }
    printf("SUCCESS: Header copied.\n");

    printf("INFO: Encoding Magic String...\n");
    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode Magic String\n");
        return e_failure;
    }
    printf("SUCCESS: Magic String encoded.\n");

    printf("INFO: Encoding Secret File Extension...\n");
    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode secret file extension\n");
        return e_failure;
    }
    printf("SUCCESS: Extension encoded.\n");

    printf("INFO: Encoding Secret File Size...\n");
    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode secret file size\n");
        return e_failure;
    }
    printf("SUCCESS: Secret file size encoded.\n");

    printf("INFO: Encoding Secret File Data...\n");
    if (encode_secret_file_data(encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode secret data\n");
        return e_failure;
    }
    printf("SUCCESS: Secret file data encoded.\n");

    printf("INFO: Copying remaining image data...\n");
    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to copy remaining image bytes\n");
        return e_failure;
    }
    printf("SUCCESS: Remaining data copied.\n");

    // Close open files
    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_success;
}