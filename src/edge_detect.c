#include "../include/edge_detect.h"

// Outputs information on how to use the program through a CLI
#define PRINT_USAGE() printf("usage: %s input_file output_file\n", PROGRAM_NAME)

static char PROGRAM_NAME[PATH_MAX+1] = {0};

int parse_long(char* str, long* out_int){
    int index_ptr;
    return (sscanf(str,"%ld%n", out_int, 
            &index_ptr) == 1 && (unsigned long) index_ptr == strlen(str));
}

int main(int argc, char **argv) {
    // copy the executed name into PROGRAM_NAME for usage printing
    strncpy(PROGRAM_NAME, argv[0], PATH_MAX);

    // Handle args
    char *input_path = NULL, *output_path = NULL;
    if (argc >= 3){
        input_path = argv[1]; 
        output_path = argv[2];
    }
    else {
        PRINT_USAGE(); 
        exit(EXIT_FAILURE);
    }
    

    // Load image from disk into memory
    struct image *in_img;
    if (!(in_img = image_load(input_path))){
        fprintf(stderr, 
                "%s\tFailed to load image from path: \n\t\t%s\n", 
                ERR_TXT, input_path);
        exit(EXIT_FAILURE);
    }
    printf("%s\tImage loaded:\n\t\twidth: %i\n\t\theight: %i\n\t\tchannels: %i\n",
            INFO_TXT, in_img->width, in_img->height, in_img->channels);
 


    // Convert to grayscale if needed
    struct image *img;
    if (in_img->channels > 1) { 
        printf("%s\tConverting to grayscale...\n", INFO_TXT);
        // Convert image to grayscale
        if (!filter_grayscale(in_img)){
            fprintf(stderr, "%s\tFailed to convert image to grayscale.\n", ERR_TXT);
            exit(EXIT_FAILURE);
        }
    
        // Strip all channels, leaving gray
        printf("%s\tStripping extra channels...\n", INFO_TXT);
        if (!(img = image_to_1channel(in_img))){
            fprintf(stderr, "%s\tFailed to allocate memory for image conversion.\n", ERR_TXT);
            exit(EXIT_FAILURE);
        }
        image_free(in_img);
    }
    else { img = in_img; }
    
    // TODO fix this abomination, argument handling should be done at the top
    //      and lump similar algos together. Flags sould be added for passing parameters
    if (argc == 3) { 
        edge_detect(img); 
    }
    else {
        operation op;
        if (!strncmp(argv[3], "--sobel", ARG_MAX)){ op = SOBEL; }
        else if (!strncmp(argv[3], "--log", ARG_MAX)){ op = LOG; }
        else if (!strncmp(argv[3], "--scharr", ARG_MAX)){ op = SCHARR; }
        else if (!strncmp(argv[3], "--blur", ARG_MAX)){ op = GAUSSIAN; }
        else if (!strncmp(argv[3], "--canny", ARG_MAX)){ op = CANNY; }
        else if (!strncmp(argv[3], "--cross", ARG_MAX)){ op = CROSS; }
        else {
            fprintf(stderr, "%s\tFailed to parse operation '%s'.\n", ERR_TXT, argv[3]);
            exit(EXIT_FAILURE);
        }

        if (op == SOBEL || op == LOG || op == SCHARR || op == CROSS){
            unsigned char thresh = 0;

            if (argc > 4) {
                long thresh_in;
                if (!parse_long(argv[4], &thresh_in)){
                    fprintf(stderr, "%s\tFailed to parse 'threshold' argument.", ERR_TXT);
                    exit(EXIT_FAILURE);
                }
                if (thresh_in > 255) { thresh_in = 255; }
                else if (thresh_in < 0) { thresh_in = 0; }
                thresh = (unsigned char) thresh_in;
            }

            switch (op){
                case SOBEL: edge_detect_sobel(img, thresh); break;
                case LOG: edge_detect_LoG(img, thresh); break;
                case SCHARR: edge_detect_scharr(img, thresh); break;
                case CROSS: edge_detect_cross(img, thresh); break;
                default: break;
            }
        }
        else if (op == GAUSSIAN){
            float sigma = 0.0;

            if (argc > 4) {
                char *p;
                sigma = strtof(argv[4], &p);
            }

            if (sigma < 0.0 || sigma > 100.0) { 
                fprintf(stderr, "%s\tFailed to parse 'weight' argument.", ERR_TXT);
                exit(EXIT_FAILURE);
            }
            gaussian_blur(img, sigma);

        }
        else if (op == CANNY){
            float sigma = 0.0;
            float t1 = 0.0;
            float t2 = 0.0;

            if (argc == 7){
                char *p;
                sigma = strtof(argv[4], &p);
                t1 = strtof(argv[5], &p);
                t2 = strtof(argv[6], &p);
            }
            else if (argc != 4){
                fprintf(stderr, "%s\tCanny requires 3 arguments (blur, thresh1, tresh2)\n", 
                        ERR_TXT);
                exit(EXIT_FAILURE);
            }

            edge_detect_canny(img, sigma, t1, t2);
        }

    }

    // Write image in memory to disk
    printf("%s\tWriting to file: \"%s\"...\n", INFO_TXT, output_path);
    if (!image_write_to_disk(img, output_path)){
        printf("failed.\n");
        fprintf(stderr, "%s\tCould not write image to disk.\n", ERR_TXT);
        exit(EXIT_FAILURE);
    }
    image_free(img);
    exit(EXIT_SUCCESS);
}

void edge_detect(struct image *img){
    edge_detect_canny(img, 1.0, 50, 20);
}

void edge_detect_sobel(struct image *img, unsigned char thresh){
    printf("%s\tApplying Sobel filter...\n", INFO_TXT);
    filter_sobel(img, 0);
    if (thresh){ 
        printf("%s\tApplying Threshhold of %u...\n", INFO_TXT, thresh);
        filter_threshold(img, thresh);
    }
}

void edge_detect_LoG(struct image *img, unsigned char thresh){
    printf("%s\tApplying LoG filter...\n", INFO_TXT);
    filter_LoG(img, 1.0);
    if (thresh){ 
        printf("%s\tApplying Threshhold of %u...\n", INFO_TXT, thresh);
        filter_threshold(img, thresh); 
    }
}

void edge_detect_scharr(struct image *img, unsigned char thresh){
    printf("%s\tApplying Scharr filter...\n", INFO_TXT);
    filter_scharr(img, 0);
    if (thresh){ 
        printf("%s\tApplying Threshhold of %u...\n", INFO_TXT, thresh);
        filter_threshold(img, thresh); 
    }
}

void gaussian_blur(struct image *img, float weight){
    printf("%s\tApplying gaussian blur...\n", INFO_TXT);
    filter_gaussian(img, 7, weight);
}


void edge_detect_canny(struct image *img, 
        float blur, 
        unsigned char thresh1, 
        unsigned char thresh2){
    
    printf("%s\tApplying Canny edge detection\n", INFO_TXT);
    filter_canny(img, blur, thresh1, thresh2);
}

void edge_detect_cross(struct image *img, unsigned char thresh){
    printf("%s\tApplying Roberts Cross filter...\n", INFO_TXT);
    filter_cross(img);
    if (thresh){ 
        printf("%s\tApplying Threshhold of %u...\n", INFO_TXT, thresh);
        filter_threshold(img, thresh); 
    }
}
