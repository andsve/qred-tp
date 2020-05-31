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

#include <stretchy_buffer.h>

#include <xxhash.h>

#include "parg.h"

// #include "../asd.h"

struct image_entry_t
{
    char filepath[4096];
    char filename[4096];
    int width;
    int height;
    int channels;
    uint8_t* data;
};

static int output_width = 512;
static int output_height = 512;
static uint8_t* output_data = NULL;
static uint8_t* output_data_compressed = NULL;
static int output_data_compressed_size = 0;

static image_entry_t* image_entries = NULL;
static stbrp_rect* image_rects = NULL;

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
            // new image entry
            image_entry_t image_entry;
            strcpy(image_entry.filepath, filename_qfd);
            strcpy(image_entry.filename, dp->d_name);

            // load image
            int n;
            image_entry.data = stbi_load(filename_qfd, &image_entry.width, &image_entry.height, &n, 4);

            // push it to the list of images
            sb_push(image_entries, image_entry);
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

void blit_image_rect(stbrp_rect& image_rect, image_entry_t& image_entry)
{
    uint64_t p = (image_rect.y * output_width) + image_rect.x;

    for (int y = 0; y < image_rect.h; ++y)
    {
        for (int x = 0; x < image_rect.w; ++x)
        {
            uint64_t i = y*image_rect.w+x;
            output_data[p*4+0] = image_entry.data[i*4+0];
            output_data[p*4+1] = image_entry.data[i*4+1];
            output_data[p*4+2] = image_entry.data[i*4+2];
            output_data[p*4+3] = image_entry.data[i*4+3];
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
        image_entry_t& image_entry = image_entries[image_rect.id];
        blit_image_rect(image_rect, image_entry);
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
        image_entry_t& image_entry = image_entries[image_rect.id];
        // printf("%d: %s [%d,%d] %s\n", image_rect.id, image_entry.filename, image_rect.x, image_rect.y, image_rect.was_packed ? "" : "[NOT PACKED!]");
        float u0 = (float)image_rect.x / ow;
        float v0 = (float)(output_height - image_rect.y) / oh;
        float u1 = (float)(image_rect.x + image_rect.w) / ow;
        float v1 = (float)(output_height - image_rect.y - image_rect.h) / oh;
        fprintf(fmeta, "%s: %f %f %f %f\n", image_entry.filename, u0, v0, u1, v1);
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
        fprintf(fheader, "const char atlas_image_data[] = {\n");
        for (int i = 0; i < output_data_compressed_size; ++i)
        {
            fprintf(fheader, "0x%02x%s%s", output_data_compressed[i], i != output_data_compressed_size-1 ? "," : "", i % 16 == 15 ? "\n" : " ");
        }
        fprintf(fheader, "};\n");

        // write meta
        fprintf(fheader, "struct atlas_entry_t {\n");
        // fprintf(fheader, "    const char* filename;\n");
        // fprintf(fheader, "    XXH64_hash_t filename;\n");
        fprintf(fheader, "    XXH32_hash_t filename;\n");
        fprintf(fheader, "    float u0;\n");
        fprintf(fheader, "    float v0;\n");
        fprintf(fheader, "    float u1;\n");
        fprintf(fheader, "    float v1;\n");
        fprintf(fheader, "};\n");
        fprintf(fheader, "atlas_entry_t atlas_entries[] = {\n");

        float ow = (float)output_width;
        float oh = (float)output_height;

        int c = sb_count(image_entries);
        for (int i = 0; i < c; ++i)
        {
            stbrp_rect& image_rect = image_rects[i];
            image_entry_t& image_entry = image_entries[image_rect.id];
            float u0 = (float)image_rect.x / ow;
            float v0 = (float)(image_rect.y) / oh;
            float u1 = (float)(image_rect.x + image_rect.w) / ow;
            float v1 = (float)(image_rect.y + image_rect.h) / oh;
            // fprintf(fheader, "  { \"%s\", %f, %f, %f, %f },\n", image_entry.filename, u0, v0, u1, v1);
            fprintf(fheader, "  { 0x%x, %f, %f, %f, %f }, // %s\n", XXH32str(image_entry.filename), u0, v0, u1, v1, image_entry.filename);
        }
        fprintf(fheader, "  { 0x0, 0.0f, 0.0f, 1.0f, 1.0f }, // full image\n");

        fprintf(fheader, "};\n");

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
        image_entry_t& image_entry = image_entries[i];
        printf("%d: %s (%s) (%dx%d)\n", i, image_entry.filename, image_entry.filepath, image_entry.width, image_entry.height);
    }
}

void t_list_rects()
{
    int c = sb_count(image_entries);
    for (int i = 0; i < c; ++i)
    {
        stbrp_rect& image_rect = image_rects[i];
        image_entry_t& image_entry = image_entries[image_rect.id];
        printf("%d: %s [%d,%d] %s\n", image_rect.id, image_entry.filename, image_rect.x, image_rect.y, image_rect.was_packed ? "" : "[NOT PACKED!]");
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
