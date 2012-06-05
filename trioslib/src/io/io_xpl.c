#include <pacbasic.h>
#include <pacio.h>
#include "pacio_local.h"

/* #define _DEBUG_ */
/* #define _DEBUG_1_ */
/* #define _DEBUG_2_ */

/*  Modification by:  Nina S. Tomita, R. Hirata Jr. (nina@ime.usp.br)       */
/*  Date: Tue Jan 13 1998                                                   */
/*  Mod: Modified to include WK support                                     */

/* Modification by:  Nina S. Tomita, R. Hirata Jr. (nina@ime.usp.br)       */
/* Date: Wed Feb 17 2000                                                   */
/* Mod: Window structure (1) changed to support multiple bands, (2) no     */
/*      longer holds information about aperture.                           */
/*      New structure "apert_t" was created.                               */
/*      Modifications were made to adequate to the structure changing.     */


/*
     -------------------------------------------
     FUNCTION: xpl_read
     -------------------------------------------
*/

xpl_t *             /*+ Purpose: Read an examples file   +*/
  xpl_read(
    char *fname,    /*+ In: name of the file of examples      +*/
    window_t **win, /*+ Out: pointer to window structure      +*/
    apert_t  **apt  /*+ Out: pointer to an aperture structure +*/
  )
/*+ Return: Pointer to example, window structure and aperture structure    +*/
{
/*  author:  Nina S. Tomita, Roberto Hirata Jr. (nina@ime.usp.br)           */
/*  date: Thu Oct 17 1996                                                   */

/*  Modification by:  Nina S. Tomita, R. Hirata Jr. (nina@ime.usp.br)       */
/*  Date: Mon Nov 3 1997                                                    */
/*  Mod: Modified to support new file format                                */

/*  Date: Thu Feb 17 2000                                                   */
/*  Mod: New parameter (aperture)                                           */

  FILE      *fd ;
  xpl_t     *xpl ; 
  freq_node *freqnode, *freqlist ;   /* pointers to a frequency node */

  char tag, dot ;
  int  tags_read, stop, i, j ;

  unsigned int n_nodes, sum, freq, fq0, fq1 ;
  int          type, wsize, wzip, label ;

  char *wpat ;
  unsigned int  *bwpat, x;


  tags_read = 0 ;
  stop = 0 ;
  *apt = NULL ;


  if((fd=fopen(fname, "r"))==NULL) {
    return (xpl_t *)trios_error(1, "File (%s) open failed.", fname) ;
  }
 
  if(!header_match(fd, "EXAMPLE ")) {
    fclose(fd) ;
    return (xpl_t *)trios_error(1, "File header does not match.") ;
  }


  /* read data header section */
  while(!stop)  {
    
    while(((dot=(char)fgetc(fd)) != '.') && (dot!=EOF)) ;

    if(dot==(char)EOF) {
      fclose(fd) ;
      return (xpl_t *)trios_error(1, "Unexpected end of file. No tag found.") ;
    }

    tag = (char)fgetc(fd) ;

    switch (tag) {
      

    /* get type ------------------------------------- */
  
    case 't': 
      if(1 != fscanf(fd, "%d", &type)) {
        fclose(fd) ;
        trios_fatal("Unexpected data or end of file") ;
      }
      tags_read++ ;
      break ;

    /* get number of different examples                                 */

    case 'n':
      if(1 != fscanf(fd, "%d", &n_nodes)) {
        fclose(fd) ;
        trios_fatal("Unexpected data or end of file") ;
      }
      tags_read++ ;
      break ;


    /* get the total number of examples                                 */ 
    case 's':
      if(1 != fscanf(fd, "%d", &sum)) {
        fclose(fd) ;
        trios_fatal("Unexpected data or end of file") ;
      }
      tags_read++ ;
      break ;

    /* read window information ---------------------------------------- */
    case 'W':
      if(NULL==(*win = win_read_data(fd))) {
	fclose(fd) ;
	return (xpl_t *)trios_error(MSG, "xpl_read: win_read_data() failed.") ;
      }
      tags_read++ ;
      break ;

    /* read aperture information -------------------------------------- */
    case 'A':
        trios_error(MSG, "Aperture operators are not supported at this moment");
      /*if(NULL==(*apt = apert_read_data(fd))) {
	      fclose(fd) ;
    	  return (xpl_t *)pac_error(MSG, "xpl_read: apert_read_data() failed.") ;
      }*/
      break ;

    case 'd' : 
      stop = 1 ;
      break ;

    default : 
      fclose(fd) ;
      (void)trios_error(1,"Unexpected tag %c ",tag) ;
      return (xpl_t *)trios_error(1, " File format error") ;
    }
  }


  if(tags_read!=4) 
    return (xpl_t *)trios_error(1, "Missing Parameters. Looking for .t, .n, .s or .W") ;

#ifdef _DEBUG_1_
pac_debug("type=%d nodes=%d sum=%d", type, n_nodes, sum) ;
#endif


  /* read data section */
  switch(type) {
    case BB:

      wsize = win_get_wsize(*win);

      if(NULL==(xpl = xpl_create(wsize,BB))) {
	fclose(fd);
	return (xpl_t *)trios_error(MSG, "xpl_read: xpl_create() failed.") ;
      }

      wzip = xpl_get_wzip(xpl) ;

      if((bwpat = (unsigned int *)malloc(sizeof(int)*wzip)) == NULL) {
	fclose(fd) ;
	return 
          (xpl_t *)trios_error(1, "Mmemory allocation error.") ;
      }

      /* reading loop --------------------------------- */
#ifdef _DEBUG_1_
pac_debug("wzip=%d", wzip) ;
#endif

      for(i= 0; i<n_nodes ; i++) {

	for(j=0; j<wzip; j++) {
	  if(1 != fscanf(fd, "%x ", &bwpat[j])) {
            free(bwpat) ;
            fclose(fd) ;
            trios_fatal("Unexpected data or end of file") ;
          }
	}

	fq0 = fq1 = 0 ;

	do{
	  if(1 != fscanf(fd, "%d", &freq)) {
            free(bwpat) ;
            fclose(fd) ;
            trios_fatal("Unexpected data or end of file") ;
          }
  
	  if (freq) {
	    if(1 != fscanf(fd, "%d", &label)) {
              free(bwpat) ;
              fclose(fd) ;
              trios_fatal("Unexpected data or end of file") ;
            }

	    if(label)  /* The first freq must be different of zero */
	      fq1 = freq ;
	  
	    else fq0 = freq ;
	  }
	}
	while(freq) ;
	
        /* Remember : xpl_BB_insert increments xpl->sum, and xpl->nodes when  */
        /*            it is neccessary                                        */

#ifdef _DEBUG_2_
pac_debug("bwpat=%x fq0=%d fq1=%d", bwpat[0], fq0, fq1) ;
#endif

	if(xpl_BB_insert(xpl, (xpl_BB **)(&xpl->root), bwpat, fq0, fq1) == -1) {
          free(bwpat) ;
	  fclose(fd) ;
	  return (xpl_t *)trios_error(MSG,"read_xpl : xpl_BB_insert() failed.") ;
	}
      }
      free(bwpat) ;
      break;

    case BG:
        trios_error(MSG, "This operator is not supported yet");
        /*
        wsize = win_get_wsize(*win);
    
        if(NULL==(xpl = xpl_create(wsize,BG))) {
            fclose(fd);
	        return (xpl_t *)pac_error(MSG, "xpl_read: xpl_create() failed.") ;
        }

      wzip = xpl_get_wzip(xpl) ;

      if((bwpat = (unsigned int *)malloc(sizeof(int)*wzip)) == NULL) {
	fclose(fd) ;
	return 
          (xpl_t *)pac_error(1, "Memory allocation error.") ;
      }

      /* reading loop --------------------------------- */
#ifdef _DEBUG_1_
pac_debug("n_nodes=%d", n_nodes) ;
#endif
        /*
      for(i= 0; i<n_nodes ; i++) {

	for(j=0; j<wzip; j++) {
	  if(1 != fscanf(fd, "%x ", &bwpat[j])) {
            free(bwpat) ;
            fclose(fd) ;
            pac_fatal("Unexpected data or end of file") ;
          }
	}

	freqlist = NULL ;

	do{
	  if(1 != fscanf(fd, "%d", &freq)) {
            free(bwpat) ;
            fclose(fd) ;
            pac_fatal("Unexpected data or end of file") ;
          }

	  if (freq) {
	    if(1 != fscanf(fd, "%d", &label)) {
              free(bwpat) ;
              fclose(fd) ;
              pac_fatal("Unexpected data or end of file") ;
            }

	    if((freqnode=freq_node_create(label, freq))==NULL) {
              free(bwpat) ;
              fclose(fd) ;
	      return (xpl_t *)pac_error(MSG, "read_xpl: freq_node_create() failed.") ;
	    }
	    
#ifdef _DEBUG_2_
pac_debug("freqnode criado");
pac_debug("label=%d , freq=%d\n", freqnode->label, freqnode->freq) ;
#endif

	    if (!set_freq(freqnode, &freqlist)) {
              free(bwpat) ;
              fclose(fd) ;
	      return (xpl_t *)pac_error(MSG, "xpl_read: set_freq() failed.") ;
	    }
	  }
	}

	while(freq) ;
	
        /* Remember : xpl_BG_insert increments xpl->sum, and xpl->nodes when  */
        /*            it is neccessary                                        */
    /*
	if(xpl_BG_insert(xpl, (xpl_BG **)(&xpl->root), bwpat, freqlist) == -1) {
          free(bwpat) ;
	  fclose(fd) ;
	  return (xpl_t *)pac_error(MSG,"read_xpl : xpl_BG_insert() failed.") ;
	}
      }
      free(bwpat) ;*/
      break;

    case GG:
    case WKF:
    case WKC:
    case WK3C:
    case WK3F:
    case WKGG2F:
    case WKGG2C:
    case GG3:
        trios_error(MSG, "This operator is not supported yet"); /*
      wsize = win_get_wsize(*win);

      if(NULL==(xpl = xpl_create(wsize,type))) {
	fclose(fd);
	return (xpl_t *)pac_error(MSG, "xpl_read: xpl_create() failed.") ;
      }

      if((wpat = (char *)malloc(sizeof(char)*wsize)) == NULL) {
	fclose(fd) ;
	return 
          (xpl_t *)pac_error(MSG, "Memory allocation error.") ;
      }

      /* reading loop --------------------------------- */
        /*
      for(i= 0; i<n_nodes ; i++) {

	for(j=0; j<wsize; j++) {
	  if(1 != fscanf(fd, "%d ", &x)) {
            fclose(fd) ;
            pac_fatal("Unexpected data or end of file") ;
          }
	  wpat[j] = (char) x ;
	}

	freqlist = NULL ;

	do{
	  if(1 != fscanf(fd, "%d", &freq)) {
            fclose(fd) ;
            pac_fatal("Unexpected data or end of file") ;
          }

	  if (freq) {
	    if(1 != fscanf(fd, "%d", &label)) {
              fclose(fd) ;
              pac_fatal("Unexpected data or end of file") ;
            }

	    if((freqnode=freq_node_create(label, freq))==NULL) {
	      free(wpat) ;
              fclose(fd) ;
	      return (xpl_t *)pac_error(MSG, "read_xpl: freq_node_create() failed.") ;
	    }
	    
#ifdef _DEBUG_2_
pac_debug("freqnode criado");
pac_debug("label=%d , freq=%d\n", freqnode->label, freqnode->freq) ;
#endif

	    if (!set_freq(freqnode, &freqlist)) {
	      free(wpat) ;
              fclose(fd) ;
	      return (xpl_t *)pac_error(MSG, "xpl_read: set_freq() failed.") ;
	    }
	  }
	}
	while(freq) ;


	/* insert new w-pattern into example's set */
        /* Remember : xpl_GG_insert increments xpl->sum, and xpl->nodes when  */
        /*            it is neccessary                                        */
	/*
	if(xpl_GG_insert(xpl, (xpl_GG **)(&xpl->root), wpat, freqlist) == -1) {
	  fclose(fd) ;
	  return (xpl_t *)pac_error(MSG," read_xpl : xpl_GG_insert() failed.") ;
	}
      }
      free(wpat) ;*/
      break;

    default:
      fclose(fd) ;
      return (xpl_t *)trios_error(1, "Mapping %d not implemented yet.", type) ;
  }

#ifdef _DEBUG_
pac_debug("xpl type=%d", xpl->type) ;
#endif

  fclose(fd) ;
  return(xpl) ;
}



