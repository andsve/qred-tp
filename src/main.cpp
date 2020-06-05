#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>


#include <stretchy_buffer.h>

#include <xxhash.h>

#include "parg.h"

// #include "../asd.h"

enum pack_entry_type
{
    IMAGE,
    FONT
};

struct pack_entry_t
{
    pack_entry_type type;
    char filepath[4096];
    char filename[4096];
    char name[4096];
    int width;
    int height;
    int channels;
    uint8_t* data;
    stbtt_packedchar* packed_char_data;
};

static int font_width = 256;
static int font_height = 256;
static float font_size = 28.0f;
// static int font_range = 96;
static int font_range = 256;
static int font_range_start = 32;

static int output_width = 1024;
static int output_height = 1024;
static uint8_t* output_data = NULL;
static uint8_t* output_data_compressed = NULL;
static int output_data_compressed_size = 0;

static pack_entry_t* image_entries = NULL;
static stbrp_rect* image_rects = NULL;

void remove_file_ending(char* filename)
{
    int l = strlen(filename);
    for (int i = l-1; i >= 0; --i)
    {
        if (filename[i] == '.') {
            filename[i] = '\0';
            break;
        }
    }
}

const char* get_file_ending(const char* filename)
{
    int l = strlen(filename);
    for (int i = l-1; i >= 0; --i)
    {
        if (filename[i] == '.') {
            return &filename[i+1];
        }
    }

    return filename;
}

bool find_files(const char* dir_path)
{
    struct dirent *dp;
    DIR *dfd;

    if ((dfd = opendir(dir_path)) == NULL)
    {
        printf("Can't open %s\n", dir_path);
        return false;
    }

    char filename_qfd[4096];

    while ((dp = readdir(dfd)) != NULL)
    {
        struct stat stbuf;
        sprintf(filename_qfd, "%s/%s", dir_path, dp->d_name);
        if(stat(filename_qfd,&stbuf ) == -1)
        {
            printf("Unable to stat file: %s\n", filename_qfd);
            continue;
        }

        if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            continue;
            // Skip directories
        }
        else
        {
            // new entry
            pack_entry_t pack_entry;
            strcpy(pack_entry.filepath, filename_qfd);
            strcpy(pack_entry.filename, dp->d_name);
            strcpy(pack_entry.name, dp->d_name);
            remove_file_ending(pack_entry.name);

            const char* file_ending = get_file_ending(dp->d_name);
            printf("file_ending: %s\n", file_ending);

            if (strcmp(file_ending, "png") == 0)
            {
                pack_entry.type = IMAGE;

                // load image
                int n;
                pack_entry.data = stbi_load(filename_qfd, &pack_entry.width, &pack_entry.height, &n, 4);
            }
            else if (strcmp(file_ending, "ttf") == 0)
            {
                pack_entry.type = FONT;

                // load ttf data
                uint8_t* ttf_data = (uint8_t*)malloc(stbuf.st_size);
                FILE* fttf = fopen(filename_qfd, "rb");
                fread(ttf_data, 1, stbuf.st_size, fttf);
                fclose(fttf);

                // stbtt_fontinfo fontinfo;
                // int r = stbtt_InitFont(&fontinfo, ttf_data, stbtt_GetFontOffsetForIndex(ttf_data, 0));
                // if (r != 0) {
                //     printf("Could not init font!\n");
                //     exit(1);
                // }


                // init packing
                stbtt_pack_context pack_context;
                uint8_t* font_pixels = (uint8_t*)malloc(font_width*font_height);
                int r = stbtt_PackBegin(&pack_context, font_pixels, font_width, font_height, 0, 1, 0);
                if (r != 1) {
                    printf("Error while calling stbtt_PackBegin!\n");
                    exit(1);
                }

                // stbtt_packedchar packed_char_data[font_range];
                pack_entry.packed_char_data = (stbtt_packedchar*)malloc(font_range * sizeof(stbtt_packedchar));
                r = stbtt_PackFontRange(&pack_context, ttf_data, stbtt_GetFontOffsetForIndex(ttf_data, 0), font_size, font_range_start, font_range, pack_entry.packed_char_data);
                if (r != 1) {
                    printf("Error while calling stbtt_PackFontRange!\n");
                    exit(1);
                }

                // for (int i = 0; i < font_range; ++i)
                // {
                //     printf("%d, %hu %hu %hu %hu\n", i, packed_char_data[i].x0, packed_char_data[i].y0, packed_char_data[i].x1, packed_char_data[i].y1);
                // }
                pack_entry.data = font_pixels;
                pack_entry.width = font_width;
                pack_entry.height = font_height;

                // cleanup
                stbtt_PackEnd(&pack_context);
                free(ttf_data);

            } else {
                printf("Unknown file extension, skipping: %s\n", dp->d_name);
                continue;
            }

            // push it to the list of images
            sb_push(image_entries, pack_entry);

        }
    }

    return true;
}

