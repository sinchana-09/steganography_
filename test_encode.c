#include <stdio.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

int main(int argc, char *argv[])
{
    // Check operation type (-e or -d)
    OperationType type = check_operation_type(argv);

    if (type == e_encode)
    {
        EncodeInfo encInfo;
        printf("INFO: Operation detected as Encoding\n");

        if (read_and_validate_encode_args(argv, &encInfo) == e_success)
        {
            printf("INFO: Argument validation successful!\n");

            if (do_encoding(&encInfo) == e_success)
            {
                printf("SUCCESS: Encoding completed successfully!\n");
            }
            else
            {
                printf("ERROR: Encoding failed!\n");
                return 1;
            }
        }
        else
        {
            printf("ERROR: Argument validation failed.\n");
            printf("Usage: ./a.out -e <source_image.bmp> <secret_file.txt> [stego_image.bmp]\n");
            return 1;
        }
    }
    else if (type == e_decode)
    {
        DecodeInfo decInfo;
        printf("INFO: Operation detected as Decoding\n");

        if (read_and_validate_decode_args(argv, &decInfo) == e_success)
        {
            printf("INFO: Argument validation successful!\n");

            if (do_decoding(&decInfo) == e_success)
            {
                printf("SUCCESS: Decoding completed successfully!\n");
            }
            else
            {
                printf("ERROR: Decoding failed!\n");
                return 1;
            }
        }
        else
        {
            printf("ERROR: Argument validation failed.\n");
            printf("Usage: ./a.out -d <stego_image.bmp> [output_file_name]\n");
            return 1;
        }
    }
    else
    {
        printf("ERROR: Unsupported operation flag.\n");
        printf("Usage:\n");
        printf(" Encoding: ./a.out -e <source.bmp> <secret.txt> [stego.bmp]\n");
        printf(" Decoding: ./a.out -d <stego.bmp> [output_file_name]\n");
        return 1;
    }

    return 0;
}