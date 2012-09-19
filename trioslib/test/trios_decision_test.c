#include <trios.h>
#include "minunit.h"

UTEST(test_decision_pair1) {
    int i, j;
    imgset_t *set = imgset_create(1, 2);
    imgset_set_dname(set, 1, "./test_img/");
    imgset_set_dname(set, 2, "./test_img/");
    imgset_set_fname(set, 1, 1, "input1.pgm");
    imgset_set_fname(set, 2, 1, "ideal1.pgm");
    imgset_write("IMGSET.s", set);
    imgset_free(set);

    window_t *win = win_create(5, 5, 1);
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            win_set_point(i, j, 1, 1, win);
        }
    }
    win_write("WIN.w", win);
    win_free(win);

    mu_assert("lcollec failed.", 1 == lcollec("IMGSET.s", "WIN.w", NULL, 1, 1, 0, "XPL_RESULT.xpl", NULL));
    mu_assert("ldecision failed", 1 == ldecision_disk("XPL_RESULT.xpl", 1, 0, AVERAGE, 0, 0, "DECISION_RESULT.mtm"));
} TEST_END

UTEST(test_decision_pair1_memory) {
    int i, j;
    imgset_t *set = imgset_create(1, 2);
    imgset_set_dname(set, 1, "./test_img/");
    imgset_set_dname(set, 2, "./test_img/");
    imgset_set_fname(set, 1, 1, "input1.pgm");
    imgset_set_fname(set, 2, 1, "ideal1.pgm");
    imgset_write("IMGSET.s", set);
    imgset_free(set);

    window_t *win = win_create(5, 5, 1);
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            win_set_point(i, j, 1, 1, win);
        }
    }
    win_write("WIN.w", win);
    win_free(win);

    mu_assert("lcollec failed.", 1 == lcollec("IMGSET.s", "WIN.w", NULL, 1, 1, 0, "XPL_RESULT.xpl", NULL));
    xpl_t *xpl = xpl_read("XPL_RESULT.xpl", &win, NULL);
    mtm_t *result = ldecision_memory(xpl, 1, 0, AVERAGE, 0, 0);
    mu_assert("ldecision failed", NULL != result);
    xpl_free(xpl);
    win_free(win);
    mtm_free(result);
} TEST_END

UTEST(test_decision_pair1_gg_memory) {
    int i, j;
    imgset_t *set = imgset_create(1, 2);
    imgset_set_dname(set, 1, "./test_img/");
    imgset_set_dname(set, 2, "./test_img/");
    imgset_set_fname(set, 1, 1, "input1_GG.pnm");
    imgset_set_fname(set, 2, 1, "ideal1_GG.pnm");
    imgset_write("IMGSET.s", set);
    imgset_free(set);

    window_t *win = win_create(5, 5, 1);
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            win_set_point(i, j, 1, 1, win);
        }
    }
    win_write("WIN.w", win);
    win_free(win);

    mu_assert("lcollec failed.", 1 == lcollec("IMGSET.s", "WIN.w", NULL, 0, 0, 0, "XPL_RESULT_GG1.xpl", NULL));
    xpl_t *xpl = xpl_read("XPL_RESULT_GG1.xpl", &win, NULL);
    mtm_t *result = ldecision_memory(xpl, 0, 0, AVERAGE, 0, 0);
    mu_assert("ldecision failed", NULL != result);
    mtm_write("RESULT_GG1.mtm", result, win, NULL);
    xpl_free(xpl);
    win_free(win);
    mtm_free(result);
} TEST_END


#include "runner.h"
