/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/*
 * Please fill in the following team struct
 */
team_t team = {
    "BlackScalie",              /* Team name */

    "Cosidian",     /* First member full name */
    "obsidiandragon2016@gmail.com",  /* First member email address */

    "",                   /* Second member full name (leave blank if none) */
    ""                    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/*
 * naive_rotate - The naive baseline version of rotate
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst)
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/*
 * code_motion_rotate - Try to use code motion to optimize
 */
char code_motion_rotate_descr[] = "code_motion_rotate: Try to use code motion to optimize";
void code_motion_rotate(int dim, pixel * src, pixel * dst) {
    int i, j;
    int tmp0, tmp1;
    int cst0 = dim * (dim - 1);
    for (i = 0 ; i < dim ; i++) {
        tmp0 = i;
        tmp1 = i * dim;
        for (j = 0 ; j < dim ; j++) {
            dst[cst0 - j * dim + tmp0] = src[tmp1 + j];
        }
    }
}

/*
 * block_rotate - Try to use block to optimize
 */
char block_rotate_descr[] = "block_rotate: Try to use block to optimize";
void block_rotate(int dim, pixel * src, pixel * dst) {
    int i, j, k, l;
    int cst0 = dim - 1;
    for (i = 0 ; i < dim ; i += 8) {
        for (j = 0 ; j < dim ; j += 8) {
            for (k = i ; k < i + 8 ; k++) {
                for (l = j ; l < j + 8 ; l++) {
                    dst[RIDX(cst0 - l, k, dim)] = src[RIDX(k, l, dim)];
                }
            }
        }
    }
}

/*
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst)
{
    block_rotate(dim, src, dst);
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_rotate_functions()
{
    add_rotate_function(&naive_rotate, naive_rotate_descr);
    add_rotate_function(&rotate, rotate_descr);
    /* ... Register additional test functions here */
    add_rotate_function(&code_motion_rotate, code_motion_rotate_descr);
    add_rotate_function(&block_rotate, block_rotate_descr);
}


/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
    int red;
    int green;
    int blue;
    int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/*
 * initialize_pixel_sum - Initializes all fields of sum to 0
 */
static void initialize_pixel_sum(pixel_sum *sum)
{
    sum->red = sum->green = sum->blue = 0;
    sum->num = 0;
    return;
}

/*
 * accumulate_sum - Accumulates field values of p in corresponding
 * fields of sum
 */
static void accumulate_sum(pixel_sum *sum, pixel p)
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->num++;
    return;
}

/*
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum)
{
    current_pixel->red = (unsigned short) (sum.red/sum.num);
    current_pixel->green = (unsigned short) (sum.green/sum.num);
    current_pixel->blue = (unsigned short) (sum.blue/sum.num);
    return;
}

/*
 * avg - Returns averaged pixel value at (i,j)
 */
static pixel avg(int dim, int i, int j, pixel *src)
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min(i+1, dim-1); ii++)
	for(jj = max(j-1, 0); jj <= min(j+1, dim-1); jj++)
	    accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst)
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}

