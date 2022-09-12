# edgedetect

A small linux utility to apply edge detection on images.

### Compilation

Ensure clang is installed (or modify the makefile to use a C compiler of choice. I do not believe I used compiler specific features.)

```bash
git clone https://github.com/RottenFishbone/edgedetect.git
cd edgedetect
make
```

### Usage

The CLI is pretty trash as it stands, however:

```bash
./edgedetect input_file output_file [--METHOD] [ARGS]
e.g.
./edgedetect dog.jpg dog_out.jpg --sobel 50
```

By default Canny edge detection is used.  
--sobel, --scharr, --log and --cross all have an optional threshold argument of range [0,255]  
--canny offers blur, threshold1, threshold2.  
--blur offers weight.  
 