/*
     -------------------------------------------
     FUNCTION: xpl_write
     -------------------------------------------
*/

int               /*+ Purpose: Write the examples file     +*/  
  xpl_write(
    char *fname,  /*+ In: Name of the file to be written   +*/
    xpl_t *xpl,   /*+ In: Pointer to example structure     +*/
    window_t *win,/*+ In: Pointer to window structure      +*/
    apert_t  *apt /*+ In: Pointer to an aperture structure +*/
  )
/*+ Return: 1 if success, 0 if failure                     +*/
{
/*  author:  Nina S. Tomita, Roberto Hirata Jr. (nina@ime.usp.br)           */
/*  date: Thu Oct 17 1996                                                   */

/*  Modification by:  Nina S. Tomita, R. Hirata Jr. (nina@ime.usp.br)       */
/*  Date: Mon Nov 3 1997                                                    */
/*  Mod: Modified to support new file format                                */

/*  Date: Thu Feb 17 2000                                                   */
/*  Mod: New parameter (aperture)                                           */


  header_t xplHeader = { "EXAMPLE ", ""};
  FILE     *fd ;
  int      type, wzip, wsize ;



#ifdef _DEBUG_
pac_debug("Entrou no XPL_WRITE") ;
#endif

  fd = fopen(fname, "w") ;
  if(fd == NULL) 
    return trios_error(1, "write_xpl : file open error.") ;
  
#ifdef _DEBUG_
pac_debug("abriu arquivo\n") ;
#endif

  header_write(fd, &xplHeader);
#ifdef _DEBUG_
pac_debug("escreveu header no arquivo") ;
#endif

  type = xpl_get_type(xpl) ;

#ifdef _DEBUG_
pac_debug("type=%d", type) ;
#endif

  fprintf(fd, "%s %d\n", ".t", type) ;
  fprintf(fd, "%s %d\n", ".n", xpl_get_n_nodes(xpl)) ;
  fprintf(fd, "%s %d\n", ".s", xpl_get_sum(xpl)) ;

  fprintf(fd, "%s\n", ".W") ;
  win_write_data(fd, win);


  if((type>3)&&(type<10)) {
    fprintf(fd, "%s\n", ".A") ;
    //apert_write_data(fd, apt);
    trios_error(MSG, "Aperture operatos are not supported at this moment");
  }

 
  fprintf(fd, "%s\n", ".d") ;


  wzip = xpl_get_wzip(xpl) ;  
  wsize = xpl->wsize ;

  switch(type) {
  
    case BB:
      xpl_BB_write_tree(fd, (xpl_BB *)(xpl->root), wzip) ;
      break ;

    case BG:
        trios_error(MSG, "This operator is not supported at this moment");
      //xpl_BG_write_tree(fd, (xpl_BG *)(xpl->root), wzip) ;
      break ;

    case GG:
    case GG3:
        trios_error(MSG, "This operator is not supported at this moment");
      //xpl_GG_write_tree(fd, (xpl_GG *)(xpl->root), wsize) ;
      break ;

    case WKF:
    case WKC:
    case WK3F:
    case WK3C:
        trios_error(MSG, "This operator is not supported at this moment");
      //xpl_WK_write_tree(fd, (xpl_GG *)(xpl->root), wsize) ;
      break ;

    case WKGG2F:
    case WKGG2C:
        trios_error(MSG, "This operator is not supported at this moment");
      //xpl_WKGG_write_tree(fd, (xpl_GG *)(xpl->root),
      //                    win_get_band_wsize(win,1), wsize) ;
      break ;

    default:
      fclose(fd) ;
      return trios_error(1,"xpl_write: mapping %d not yet implemented.", type);
      break;
  }


  fclose(fd) ;
  return(1) ;

}