/*
 * optimized_smooth - The optimized version of smooth
 */
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
char optimized_smooth_descr[] = "optimized_smooth: Optimized implementation";
void optimized_smooth(int dim, pixel * src, pixel * dst) {
    int i, j;
    int tmp;

    dst[0].red = (src[0].red + src[1].red + src[dim].red + src[dim + 1].red) >> 2;
    dst[0].green = (src[0].green + src[1].green + src[dim].green + src[dim + 1].green) >> 2;
    dst[0].blue = (src[0].blue + src[1].blue + src[dim].blue + src[dim + 1].blue) >> 2;
    i = dim - 1;
    dst[i].red = (src[i].red + src[i - 1].red + src[i + dim].red + src[i + dim -1].red) >> 2;
    dst[i].green = (src[i].green + src[i - 1].green + src[i + dim].green + src[i + dim -1].green) >> 2;
    dst[i].blue = (src[i].blue + src[i - 1].blue + src[i + dim].blue + src[i + dim -1].blue) >> 2;
    i = i * dim;
    dst[i].red = (src[i].red + src[i + 1].red + src[i - dim].red + src[i - dim + 1].red) >> 2;
    dst[i].green = (src[i].green + src[i + 1].green + src[i - dim].green + src[i - dim + 1].green) >> 2;
    dst[i].blue = (src[i].blue + src[i + 1].blue + src[i - dim].blue + src[i - dim + 1].blue) >> 2;
    i = i + dim - 1;
    dst[i].red = (src[i].red + src[i - 1].red + src[i - dim].red + src[i - dim - 1].red) >> 2;
    dst[i].green = (src[i].green + src[i - 1].green + src[i - dim].green + src[i - dim - 1].green) >> 2;
    dst[i].blue = (src[i].blue + src[i - 1].blue + src[i - dim].blue + src[i - dim - 1].blue) >> 2;

    for (i = 1 ; i < dim - 1 ; i++) {
        dst[i].red = (src[i].red + src[i - 1].red + src[i + 1].red + src[i + dim].red + src[i + dim - 1].red + src[i + dim + 1].red) / 6;
        dst[i].green = (src[i].green + src[i - 1].green + src[i + 1].green + src[i + dim].green + src[i + dim - 1].green + src[i + dim + 1].green) / 6;
        dst[i].blue = (src[i].blue + src[i - 1].blue + src[i + 1].blue + src[i + dim].blue + src[i + dim - 1].blue + src[i + dim + 1].blue) / 6;
    }
    for (i = dim * (dim - 1) + 1 ; i < dim * dim - 1 ; i++) {
        dst[i].red = (src[i].red + src[i - 1].red + src[i + 1].red + src[i - dim].red + src[i - dim - 1].red + src[i - dim + 1].red) / 6;
        dst[i].green = (src[i].green + src[i - 1].green + src[i + 1].green + src[i - dim].green + src[i - dim - 1].green + src[i - dim + 1].green) / 6;
        dst[i].blue = (src[i].blue + src[i - 1].blue + src[i + 1].blue+ src[i - dim].blue + src[i - dim - 1].blue+ src[i - dim + 1].blue) / 6;
    }
    for (j = dim ; j < dim * (dim - 1) ; j+= dim) {
        dst[j].red = (src[j].red + src[j + 1].red + src[j - dim].red + src[j - dim + 1].red + src[j + dim].red + src[j + dim + 1].red) / 6;
        dst[j].green = (src[j].green + src[j + 1].green + src[j - dim].green+ src[j - dim + 1].green + src[j + dim].green + src[j + dim + 1].green) / 6;
        dst[j].blue = (src[j].blue + src[j + 1].blue + src[j - dim].blue + src[j - dim + 1].blue + src[j + dim].blue + src[j + dim + 1].blue) / 6;
    }
    for (j = 2 * dim - 1 ; j < dim * dim - 1 ; j += dim) {
        dst[j].red = (src[j].red + src[j - 1].red + src[j - dim].red + src[j - dim - 1].red + src[j + dim].red + src[j + dim - 1].red) / 6;
        dst[j].green = (src[j].green + src[j - 1].green + src[j - dim].green + src[j - dim - 1].green + src[j + dim].green + src[j + dim - 1].green) / 6;
        dst[j].blue = (src[j].blue + src[j - 1].blue + src[j - dim].blue + src[j - dim - 1].blue + src[j + dim].blue + src[j + dim - 1].blue) / 6;
    }

    for (i = 1 ; i < dim - 1 ; i++) {
        for (j = 1 ; j < dim - 1 ; j++) {
            tmp = i * dim + j;
            dst[tmp].red = (src[tmp].red + src[tmp - 1].red + src[tmp + 1].red + src[tmp - dim].red + src[tmp - dim - 1].red + src[tmp - dim + 1].red + src[tmp + dim].red + src[tmp + dim - 1].red + src[tmp + dim + 1].red) / 9;
            dst[tmp].green = (src[tmp].green + src[tmp - 1].green + src[tmp + 1].green + src[tmp - dim].green + src[tmp - dim - 1].green + src[tmp - dim + 1].green + src[tmp + dim].green + src[tmp + dim - 1].green + src[tmp + dim + 1].green) / 9;
            dst[tmp].blue = (src[tmp].blue + src[tmp - 1].blue + src[tmp + 1].blue + src[tmp - dim].blue + src[tmp - dim - 1].blue + src[tmp - dim + 1].blue + src[tmp + dim].blue + src[tmp + dim - 1].blue + src[tmp + dim + 1].blue) / 9;
        }
    }
}

/*
 * smooth - Your current working version of smooth.
 * IMPORTANT: This is the version you will be graded on
 */
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst)
{
    optimized_smooth(dim, src, dst);
}


/*********************************************************************
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_smooth_functions() {
    add_smooth_function(&smooth, smooth_descr);
    add_smooth_function(&naive_smooth, naive_smooth_descr);
    /* ... Register additional test functions here */
    add_smooth_function(&optimized_smooth, optimized_smooth_descr);
}
