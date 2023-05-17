# Disclaimer

This is a toy for basic image manipulation and is *definitely* not production quality. That said, it might provide some useful techniques/insights and *is* fully functional.

# edgedetect

A small linux utility to apply edge detection on images.

Edge detection is provided through CPU-executed convolution kernels on grayscale images. Thus,
the output will always be grayscale (using the [luminosity method](https://mmuratarat.github.io/2020-05-13/rgb_to_grayscale_formulas)).

### Compilation

Ensure clang is installed (or modify the makefile to use a C compiler of choice. I do not believe I used compiler specific features.)

```bash
git clone https://github.com/RottenFishbone/edgedetect.git
cd edgedetect
make
```

### Usage

The CLI is pretty fragile and limited as it stands, however:

```bash
./edgedetect input_file output_file [--METHOD] [ARGS]
e.g.
./edgedetect dog.jpg dog_out.jpg --sobel 50
```

### Methods
All methods are implemented completely from scratch and may serve as a very clear reference as to how each method works due to the simple structure of the project.
All filters can be found in `src/processing.c`.

#### Basic Filters
Each of these take a threshold argument that applies a threshold from 0-255 on the output bytes.
 - `--sobel <threshold>` Applies the [Sobel](https://en.wikipedia.org/wiki/Sobel_operator) Operator
 - `--cross <threshold>` Applies the [Roberts Cross](https://en.wikipedia.org/wiki/Roberts_cross) Operator
 - `--scharr <threshold>` Applies the [Scharr](https://en.wikipedia.org/wiki/Sobel_operator#Alternative_operators) Operator
 
#### Compound Filter 
These filters are comprised of multiple passes, and apply a gaussian blur kernel.
- `--log <weight>` [Laplacian of Gaussian](https://en.wikipedia.org/wiki/Blob_detection#The_Laplacian_of_Gaussian), applies a Gaussian blur of `weight` and then applies a Laplacian operator. This provides a quality edge detection, albeit sensitive to noise.
- `--canny <weight threshold1 threshold2>` [Canny edge detection](https://en.wikipedia.org/wiki/Canny_edge_detector) applies blur of `weight`, then a sobel filter, followed by a hysterisis threshold. This thresholds the image twice and rebuilds lines lost by the first threshold using lines found in the second threshold. This is accomplished in an indeterminant number of passes, as lines will be rebuilt if in the lower threshold they exist as a moore neighbour to an existing pixel in the stricter threshold.

#### Other
`--blur <weight>` Applies a 5x5 Guassian blur kernel. The kernel is dynamically generated using the [mathematical definition](https://en.wikipedia.org/wiki/Gaussian_filter).

### Examples
|  Method    | Command   | Example   |
|:----------:|:---------:|:---------:|
|(baseline)  | `N/A`     |<img src="https://github.com/RottenFishbone/edgedetect/assets/2926677/66f6e611-5495-464a-ba2f-864871b5110f" width=50% height=50%>|
| Sobel  | `--sobel 40` |<img src="https://github.com/RottenFishbone/edgedetect/assets/2926677/5035cdd2-2e11-4552-923b-abc342df0de8" width=50% height=50%>|
| Cross | `--cross 40` |<img src="https://github.com/RottenFishbone/edgedetect/assets/2926677/36a608ee-afcd-44bd-b29d-de1d6201a5d8" width=50% height=50%>|
| Scharr| `--scharr 75` | <img src="https://github.com/RottenFishbone/edgedetect/assets/2926677/b38b5c08-f31f-40a8-9460-f9e5accf42ab" width=50% height=50%>|
| Laplacian of Gaussian | `--log 6.0` |<img src="https://github.com/RottenFishbone/edgedetect/assets/2926677/4be1053d-483e-4f94-9cf4-059913471dfb" width=50% height=50%>|
| Canny| `--canny 1.0 20 10` | <img src="https://github.com/RottenFishbone/edgedetect/assets/2926677/5ce060fb-a397-4597-84c6-b77645712a67" width=50% height=50%>|

