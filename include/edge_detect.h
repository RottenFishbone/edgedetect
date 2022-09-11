/**
 * @file edgedetect.h
 * @author Jayden Dumouchel
 * @date 9 Sep 2022
 *
 * @brief CLI based program providing unoptimized edge detection methods
 */

#ifndef _EDGE_DETECT_H
#define _EDGE_DETECT_H

#include <stdlib.h>
#include <stdio.h>
#include <linux/limits.h>

#include "common.h"
#include "image.h"
#include "processing.h"

typedef enum operation {
    SOBEL,
    LOG,
    SCHARR,
    CANNY,
    GAUSSIAN,
    CROSS,
} operation;

void edge_detect(struct image *img);
void edge_detect_sobel(struct image *img, unsigned char thresh);
void edge_detect_LoG(struct image *img, unsigned char thresh);
void edge_detect_scharr(struct image *img, unsigned char thresh);
void gaussian_blur(struct image *img, float weight);
void edge_detect_cross(struct image *img, unsigned char thresh);
void edge_detect_canny(struct image *img, 
                       float blur, 
                       unsigned char thresh1,
                       unsigned char thresh2);
#endif