void create_rects()
{
    int c = sb_count(image_entries);
    image_rects = (stbrp_rect*)malloc(c*sizeof(stbrp_rect));

    for (int i = 0; i < c; ++i)
    {
        image_rects[i].id = i;
        image_rects[i].w = image_entries[i].width;
        image_rects[i].h = image_entries[i].height;
    }
}

bool pack_images()
{
    int c = sb_count(image_entries);

    printf("Initializing target...\n");
    stbrp_context context;
    stbrp_node* nodes = (stbrp_node*)malloc(c*sizeof(stbrp_node));
    stbrp_init_target(&context, output_width, output_height, nodes, c);

    printf("Packing %d images...\n", c);
    int r = stbrp_pack_rects(&context, image_rects, c);
    if (r == 1) {
        printf("Success!\n");
    } else {
        printf("FAIL! Couldn't fit, consider changing atlas resolution!\n");
    }

    return r == 1;
}

void blit_image_rect(stbrp_rect& image_rect, pack_entry_t& pack_entry)
{
    uint64_t p = (image_rect.y * output_width) + image_rect.x;

    for (int y = 0; y < image_rect.h; ++y)
    {
        for (int x = 0; x < image_rect.w; ++x)
        {
            uint64_t i = y*image_rect.w+x;
            output_data[p*4+0] = pack_entry.data[i*4+0];
            output_data[p*4+1] = pack_entry.data[i*4+1];
            output_data[p*4+2] = pack_entry.data[i*4+2];
            output_data[p*4+3] = pack_entry.data[i*4+3];
            p += 1;
        }
        p += output_width - image_rect.w;
    }

}

void blit_font_image_rect(stbrp_rect& image_rect, pack_entry_t& pack_entry)
{
    uint64_t p = (image_rect.y * output_width) + image_rect.x;

    for (int y = 0; y < image_rect.h; ++y)
    {
        for (int x = 0; x < image_rect.w; ++x)
        {
            uint64_t i = y*image_rect.w+x;
            // output_data[p*4+0] = pack_entry.data[i];
            // // output_data[p*4+1] = pack_entry.data[i*4+1];
            // // output_data[p*4+2] = pack_entry.data[i*4+2];
            // output_data[p*4+3] = 255;
            output_data[p*4+0] = pack_entry.data[i];
            output_data[p*4+1] = pack_entry.data[i];
            output_data[p*4+2] = pack_entry.data[i];
            output_data[p*4+3] = pack_entry.data[i];
            p += 1;
        }
        p += output_width - image_rect.w;
    }

}

