#include <trios.h>
#include "minunit.h"



int                  /*+ Purpose: collect binary examples from a set of
                          gray scale images.                        +*/
  lcolthresh(
    char *fname_i1,  /*+ In: IMGSET file                                    +*/
    char *fname_i2,  /*+ In: WINSPEC file                                   +*/
    int  maxgrey,    /*+ In: maximun gray level value                       +*/
    char *o1_fname   /*+ Out: output XPL file                               +*/
);

TEST(test_pair1) {
    imgset_t *set = imgset_create(1, 2);
    imgset_set_fname(set, 1, 1, "./test_img/input1.pgm");
    imgset_set_fname(set, 2, 1, "./test_img/ideal1.pgm");
    imgset_write("IMGSET.s", set);
    imgset_free(set);

    window_t *win = win_create(2, 2, 1);
    win_set_point(0, 0, 1, 1, win);
    win_set_point(0, 1, 1, 1, win);
    win_set_point(1, 0, 1, 1, win);
    win_set_point(1, 1, 1, 1, win);
    win_write("WIN.w", win);
    win_free(win);

    mu_assert("lcollec failed.", 1 == lcolthresh("IMGSET.s", "WIN.w", 255, "THRESH.xpl"));
} TEST_END


#include "runner.h"