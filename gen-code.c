#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <png.h>

typedef struct {
    int rows;
    int cols;
    int map[200][320];
} Image;

Image* readPNGImage(char* filename) {
    Image *image = malloc(sizeof(Image));
    png_byte color_type;
    png_bytep * row_pointers;
    png_structp png_ptr;
    png_infop info_ptr;
    int i, j;

    // 8 is the maximum size that can be checked
    char header[8];

    /* open file and test for it being a png */
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "[read_png_file] File %s could not be opened for reading", filename);
        return NULL;
    }

    if (fread(header, 1, 8, fp) != 8){
       fprintf(stderr, "[read_png_file] File %s is not is too short", filename);
       return NULL;
    }

    if (png_sig_cmp(header, 0, 8)){
       fprintf(stderr, "[read_png_file] File %s is not recognized as a PNG file", filename);
       return NULL;
    }

    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr){
        fprintf(stderr, "[read_png_file] png_create_read_struct failed");
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr){
        fprintf(stderr, "[read_png_file] png_create_info_struct failed");
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))){
        fprintf(stderr, "[read_png_file] Error during init_io");
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    image->cols = png_get_image_width(png_ptr, info_ptr);
    image->rows = png_get_image_height(png_ptr, info_ptr);

    color_type = png_get_color_type(png_ptr, info_ptr);
    if(color_type != PNG_COLOR_TYPE_RGB){
        fprintf(stderr, "[read_png_file] Only RGB PNGs are supported");
        return NULL;
    }
    png_read_update_info(png_ptr, info_ptr);

    /* read file */
    if (setjmp(png_jmpbuf(png_ptr))){
        fprintf(stderr, "[read_png_file] Error during read_image");
        return NULL;
    }

    /* memory allocation */
    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * image->rows);
    for (i = 0; i < image->rows; i += 1){
        row_pointers[i] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
    }

    png_read_image(png_ptr, row_pointers);
    fclose(fp);

    for (i = 0; i < image->rows; i += 1) {
        png_byte* row = row_pointers[i];
        for (j = 0; j < image->cols; j += 1) {
            png_byte* ptr = &(row[j * 3]);
            image->map[i][j] = ptr[0];
        }
    }

    /* clean up */
    for (i = 0; i < image->rows; i += 1){
        free(row_pointers[i]);
    }
    free(row_pointers);

    return image;
}

unsigned char code[2048];
int offset;

void assert_bytes(FILE *fp, int bytes) {
    int left = (sizeof(code) - 1) - (offset + 3);
    if (left >= bytes) {
        return;
    }

    while (left >= 2) {
        code[offset++] = 0x80; // BRA rel
        code[offset++] = 0x00;
        left -= 2;
    }
    while (left >= 1) {
        code[offset++] = 0xEA; // NOP
        left--;
    }

    code[offset++] = 0x4C; // JMP abs
    code[offset++] = 0x00;
    code[offset++] = 0xC8;

    fwrite(code, sizeof(code), 1, fp);
    memset(code, 0, sizeof(code));
    offset = 0;
}

void set_addr(FILE *fp, int addr) {
    assert_bytes(fp, 2);
    code[offset++] = 0xA9; // LDA imm
    code[offset++] = addr;

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3E;
    code[offset++] = 0xC0;

    assert_bytes(fp, 2);
    code[offset++] = 0xA9; // LDA imm
    code[offset++] = addr>>8;

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3F;
    code[offset++] = 0xC0;
}

long put_raw(FILE *fp, FILE *raw)
{
    static long size = 0;
    static int addr[2] = {0x0000, 0x8000};

    unsigned char buffer[877][2];
    if (fread(buffer, sizeof(buffer), 1, raw) != 1) {
        return size;
    }

    for (int c = 0; c < 2; c++) {
        set_addr(fp, addr[c]);

        for (int i = 0; i < 877; i++) {
            int val = buffer[i][c];

            if (val == 0x00) {
                val = 0x01;
            }

            assert_bytes(fp, 2);
            code[offset++] = 0xA9; // LDA imm
            code[offset++] = val;

            assert_bytes(fp, 3);
            code[offset++] = 0x8D; // STA abs
            code[offset++] = 0x3D;
            code[offset++] = 0xC0;

            size++;
            addr[c]++;

            if (addr[c] % 0x8000 == 0x0000) {
                addr[c] -= 0x8000;
                if (i < 876) {
                    set_addr(fp, addr[c]);
                }
            }
        }
    }
    return size;
}