void generate_atlas_image()
{
    uint64_t s = output_width*output_height;
    output_data = (uint8_t*)malloc(s*4*sizeof(uint8_t));

    // clear to transparent
    for (uint64_t p = 0; p < s; p+=1)
    {
        output_data[p*4+0] = 0;
        output_data[p*4+1] = 0;
        output_data[p*4+2] = 0;
        output_data[p*4+3] = 0;
    }
    // for (int p = 0; p < s*4; p+=4)
    // {
    //     output_data[p] = 255;
    //     output_data[p+1] = (p / 4) * 32;
    //     output_data[p+2] = 0;
    //     output_data[p+3] = 255;
    // }

    int c = sb_count(image_entries);
    for (int i = 0; i < c; ++i)
    {
        stbrp_rect& image_rect = image_rects[i];
        pack_entry_t& pack_entry = image_entries[image_rect.id];
        if (pack_entry.type == IMAGE) {
            blit_image_rect(image_rect, pack_entry);
        } else if (pack_entry.type == FONT) {
            blit_font_image_rect(image_rect, pack_entry);
        } else {
            printf("Unable to blit unknown type: %d\n", pack_entry.type);
        }

    }
}

bool write_atlas_meta_to_disk(const char* output_filepath)
{
    printf("Writing meta data...\n");
    bool r = true;
    FILE *fmeta = fopen("meta.txt", "w");

    float ow = (float)output_width;
    float oh = (float)output_height;

    int c = sb_count(image_entries);
    for (int i = 0; i < c; ++i)
    {
        stbrp_rect& image_rect = image_rects[i];
        pack_entry_t& pack_entry = image_entries[image_rect.id];
        // printf("%d: %s [%d,%d] %s\n", image_rect.id, pack_entry.filename, image_rect.x, image_rect.y, image_rect.was_packed ? "" : "[NOT PACKED!]");
        float u0 = (float)image_rect.x / ow;
        float v0 = (float)(output_height - image_rect.y) / oh;
        float u1 = (float)(image_rect.x + image_rect.w) / ow;
        float v1 = (float)(output_height - image_rect.y - image_rect.h) / oh;
        fprintf(fmeta, "%s: %f %f %f %f\n", pack_entry.name, u0, v0, u1, v1);
    }

// meta_close_and_exit:
    fclose(fmeta);
    return r;
}

bool write_atlas_image_to_disk(const char* output_filepath)
{
    printf("Writing output file: %s\n", output_filepath);
    int r = stbi_write_png(output_filepath, output_width, output_height, 4, output_data, output_width*4);
    if (r != 0) {
        printf("Success!\n");
    } else {
        printf("Fail!\n");
    }

    return r != 0;
}

void store_header_image_data(void *context, void *data, int size)
{
    output_data_compressed = (uint8_t*)malloc(size);
    memcpy(output_data_compressed, data, size);
    output_data_compressed_size = size;
}

XXH64_hash_t XXH64str(const char* str)
{
    return XXH64(str, strlen(str), 0);
}

XXH32_hash_t XXH32str(const char* str)
{
    return XXH32(str, strlen(str), 0);
}

