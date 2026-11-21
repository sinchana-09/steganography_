# LSB Image Steganography in C

A command-line steganography tool written in C that hides secret text files inside 24-bit uncompressed BMP images using **Least Significant Bit (LSB)** manipulation, without altering the visual appearance of the image.

## Project Structure

* `test_encode.c` — Main driver program handling command-line arguments and routing between encoding and decoding.
* `encode.c` / `encode.h` — Functions and structures for embedding the magic string, secret file extension, file size, and secret data into the cover BMP image.
* `decode.c` / `decode.h` — Functions and structures for extracting hidden data from a stego BMP image and reconstructing the original secret file.
* `common.h` — Shared constants such as the magic string identifier (`#*`).
* `types.h` — Custom data types (`uint`) and status/operation enums (`Status`, `OperationType`).
* `beautiful.bmp` — Sample 24-bit uncompressed source BMP image.
* `secret.txt` — Sample text file containing the secret message to hide.
* `stego_img.bmp` — Output BMP image containing the hidden data.
* `output.txt` — Extracted secret message generated after decoding.

## How It Works

1. **Encoding (`-e`):**
   * Validates the source `.bmp` file and checks that its image capacity is large enough to hold the secret file.
   * Copies the 54-byte BMP header unchanged to the output image.
   * Embeds a magic string signature (`#*`), the secret file's extension (`.txt`), the secret file's size, and the actual secret bytes into the least significant bits of the image's RGB pixel bytes.
   * Copies the remaining pixel data to produce `stego_img.bmp`.

2. **Decoding (`-d`):**
   * Skips the 54-byte BMP header of `stego_img.bmp`.
   * Reads the LSBs to verify the magic string signature.
   * Extracts the file extension and file size, then reconstructs the hidden message into `output.txt`.

## Compilation & Usage

### 1. Compile the Project
```bash
gcc *.c -o stego

```

### 2. Encode a Secret File into an Image
```bash
./stego -e beautiful.bmp secret.txt stego_img.bmp
```

### 3. Decode the Secret File from the Stego Image
```bash
./stego -d stego_img.bmp output.txt
```

## Requirements
* GCC compiler (or any standard C compiler)
* Uncompressed 24-bit `.bmp` image files