/*
     -------------------------------------------
     FUNCTION: xpl_BB_write_tree
     -------------------------------------------
*/

void             /*+ Purpose: Write BB examples                              +*/
  xpl_BB_write_tree(
    FILE *fd,    /*+ In: file descriptor where the examples will be written  +*/
    xpl_BB *p,   /*+ In: Pointer to the examples tree                        +*/
    int wzip     /*+ In: compacted w-pattern size                            +*/
  )
/*+ Return: Nothing                                                          +*/
{
/*  author:  Nina S. Tomita, Roberto Hirata Jr. (nina@ime.usp.br)             */
/*  date: Thu Oct 17 1996                                                     */

  int i ;
  
  if(p != NULL) {

    xpl_BB_write_tree(fd, p->left, wzip) ;
    
#ifdef _DEBUG_2_
    if(p->wpat[0]==0) {
      pac_debug("ZERO %x", p->wpat[0]) ;
    }
#endif

    for(i=0; i<wzip; i++) {
      fprintf(fd, "%x ", p->wpat[i]) ;
    }

    if (p->fq1) 
      fprintf(fd, "%d %d ", p->fq1, 1) ;

    if (p->fq0) 
      fprintf(fd, "%d %d ", p->fq0, 0) ;

    fprintf(fd, "%d\n",0) ;

    xpl_BB_write_tree(fd, p->right, wzip) ;
  }

}