bool write_atlas_to_header(const char* output_filepath)
{
    printf("Compressing image data...\n");
    int r = stbi_write_png_to_func(store_header_image_data, NULL, output_width, output_height, 4, output_data, output_width*4);
    if (r != 0) {
        printf("Success!\n");


        printf("Writing output file: %s\n", output_filepath);
        FILE *fheader = fopen(output_filepath, "w");
        fprintf(fheader, "#pragma once\n");

        // write png data blob
        fprintf(fheader, "static const unsigned char atlas_image_data[%d] = {\n", output_data_compressed_size);
        for (int i = 0; i < output_data_compressed_size; ++i)
        {
            fprintf(fheader, "0x%02x%s%s", output_data_compressed[i], i != output_data_compressed_size-1 ? "," : "", i % 16 == 15 ? "\n" : " ");
        }
        fprintf(fheader, "};\n");

        // write image meta
        fprintf(fheader, "struct atlas_entry_t {\n");
        // fprintf(fheader, "    const char* filename;\n");
        // fprintf(fheader, "    XXH64_hash_t filename;\n");
        fprintf(fheader, "    XXH32_hash_t filename;\n");
        fprintf(fheader, "    float u0;\n");
        fprintf(fheader, "    float v0;\n");
        fprintf(fheader, "    float u1;\n");
        fprintf(fheader, "    float v1;\n");
        fprintf(fheader, "};\n");

        int c = sb_count(image_entries);
        int ic = 0;
        int fc = 0;
        for (int i = 0; i < c; ++i)
        {
            pack_entry_t& pack_entry = image_entries[i];
            if (pack_entry.type == IMAGE) {
                ic++;
            } else if (pack_entry.type == FONT) {
                fc++;
            }
        }
        fprintf(fheader, "static const atlas_entry_t atlas_entries[%d] = {\n", ic+1);

        float ow = (float)output_width;
        float oh = (float)output_height;

        bool found_font = false;
        for (int i = 0; i < c; ++i)
        {
            stbrp_rect& image_rect = image_rects[i];
            pack_entry_t& pack_entry = image_entries[image_rect.id];

            if (pack_entry.type == FONT)
            {
                found_font = true;
                continue;
            }

            float u0 = (float)image_rect.x / ow;
            float v0 = (float)(image_rect.y) / oh;
            float u1 = (float)(image_rect.x + image_rect.w) / ow;
            float v1 = (float)(image_rect.y + image_rect.h) / oh;

            // fprintf(fheader, "  { \"%s\", %f, %f, %f, %f },\n", pack_entry.filename, u0, v0, u1, v1);
            fprintf(fheader, "  { 0x%x, %f, %f, %f, %f }, // %s\n", XXH32str(pack_entry.name), u0, v0, u1, v1, pack_entry.filename);
        }
        fprintf(fheader, "  { 0x0, 0.0f, 0.0f, 1.0f, 1.0f }, // full image\n");

        fprintf(fheader, "};\n");

        // write font meta
        if (found_font) {
            // typedef struct
            // {
            //    unsigned short x0,y0,x1,y1; // coordinates of bbox in bitmap
            //    float xoff,yoff,xadvance;
            //    float xoff2,yoff2;
            // } stbtt_packedchar;

            // stbtt_packedchar* pack_entry.packed_char_data
            fprintf(fheader, "struct font_char_entry_t {\n");
            fprintf(fheader, "    float w;\n");
            fprintf(fheader, "    float h;\n");
            fprintf(fheader, "    float u0;\n");
            fprintf(fheader, "    float v0;\n");
            fprintf(fheader, "    float u1;\n");
            fprintf(fheader, "    float v1;\n");
            fprintf(fheader, "    float xoff;\n");
            fprintf(fheader, "    float yoff;\n");
            fprintf(fheader, "    float xadvance;\n");
            fprintf(fheader, "    float xoff2;\n");
            fprintf(fheader, "    float yoff2;\n");
            fprintf(fheader, "};\n");

            fprintf(fheader, "struct font_entry_t {\n");
            fprintf(fheader, "    XXH32_hash_t font_name;\n");
            fprintf(fheader, "    uint32_t cp_start;\n");
            fprintf(fheader, "    uint32_t cp_count;\n");
            fprintf(fheader, "    font_char_entry_t font_chars[%d];\n", font_range);
            fprintf(fheader, "};\n");

            fprintf(fheader, "static const font_entry_t font_entries[%d] = {\n", fc);

            for (int i = 0; i < c; ++i)
            {
                stbrp_rect& image_rect = image_rects[i];
                pack_entry_t& pack_entry = image_entries[image_rect.id];
                if (pack_entry.type != FONT)
                {
                    continue;
                }

                float img_u0 = (float)image_rect.x / ow;
                float img_v0 = (float)image_rect.y / oh;

                float iw = (float)image_rect.w;
                float ih = (float)image_rect.h;
                float dx = iw / ow;
                float dy = ih / oh;

                // float img_u1 = (float)(image_rect.x + image_rect.w) / ow;
                // float img_v1 = (float)(image_rect.y + image_rect.h) / oh;

                fprintf(fheader, "  { 0x%x, %d, %d, { // %s\n", XXH32str(pack_entry.name), font_range_start, font_range, pack_entry.filename);
                for (int j = 0; j < font_range; ++j)
                {
                    stbtt_packedchar& packedchar = pack_entry.packed_char_data[j];

                    // float u0 = img_u0;// + packedchar.x0;
                    // float v0 = img_v0;// + packedchar.y0;
                    // float u1 = img_u0;
                    // float v1 = img_v0;

                    // u0 += dx * packedchar.x0;
                    // v0 += dy * packedchar.y0;
                    // u1 += dx * packedchar.x1;
                    // v1 += dy * packedchar.y1;
                    float u0 = (float)packedchar.x0 / iw;
                    float v0 = (float)packedchar.y0 / ih;
                    float u1 = (float)packedchar.x1 / iw;
                    float v1 = (float)packedchar.y1 / ih;

                    u0 *= dx;
                    v0 *= dy;
                    u1 *= dx;
                    v1 *= dy;

                    u0 += img_u0;
                    v0 += img_v0;
                    u1 += img_u0;
                    v1 += img_v0;

                    float c_w = packedchar.x1-packedchar.x0;
                    float c_h = packedchar.y1-packedchar.y0;

                    fprintf(fheader, "    { %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f },\n", c_w, c_h, u0, v0, u1, v1, packedchar.xoff, packedchar.yoff, packedchar.xadvance, packedchar.xoff2, packedchar.yoff2);
                }
                fprintf(fheader, "  } }, // %s\n", pack_entry.filename);
            }
            fprintf(fheader, "};\n");

        }


        fclose(fheader);


    } else {
        printf("Fail!\n");
    }
    return r != 0;
}

