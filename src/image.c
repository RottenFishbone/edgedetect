#include "../include/image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

void image_free(struct image *img){ free(img->data); free(img); }

struct image *image_load(const char *path){
    struct image *img = malloc(sizeof(struct image));
    if (!img) { return NULL; }
    
    if (!(img->data=stbi_load(path, &img->width, &img->height, &img->channels, 0))){
        return NULL;
    }
    
    img->padding = 0;
    return img;
}


int image_write_to_disk(struct image *img, const char *path) {
    return stbi_write_png(path,
            img->width, img->height, img->channels, img->data,
            img->width * img->channels);
}


struct image *image_to_1channel(struct image *img){
    // TODO allow conversion of padded images
    if (img->padding) { return 0; }

    // Allocate space for the new one channel image
    struct image *img_out = malloc(sizeof(struct image));
    if (!img_out) { return 0; }
    img_out->data = malloc(img->width * img->height);
    if (!img_out->data) { return 0; }
    
    img_out->width = img->width;
    img_out->height = img->height;
    img_out->channels = 1;
    img_out->padding = 0;

    size_t img_size = (size_t) (img->width * img->height * img ->channels);
    for (size_t i = 0; i < img_size; i+=img->channels){
        img_out->data[i/img->channels] = img->data[i];
    }

    return img_out;
}

struct image *image_clone(struct image *img){
    struct image *new_img = malloc(sizeof(struct image));
    if (!new_img) { return 0; }
    new_img->width = img->width;
    new_img->height = img->height;
    new_img->channels = img->channels;
    new_img->padding = img->padding;
    new_img->data = malloc(img->width * img->height);
    if (!new_img->data) { return 0; }
    memcpy(new_img->data, img->data, img->width*img->height*img->channels);

    return new_img;
}

struct image *image_pad(struct image *img, int amount){
    // Sanity checking
    if (img->channels != 1) { return 0; }
    if (amount <= 0) { return 0; }
    
    // Allocated space for newly padded image
    struct image *padded = malloc(sizeof(struct image));
    if (!padded) { return 0; }

    padded->width = img->width+amount*2;
    padded->height = img->height+amount*2;
    padded->channels = img->channels;
    padded->padding = img->padding + amount;

    padded->data = calloc(padded->width * padded->height, 1);
    if (!padded->data) { return 0; }
    
    for (int y = 0; y < img->height; ++y){
        for (int x = 0; x < img->width; ++x) {
            int src_i = x + y * img->width;
            int dest_i = amount+x + (amount+y)*padded->width;
            padded->data[dest_i] = img->data[src_i];
        }
    }

    return padded;
}

struct image *image_unpad(struct image *img, int amount){
    //TODO make this a superset of unpad into to deduplicate.

    // Sanity checking
    if (img->padding < amount) { return 0;}
    if (img->channels != 1) { return 0; }
    if (amount <= 0) { return 0; }
    
    // Allocated space for newly padded image
    struct image *unpadded = malloc(sizeof(struct image));
    if (!unpadded) { return 0; }

    unpadded->width = img->width-amount*2;
    unpadded->height = img->height-amount*2;
    unpadded->channels = img->channels;
    unpadded->padding = img->padding - amount;

    unpadded->data = malloc(unpadded->width * unpadded->height);
    if (!unpadded->data) { return 0; }
    
    for (int y = 0; y < unpadded->height; ++y){
        for (int x = 0; x < unpadded->width; ++x) {
            int dest_i = x + y * unpadded->width;
            int src_i = amount+x + (amount+y)*img->width;
            unpadded->data[dest_i] = img->data[src_i];
        }
    }

    return unpadded;
}


int image_unpad_into(struct image *dest, struct image *src){
    // Sanity checking
    if (src->padding < dest->padding) { return 0;}
    if (src->channels != 1 || dest->channels != src->channels) { return 0; }
    
    // Calculate and sanity check the amount of unpadding
    int amt_x = src->width - dest->width;
    int amt_y = src->height - dest->height;
    if (amt_x != amt_y) { return 0; }
    int amount = amt_x/2;

    for (int y = 0; y < dest->height; ++y){
        for (int x = 0; x < dest->width; ++x) {
            int dest_i = x + y * dest->width;
            int src_i = amount+x + (amount+y) * src->width;
            dest->data[dest_i] = src->data[src_i];
        }
    }

    return 1;
}


int image_merge_add(struct image *img_a, struct image *img_b){
    // Ensure images are compatible for merging
    if (img_a->width != img_b->width ||
        img_a->height != img_b->height ||
        img_a->channels != img_b->channels ||
        img_a->padding != img_b->padding) {
        fprintf(stderr, "%s\tImage dimension mismatch. \n\t\tAborting merge.\n", WARN_TXT);
        return 0;
    }
    
    // Add bytes between two images, clamping to 255
    for (int i = 0; i < img_a->height * img_a->width; ++i){
        unsigned int a = (unsigned int) img_a->data[i];
        unsigned int b = (unsigned int) img_b->data[i];
        img_a->data[i] = MIN(a + b, 255);
    }
    return 1;
}