int main(int argc, char **argv) {
    char name[100];
    Image *image[2];
    int img = 1;

    static int mod_stat[370];

    long total_mod = 0;

    int max_mod = 0;
    int max_mod_img = 0;

    FILE *fp = fopen("Bad-Apple-IIgs.code", "wb");

    FILE *raw = fopen("data/bad-apple.raw", "rb");

    while (put_raw(fp, raw) < 60000) {
    }

    ////////////////////////////////////////////// -> DOC
    assert_bytes(fp, 2);
    code[offset++] = 0xA9; // LDA imm
    code[offset++] = 0b00100000; // DOC regs, auto-inc

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3C;
    code[offset++] = 0xC0;

    ////////////////////////////////////////////// OSC 1 -> Sync
    assert_bytes(fp, 2);
    code[offset++] = 0xA9; // LDA imm
    code[offset++] = 0xA1; // Ctrl, OSC 1

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3E;
    code[offset++] = 0xC0;

    assert_bytes(fp, 2);
    code[offset++] = 0xA9; // LDA imm
    code[offset++] = 0b00000100; // Right, no IRQ, sync, go

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3D;
    code[offset++] = 0xC0;

    ////////////////////////////////////////////// OSC 0 -> Free Run
    assert_bytes(fp, 2);
    code[offset++] = 0xA9; // LDA imm
    code[offset++] = 0xA0; // Ctrl, OSC 0

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3E;
    code[offset++] = 0xC0;

    assert_bytes(fp, 2);
    code[offset++] = 0xA9; // LDA imm
    code[offset++] = 0b00010000; // Left, no IRQ, free run, go

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3D;
    code[offset++] = 0xC0;

    ////////////////////////////////////////////// OSC 1 -> Free Run
    assert_bytes(fp, 2);
    code[offset++] = 0xA9; // LDA imm
    code[offset++] = 0b00000000; // Right, no IRQ, free run, go

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3D;
    code[offset++] = 0xC0;

    ////////////////////////////////////////////// -> RAM
    assert_bytes(fp, 4);
    code[offset++] = 0xAF; // LDA long
    code[offset++] = 0xCA;
    code[offset++] = 0x00;
    code[offset++] = 0xE1;

    assert_bytes(fp, 2);
    code[offset++] = 0x29; // AND imm
    code[offset++] = 0b00001111; // System volume

    assert_bytes(fp, 2);
    code[offset++] = 0x09; // ORA imm
    code[offset++] = 0b01100000; // DOC RAM, auto-inc

    assert_bytes(fp, 3);
    code[offset++] = 0x8D; // STA abs
    code[offset++] = 0x3C;
    code[offset++] = 0xC0;

    sprintf(name, "data/bad-apple-%d.png", img);
    image[0] = readPNGImage(name);
    if (!image[0] || image[0]->cols != 320 || image[0]->rows != 200) {
        return 1;
    }
    img++;

    while (img < 6556) {

        put_raw(fp, raw);

        sprintf(name, "data/bad-apple-%d.png", img);
        image[1] = readPNGImage(name);
        if (!image[1] || image[1]->cols != 320 || image[1]->rows != 200) {
            return 1;
        }

        int mod = 0;

        for (int y = 0; y < 200; y++) {
            int edge[2][320];

            for (int i = 0; i < 2; i++) {
                edge[i][0] = image[i]->map[y][0] ? 1 : -1;

                for (int x = 1; x < 320; x++) {
                    if (image[i]->map[y][x] == image[i]->map[y][x-1]) {
                        edge[i][x] = 0;
                    }
                    if (image[i]->map[y][x] > image[i]->map[y][x-1]) {
                        edge[i][x] = 1;
                    }
                    if (image[i]->map[y][x] < image[i]->map[y][x-1]) {
                        edge[i][x] = -1;
                    }
                }
            }

            for (int x = 0; x < 320; x += 2) {
                if (edge[1][x]   != edge[0][x] ||
                    edge[1][x+1] != edge[0][x+1]) {

                    int val = 0;
                    switch (edge[1][x]) {
                        case 0:
                            val |= 0x00;
                            break;
                        case -1:
                            val |= 0x10;
                            break;
                        case 1:
                            val |= 0x20;
                            break;
                    }
                    switch (edge[1][x+1]) {
                        case 0:
                            val |= 0x00;
                            break;
                        case -1:
                            val |= 0x01;
                            break;
                        case 1:
                            val |= 0x02;
                            break;
                    }

                    assert_bytes(fp, 2);
                    code[offset++] = 0xA9; // LDA imm
                    code[offset++] = val;

                    assert_bytes(fp, 3);
                    int addr = 0x2000 + y * (320/2) + x/2;
                    code[offset++] = 0x8D; // STA abs
                    code[offset++] = addr;
                    code[offset++] = addr>>8;

                    mod++;
                }
            }
        }

        if (mod < 1200) {
            assert_bytes(fp, 2);
            code[offset++] = 0xA9; // LDA imm
            code[offset++] = 0x30;

            assert_bytes(fp, 3);
            code[offset++] = 0x20; // JSR
            code[offset++] = 0xA8;
            code[offset++] = 0xFC;
        }

        assert_bytes(fp, 5);
        code[offset++] = 0x2C; // BIT abs
        code[offset++] = 0x19;
        code[offset++] = 0xC0;
        code[offset++] = 0x10; // BPL
        code[offset++] = 0xFB;

        assert_bytes(fp, 5);
        code[offset++] = 0x2C; // BIT abs
        code[offset++] = 0x19;
        code[offset++] = 0xC0;
        code[offset++] = 0x30; // BMI
        code[offset++] = 0xFB;

        if (mod > 2370) {
            printf("mod %d: %d\n", img, mod);
        }

        mod_stat[mod / 10]++;

        total_mod += mod;

        if (mod > max_mod) {
            max_mod = mod;
            max_mod_img = img;
        }

        free(image[0]);
        image[0] = image[1];
        img++;
    }

    assert_bytes(fp, 2);
    code[offset++] = 0x80; // BRA
    code[offset++] = 0xFE;
    fwrite(code, sizeof(code), 1, fp);

    fclose(fp);

    for (int i = 0; i < 370; i++) {
        printf("%04d: %d\n", (i + 1) * 10, mod_stat[i]);
    }

    printf("total mod %ld\n", total_mod);

    printf("max mod %d: %d\n", max_mod_img, max_mod);

    return 0;
}
