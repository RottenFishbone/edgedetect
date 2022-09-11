#include "../include/processing.h"

// A gloriously inefficient way to test if an index is not in the padding of an image
int inner_image_contains(struct image *img, size_t i){
    size_t w = (size_t)(img->width);  
    size_t p = (size_t)(img->padding);

    size_t inner_w = (size_t)(img->width - img->padding*2);
    size_t inner_h = (size_t)(img->height - img->padding*2);

    size_t x = i%w, y = i/w;
    size_t tl = w*p + p;
    size_t tl_x = tl%w, tl_y = tl/w;
    
    return (tl_x <= x && tl_x+inner_w >= x && tl_y <= y && tl_y+inner_h >= y);
}

int image_convolve(struct image *img, struct kernel *k){
    // Calculate required padding
    int half_w = k->width/2, half_h = k->height/2;
    int req_padding = MAX(half_w, half_h);
    if (img->padding < req_padding) { 
        fprintf(stderr, "%s\tNot enough padding for convolution. \n\t\tAborting convolution.",
                WARN_TXT);
        return 0; 
    }
    // TODO add striding
    if (img->channels != 1) { 
        fprintf(stderr, "%s\tToo many channels for convolution. \n\t\tAborting convolution.",
                WARN_TXT);
        return 0; 
    }

    // Allocate memory for the convolution result
    struct image *tmp_img = image_clone(img);
    if (!tmp_img) { 
        fprintf(stderr, "%s\tFailed to clone image. \n\t\tAborting convolution.",
                WARN_TXT);
        return 0;
    }
        
    // Apply the kernel matrix on each byte of the image
    size_t image_size = img->width * img->height;
    size_t top_left = img->width * img->padding + img->padding;
    size_t bottom_right = image_size - top_left;
    for (size_t i = top_left; i < bottom_right; ++i){
        if (!inner_image_contains(img, i)){continue;}

        // Create a value to store the result of the selected cell
        float cell = 0.0;

        // Apply each value from the kernel to the respective offset in the image
        for (int ky = 0; ky < k->height; ++ky){
            for (int kx = 0; kx < k->width; ++kx){
                float k_val = k->values[ky][kx];
                int offset = (kx - half_w) + (ky - half_h) * img->width;
                // Sum the product of the kernel and the offset cell
                cell += k_val * (float)(img->data[offset+i]);
            }
        }
        
        // Apply the divisor after all summing is complete
        cell /= k->divisor;
        // Clamp the cell's result to a byte
        cell = roundf(cell);
        if (cell > 255.0) { cell = 255.0; }
        else if (cell < 0.0) { cell = 0.0; }
        // Store into a new image for final result
        tmp_img->data[i] = (unsigned char) cell;
    }
    // Move tmp_img's data into the passed image
    memmove(img->data, tmp_img->data, img->width*img->height*img->channels);
    image_free(tmp_img);
    return 1;
}

int filter_grayscale(struct image *img){
    //TODO allow processing of padded images
    if (img->padding) { return 0; }

    // Return failure if one of the image parameters is 0
    if (!img->channels || !img->width || !img->height) { return 0; }
    // Only supporting 3 and 4 channel rgb(a) images 
    // TODO allow arbitrary channels/schemes?
    if (img->channels != 3 && img->channels != 4) { return 0; }

    size_t img_size = (size_t) (img->width * img->height * img->channels);
    // Note: infinite loop if img->channels == 0
    for (size_t i = 0; i < img_size; i+=img->channels){
        // Map the color channels to pointer references
        unsigned char *r = &img->data[i];
        unsigned char *g = &img->data[i+1];
        unsigned char *b = &img->data[i+2];

        // Create floats of each color channel
        float dec_r = (float)(*r), dec_g = (float)(*g), dec_b = (float)(*b);
        // Calculate and clamp the grayscale using weighted values
        float grayscale = 0.299*dec_r + 0.587*dec_g + 0.144*dec_b;
        if (grayscale > 255.0) { grayscale = 255.0; }
        else if (grayscale < 0.0) { grayscale = 0.0; }
        
        // Convert back to a byte
        unsigned char gray_byte = (unsigned char) (roundf(grayscale));
        *r = gray_byte; *b = gray_byte; *g = gray_byte;
    }

    return 1;
}

struct kernel *kernel_create(int h, int w, float div, float vals[h][w]){
    // Allocate space for the struct and the dynamic array of pointers
    struct kernel *k = malloc(sizeof(struct kernel) + sizeof(float*[h]));
    if (!k) { return 0; }
    
