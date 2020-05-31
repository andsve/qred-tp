#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include "parg.h"

int main(int argc, char *argv[])
{
    struct parg_state ps;
    int c;

    parg_init(&ps);

    const char* input_folder_path = 0x0;
    const char* ouput_file_path = 0x0;

    while ((c = parg_getopt(&ps, argc, argv, "i:vo:vh")) != -1) {
        switch (c) {
        case 1:
            printf("nonoption '%s'\n", ps.optarg);
            break;
        case 'h':
show_help_and_exit:
            printf("Usage: qred_tp [-h] -i <input-folder> -o <output-file-path>\n");
            return EXIT_SUCCESS;
            break;
        case 'i':
            input_folder_path = ps.optarg;
            break;
        case 'o':
            ouput_file_path = ps.optarg;
            break;
        // case 'v':
        //     printf("testparg 1.0.0\n");
        //     return EXIT_SUCCESS;
        //     break;
        // case '?':
        //     if (ps.optopt == 's') {
        //         printf("option -s requires an argument\n");
        //     }
        //     else {
        //         printf("unknown option -%c\n", ps.optopt);
        //     }
        //     return EXIT_FAILURE;
        //     break;
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

    return 0;
}
