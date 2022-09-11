/**
 * @file processing.h
 * @author Jayden Dumouchel
 * @date 9 Sep 2022
 *
 * @brief Image processing library
 *
 * Provides a set of functions to apply (post)processing of `struct image` on
 * the bytes.
 *
 * Kernel can be created/destroyed here to allow for robust usage of convolution.
 */

#ifndef _ED_PROCESSING_H
#define _ED_PROCESSING_H

#include <limits.h>

#include "common.h"
#include "image.h"

/**
 * A kernel stores the information needed to perform a convolution.
 *
 * Kernels can be created easily using kernel_create().
 */
struct kernel {
    int width;          /// The width of the matrix
    int height;         /// The height of the matrix
    float divisor;      /// All values are divided by this value after summing
    float *values[];    /// Stores the actual values of the kernel (as 2d array)
};

/** 
 * @brief Convolve an image using the passed kernel
 *
 * Convolution is applied per byte using as the sum of the kernel affected on its 
 * correspondent neighbours.
 *
 * The image must be appropriately padded and may only have a single channel
 *
 * @param img The image to convolve in place
 * @param k The kernel used for convolution
 */
int image_convolve(struct image *img, struct kernel *k);

/**
 * @brief Converts an rgb(a) image into a grayscale image.
 *
 * Grayscale conversion is performed using weighted values, not the average.
 * The alpha channel is left alone, all RGB channels will be equal. As such,
 * it is beneficial to perform `image_to_1channel()` to condense them.
 *
 * @param img The image to convert to grayscale (in place)
 */
int filter_grayscale(struct image *img);

/**
 * @brief Applies a Sobel edge detection filter to a (1 channel) image
 *
 * The filter is applied in 2 passes and the subsequent image is merged by addition.
 * Note: This sobel implementation requires 4x images to be loaded in memory simultaneously.
 *
 * @param img The image to apply the filter to (in place)
 * @param thinned A boolean to apply edge thinning (via maximum supression)
 */
int filter_sobel(struct image *img, int thinned);

/**
 * @brief Applies a Scharr edge detection filter to a (1 channel) image
 *
 * The filter is applied in 2 passes and the subsequent image is merged by addition.
 * Note: This implementation requires 4x images to be loaded in memory simultaneously.
 *
 * @param img The image to apply the filter to (in place)
 * @param thinned A boolean to apply edge thinning (via maximum supression)
 */
int filter_scharr(struct image *img, int thinned);

/**
 * @brief Applies the Roberts Cross edge detection kernels to a (1 channel) image
 *
 * @param img The image to apply to the filter to (in place)
 */
int filter_cross(struct image *img);

/**
 * @brief Applies a Laplacian of Guassian filter to an (1 channel) image.
 *
 * This is a 2-pass filter composed of a guassian blur (for denoising) and a laplace filter
 * for sensitive edge detection.
 *
 * @param img The image to apply the filter to
 * @param sigma The sigma value of the gaussian filter
 */
int filter_LoG(struct image *img, float sigma);

/**
 * @brief Applies a threshold function to an image, reducing all values to 0 or 255.
 *
 * @param img The image to threshold in place.
 * @param value The minimum byte value to allow.
 */
int filter_threshold(struct image *img, unsigned char value);

/**
 * @brief Applies a multi-pass threshold for better denoising.
 * Applies the threshold function twice, keeping the bytes that are discarded
 * by the higher `t1` but kept by `t2` *provided* that they connected to values left
 * undiscarded by `t1`.
 *
 * @param img The image to apply the filter to
 * @param t1 The larger threshold
 * @param t2 The smaller threshold
 */
int filter_hysteresis_threshold(struct image *img, unsigned char t1, unsigned char t2);

/**
 * @brief Applies a gaussian blur to an image.
 *
 * @param img The image to apply the blur.
 * @param size The size of the filter (size X size kernel)
 * @param sigma The blur strength
 */
int filter_gaussian(struct image *img, int size, float sigma);

/**
 * @brief Applies 2 separate convolutions and merges the result.
 *
 * @param img The image to convolve.
 * @param k1 The first kernel to apply.
 * @param k2 The second kernel to apply.
 * @param thinned A boolean to apply thinning
 */
int filter_two_pass(struct image *img, struct kernel *k1, struct kernel *k2, int thinned);

/**
 * @brief Applies the popular Canny edge detection operation.
 *
 * This is a multi-stage algorithm applied as:
 *      blur -> sobel -> edge thinning -> hysteresis threshold
 * As such, given my poor implementations, this is a fairly intensive process.
 *
 * @param img The image to apply to
 * @param sigma The weight of the gaussian blur
 * @param t1 The stricter (larger) threshold
 * @param t2 The softer (smaller) threshold
 */
int filter_canny(struct image *img, float sigma, unsigned char t1, unsigned char t2);

/**
 * @brief Allocates and populates a new kernel from a 2d array.
 *
 * Note, the kernel MUST be freed after use (preferrably via kernel_free()
 * @param w Width of the kernel
 * @param h Height of the kernel
 * @param div The divisor, every value is divided by this after summing
 * @param vals A 2D array to populate the kernel with
 */
struct kernel *kernel_create(int h, int w, float div, float vals[h][w]);

/**
 * @brief Frees a kernel and its allocated data.
 * @param k The kernel to free
 */
void kernel_free(struct kernel *k);

/**
 * @brief Builds a gaussian blur kernel
 *
 * Note: I am waaay out of my depth calculating this bullshit.
 * I naively implemented the commonly cited formula for 2D Gaussian Filters
 *      1/(2*pi*sigma^2) * e^-((x^2+y^2)/(2sigma^2))
 *
 * @param size The matrix dimensions (size x size )
 * @param weight The strength of the kernel (sigma)
 */
struct kernel *kernel_gaussian(int size, float weight);

#endif