void t_list_images()
{
    int c = sb_count(image_entries);
    for (int i = 0; i < c; ++i)
    {
        pack_entry_t& pack_entry = image_entries[i];
        printf("%d: %s (%s) (%dx%d)\n", i, pack_entry.filename, pack_entry.filepath, pack_entry.width, pack_entry.height);
    }
}

void t_list_rects()
{
    int c = sb_count(image_entries);
    for (int i = 0; i < c; ++i)
    {
        stbrp_rect& image_rect = image_rects[i];
        pack_entry_t& pack_entry = image_entries[image_rect.id];
        printf("%d: %s [%d,%d] %s\n", image_rect.id, pack_entry.filename, image_rect.x, image_rect.y, image_rect.was_packed ? "" : "[NOT PACKED!]");
    }
}

int main(int argc, char *argv[])
{
    struct parg_state ps;
    int c;

    parg_init(&ps);

    const char* input_folder_path = 0x0;
    const char* ouput_file_path = 0x0;
    bool write_header = false;

    while ((c = parg_getopt(&ps, argc, argv, "h:s:i:vo:v?")) != -1) {
        switch (c) {
        case 1:
            printf("nonoption '%s'\n", ps.optarg);
            break;
        case '?':
show_help_and_exit:
            printf("Usage: qred_tp [-?] [-s size] -i <input-folder> [-o <output-image-file>] [-h <output-header-file>]\n");
            return EXIT_SUCCESS;
            break;
        case 'i':
            input_folder_path = ps.optarg;
            break;
        case 'o':
            ouput_file_path = ps.optarg;
            break;
        case 'h':
            ouput_file_path = ps.optarg;
            write_header = true;
            break;
        case 's':
            output_width = output_height = atoi(ps.optarg);
            break;
        default:
            printf("error: unhandled option -%c\n", c);
            return EXIT_FAILURE;
            break;
        }
    }

    if (!input_folder_path) {
        printf("Missing input folder path!\n");
        goto show_help_and_exit;
    }

    if (!ouput_file_path) {
        printf("Missing output file path!\n");
        goto show_help_and_exit;
    }

    printf("input_folder_path: %s\n", input_folder_path);
    printf("ouput_file_path: %s\n", ouput_file_path);

    if (!find_files(input_folder_path))
    {
        return EXIT_FAILURE;
    }

    t_list_images();

    create_rects();
    if (!pack_images()) {
        return EXIT_FAILURE;
    }

    t_list_rects();

    generate_atlas_image();

    if (write_header) {
        write_atlas_to_header(ouput_file_path);
    } else {
        write_atlas_image_to_disk(ouput_file_path);
        write_atlas_meta_to_disk("meta.txt");
    }


    return 0;
}