    // Copy the vals into the dynamically allocated memory
    k->width = w; k->height = h; k->divisor = div;
    for (int y = 0; y < h; ++y){
        if (!(k->values[y] = malloc(sizeof(float[w])))) { return 0; }
        for (int x = 0; x < w; ++x){
            k->values[y][x] = vals[y][x];
        }
    }
    return k;
}

void kernel_free(struct kernel *k){
    // Deep free the values
    for (int i = 0; i < k->height; ++i){ free(k->values[i]); }
    // Free the struct
    free(k);
}


int filter_LoG(struct image *img, float sigma){
    if (!img->height || !img->width) { return 0; }
    if (img->channels != 1) { return 0; }
    
    static float k_vals[3][3] = {
        {  0, -1,  0 },
        { -1,  4, -1 },
        {  0, -1,  0 }
    };

    struct kernel *gauss_k = kernel_gaussian(5, sigma);
    if (!gauss_k) { return 0; }
    
    struct kernel *lap_k = kernel_create(3, 3, 1.0, k_vals);
    if (!lap_k) { return 0; }

    // Pad the image, if needed
    struct image *padded_img = img;
    int req_padding = MAX(
            MAX(lap_k->width/2, gauss_k->width/2),
            MAX(lap_k->height/2, gauss_k->height/2));
    if (img->padding < req_padding) {
        if (!(padded_img = image_pad(img, req_padding))) { 
            fprintf(stderr, "\n%s\tFailed to pad image for convolution\n", WARN_TXT);
            return 0;
        }
    }

    // Apply blur to denoise
    if (!image_convolve(padded_img, gauss_k)){ 
        fprintf(stderr, "\n%s\tFailed gaussian filter convolution. \n\t\tAborting LoG.\n", 
                WARN_TXT); 
        return 0;
    }
    kernel_free(gauss_k);
    
    // Apply laplace filter for edge detection
    if (!image_convolve(padded_img, lap_k)){ 
        fprintf(stderr, "\n%s\tFailed laplacian filter convolution. \n\t\tAborting LoG.\n", 
                WARN_TXT); 
        return 0;
    }
    kernel_free(lap_k);
    
    // Unpad and cleanup, if needed
    if (img != padded_img){
        image_unpad_into(img, padded_img);
        image_free(padded_img);
    }

    return 1;
}


int filter_scharr(struct image *img, int thinned){
    static float kx_vals[3][3] = {
        { 47,  0, -47 },
        { 162, 0, -162 },
        { 47,  0, -47 }
    };
    static float ky_vals[3][3] = {
        {  47,  162,  47 },
        {  0,  0,  0 },
        { -47, -162, -47 }
    };

    struct kernel *kx = kernel_create(3, 3, 80.0, kx_vals);
    struct kernel *ky = kernel_create(3, 3, 80.0, ky_vals);
    if (!kx || !ky) { return 0; }

    filter_two_pass(img, kx, ky, thinned);

    kernel_free(kx); 
    kernel_free(ky);
    return 1;
}


int filter_sobel(struct image *img, int thinned){
    static float kx_vals[3][3] = {
        { 1, 0, -1 },
        { 2, 0, -2 },
        { 1, 0, -1 }
    };
    static float ky_vals[3][3] = {
        {  1,  2,  1 },
        {  0,  0,  0 },
        { -1, -2, -1 }
    };

    // Sanity checks
    if (img->channels != 1) { return 0; }
    if (!img->width || !img->height) { return 0; }
    
    struct kernel *kx = kernel_create(3, 3, 4.0, kx_vals);
    struct kernel *ky = kernel_create(3, 3, 4.0, ky_vals);
    if (!kx || !ky) { return 0; }

    filter_two_pass(img, kx, ky, thinned);

    kernel_free(kx); 
    kernel_free(ky);
    return 1;
}


