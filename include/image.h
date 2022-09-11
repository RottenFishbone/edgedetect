/**
 * @file image.h
 * @author Jayden Dumouchel
 * @date 9 Sep 2022
 *
 * @brief Image loading/manipulation
 *
 * Provides a common definition of an image within program memory.
 * Furthermore, provides functions that provide basic image manipulation and
 * handling.
 * All image loading/writing is provided through the fabled public domain library `stb_image`.
 *
 * @see https://github.com/nothings/stb
 */

#ifndef _ED_IMAGE_H
#define _ED_IMAGE_H

#include "common.h"

struct image {
    int width;
    int height;
    int channels;
    int padding;
    unsigned char *data;
};

int image_write_to_disk(struct image *img, const char *path);

void image_free(struct image *img);

struct image *image_load(const char *path);

struct image *image_clone(struct image *img);

struct image *image_to_1channel(struct image *img);

/**
 * @brief Generates a padded version of an image.
 *
 * A new image is generated after padding (as it has an increased footprint) using
 * black as a pad color. This is primarily to allow for convolution to run unimpeded.
 *
 * @param img The image to pad.
 * @param amount The amount of padding to add *to each side*.
 */
struct image *image_pad(struct image *img, int amount);

/**
 * @brief Generates an unpadded version of an image.
 *
 * Unpadding can only unpad an amount <= the current padding of the image.
 *
 * @param img The image to unpad
 * @param amount The amount of padding to remove.
 */
struct image *image_unpad(struct image *img, int amount);

/**
 * @brief Applies unpadding into `dest` images memory.
 *
 * Attempts to detect the padding amount from dest to src. This allows
 * for undoing padding without cloning several times.
 * This *should* only be used to unpad back into the source image after 
 * image data manipulation.
 * @param dest The image to load unpadded data into.
 * @param src The padded image to load from.
 */
int image_unpad_into(struct image *dest, struct image *src);

/**
 * @brief Merges img_b into img_a by adding the values of the bytes.
 *
 * A merge is performed in place on img_a taking the min(a+b, 255) of each byte.
 *
 * @param img_a The image to merge into (and with)
 * @param img_b The image to merge from (and with)
 */
int image_merge_add(struct image *img_a, struct image *img_b);

#endif