int filter_two_pass(struct image *img, struct kernel *k1, struct kernel *k2, int thinned){
    //TODO allow for passing gradient formula/functions. As is stands, sobel's is implemented

    // Sanity checks
    if (img->channels != 1) { return 0; }
    if (!img->width || !img->height) { return 0; }

    // Pad the image, if needed
    struct image *padded_img_x = img;
    int req_padding = MAX(
            MAX(k1->width/2, k2->width/2),
            MAX(k1->height/2, k2->height/2));
    if (img->padding < req_padding) {
        if (!(padded_img_x = image_pad(img, req_padding))) { 
            fprintf(stderr, "\n%s\tFailed to pad image for convolution\n", WARN_TXT);
            return 0;
        }
    }

    // Clone the padded image for a separate pass
    struct image *padded_img_y;
    if (!(padded_img_y=image_clone(padded_img_x))){
        fprintf(stderr, "\n%s\tFailed to clone image for convolution\n", WARN_TXT);
        return 0;
    }

    // Convolve the image with first kernel
    if (!image_convolve(padded_img_y, k1)){ 
        fprintf(stderr, "\n%s\tFailed first convolution. \n\t\tAborting\n", 
                WARN_TXT); 
        return 0;
    }

    if (!image_convolve(padded_img_x, k2)){ 
        fprintf(stderr, "\n%s\tFailed second convolution. \n\t\tAborting.\n", 
                WARN_TXT); 
        return 0;
    }

    if (thinned){
        enum dir { VERT, HORIZ, DIAG_FORW, DIAG_BACK };
    
        int img_size = padded_img_x->width * padded_img_x->height;
        enum dir dirs[img_size];
        for (int i = 0; i < img_size; ++i){
            float gy = (float)padded_img_y->data[i];
            float gx = (float)padded_img_x->data[i];

            float angle = atan2(gy, gx);
            if ((angle <= 0.f && angle > -1.f * M_PI / 8.f) || 
                (angle > 0.f && angle <= 1.f * M_PI / 8.f ) || 
                (angle <= 1.f * M_PI && angle > 7.f * M_PI / 8.f) ||
                (angle > -1.f * M_PI && angle <= -7.f * M_PI / 8.f)){

                dirs[i] = HORIZ;
            }
            else if ((angle > 1.f * M_PI / 8.f && angle <= 3.f * M_PI / 8.f) ||
                     (angle <= -1.f * M_PI / 8.f && angle > -3.f * M_PI / 8.f)){
                dirs[i] = DIAG_FORW;
            }
            else if ((angle > 3.f * M_PI / 8.f && angle <= 5.f * M_PI / 8.f) ||
                     (angle <= -3.f * M_PI / 8.f && angle > -5.f * M_PI / 8.f)){
                dirs[i] = VERT;
            }
            // TODO fix issue with DIAG_BACK angle detection?
            else if ((angle > 5.f * M_PI / 8.f && angle <= 7.f * M_PI / 8.f) ||
                     (angle <= -5.f * M_PI / 8.f && angle > -7.f * M_PI / 8.f)){
                dirs[i] = DIAG_BACK;
            }
        }
 
        image_merge_add(padded_img_x, padded_img_y);
        image_free(padded_img_y);

        for (int i = 0; i < img_size; ++i){
            unsigned char *cell, *a, *b;
            cell = &padded_img_x->data[i];
            int width = padded_img_x->width;
            switch (dirs[i]){
                case HORIZ:
                    a = &padded_img_x->data[i-width];
                    b = &padded_img_x->data[i+width];
                    break;
                case VERT:
                    a = &padded_img_x->data[i-1];
                    b = &padded_img_x->data[i+1];
                    break;
                case DIAG_FORW:
                    a = &padded_img_x->data[i+1-width];
                    b = &padded_img_x->data[i-1+width];
                    break;
                case DIAG_BACK:
                    a = &padded_img_x->data[i-1-width];
                    b = &padded_img_x->data[i+1+width];
                    break;
                default: break;
            }

            if (*cell < *a || *cell < *b) { *cell = 0;}
        }
 
        if (img != padded_img_x){
            image_unpad_into(img, padded_img_x);
            image_free(padded_img_x);
        }
    }
    else {
        image_merge_add(padded_img_x, padded_img_y);
        image_free(padded_img_y);
        
        if (img != padded_img_x){
            image_unpad_into(img, padded_img_x);
            image_free(padded_img_x);
        }
    }

    return 1;
}


struct kernel *kernel_gaussian(int size, float weight){
    if (size < 3 || weight <= 0.0) { return 0; }
    
    int offset = size/2;
    float s = weight*weight*2.0;

    float k[size][size];
    for (int arr_y = 0; arr_y < size; ++arr_y){
        for (int arr_x = 0; arr_x < size; ++arr_x){
            int x = arr_x - offset;
            int y = arr_y - offset;

            k[arr_y][arr_x] = 1.0 / (s * M_PI) * powf(M_E, -((x*x+y*y)/s)); 
        }
    }
    
    return kernel_create(size, size, 1.0, k);
}


int filter_gaussian(struct image *img, int size, float sigma){
    struct kernel *k = kernel_gaussian(size, sigma);
    if (!k) { return 0; }

    // Ensure image is padded
    int req_padding = MAX(k->width/2, k->height/2);
    struct image *padded_img = img;
    if (img->padding < req_padding) {
        if (!(padded_img = image_pad(img, req_padding))) { 
            fprintf(stderr, "\n%s\tFailed to pad image for convolution\n", WARN_TXT);
            return 0;
        }
    }

    // Convolve the image with gaussian filter
    if (!image_convolve(padded_img, k)){ 
        fprintf(stderr, "\n%s\tFailed convolution. \n\t\tAborting\n", 
                WARN_TXT); 
        return 0;
    }
    kernel_free(k);

    // Free padded_img, if one was made
    if (img != padded_img){
        image_unpad_into(img, padded_img);
        image_free(padded_img);
    }

    return 1;
}


int filter_threshold(struct image *img, unsigned char value){
    if (!img->width || !img->height) { return 0; }
    if (img->channels != 1) { return 0; }

    for (size_t i = 0; i < (size_t)(img->height*img->width); ++i){
        img->data[i] = img->data[i] < value ? 0 : 255;
    }

    return 1;
}


int filter_hysteresis_threshold(struct image *img, unsigned char t1, unsigned char t2){
    // Sanity checks
    if (t1 <= t2) { return 0; }
    if (!img->width || !img->height) { return 0; }
    if (img->channels != 1) { return 0; }
    
    struct image *padded_img = img;
    if (!img->padding){
        if (!image_pad(padded_img, 1)){
            fprintf(stderr, "%s\tFailed to pad image.\n\tAborting.\n", WARN_TXT);
            return 0;
        }
    }

    // Apply two separate thresholds and save them separately
    size_t image_size = (size_t) (padded_img->width) * (size_t)(padded_img->height);
    unsigned char t1_data[image_size], t2_data[image_size];
    for (size_t i = 0; i < image_size; ++i){
        t1_data[i] = padded_img->data[i] < t1 ? 0 : 255;
        t2_data[i] = padded_img->data[i] < t2 ? 0 : 255;
    }
    // find the rectangle of the inner image (without padding)
    size_t top_left = padded_img->width * padded_img->padding + padded_img->padding;
    size_t bottom_right = image_size - top_left;
    
    const long moore_offsets[8] = {
        -padded_img->width-1, -padded_img->width, -padded_img->width+1,
                   -1,                         +1,
         padded_img->width-1,  padded_img->width,  padded_img->width+1
    };
    
    // Apply edge rebuilding
    printf("%s\tStarting hysteresis threshold...\n", INFO_TXT);
    size_t changed_pixels;
    size_t total_changed = 0;
    size_t step = 0;
    do {
        changed_pixels = 0;
        for (size_t i = top_left; i < bottom_right; i++){
            if (t1_data[i] || !t2_data[i]) { continue; }
            if (!inner_image_contains(padded_img, i)) { continue; }
            
            for (size_t off_i = 0; off_i < 8; ++off_i){
                size_t nbr_i = (size_t)((long) i + moore_offsets[off_i]);
                if (t1_data[nbr_i]) {
                    t1_data[i] = 255;
                    changed_pixels++;
                    break;
                }
            }
        }
        step++;
        total_changed += changed_pixels;
    } while (changed_pixels);
    printf("\t\tRecovered %lu pixels in %lu steps.\n", total_changed, step);
    
    memcpy(padded_img->data, t1_data, image_size);

    // Free padded_img, if one was made
    if (img != padded_img){
        image_unpad_into(img, padded_img);
        image_free(padded_img);
    }

    return 1;
}


int filter_cross(struct image *img){
    static float kx_vals[2][2] = {
        {  1,  0 },
        {  0, -1 },
    };
    static float ky_vals[2][2] = {
        {  0,  1 },
        { -1,  0 },
    };

    // Sanity checks
    if (img->channels != 1) { return 0; }
    if (!img->width || !img->height) { return 0; }
    
    struct kernel *kx = kernel_create(2, 2, 1.0, kx_vals);
    struct kernel *ky = kernel_create(2, 2, 1.0, ky_vals);
    if (!kx || !ky) { return 0; }

    filter_two_pass(img, kx, ky, 0);

    kernel_free(kx); 
    kernel_free(ky);
    return 1;
}


int filter_canny(struct image *img, float sigma, unsigned char t1, unsigned char t2){
    if (!img->width || !img->height) { return 0; }
    if (img->channels != 1) { return 0; }
    if (sigma < 0.0) { return 0; }

    filter_gaussian(img, 5, sigma);
    filter_sobel(img, 1);
    filter_hysteresis_threshold(img, t1, t2);

    return 1;
}
