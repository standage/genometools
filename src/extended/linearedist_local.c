/*
  Copyright (C) 2015 Annika Seidel, annika.seidel@studium.uni-hamburg.de

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "core/assert_api.h"
#include "core/unused_api.h"
#include "core/error.h"
#include "core/array.h"
#include "core/ma.h"
#include "core/minmax.h"

#include "extended/linearedist_local.h"
#include "extended/linearedist.h"
#include "extended/alignment.h"
#include "extended/max.h"

#define LINEAR_EDIST_GAP          ((GtUchar) UCHAR_MAX)

static void firstLStabcolumn(GtWord *Ltabcolumn,
                             GtUwordPair *Starttabcolumn,
                             GtUword ulen)
{
  GtUword rowindex;

  gt_assert(Ltabcolumn != NULL && Starttabcolumn != NULL);
  for (rowindex = 0; rowindex <= ulen; rowindex++)
  {
    Ltabcolumn[rowindex] = 0;
    Starttabcolumn[rowindex].a = rowindex;
    Starttabcolumn[rowindex].b = 0;
  }
}

static void nextLStabcolumn(GtWord *Ltabcolumn,
                            GtUwordPair *Starttabcolumn,
                            GtUword colindex, GtUchar b,
                            const GtUchar *useq, GtUword ulen,
                            Gtmaxcoordvalue *max,
                            const GtWord matchscore,
                            const GtWord mismatchscore,
                            const GtWord gapscore)
{
  GtUword rowindex;
  GtUwordPair northwestStarttabentry, westStarttabentry;
  GtWord val, northwestLtabentry, westLtabentry;

  gt_assert(max != NULL);
  westLtabentry = Ltabcolumn[0];
  westStarttabentry = Starttabcolumn[0];

  Ltabcolumn[0] = 0;
  Starttabcolumn[0].a = 0;
  Starttabcolumn[0].b = colindex;
  for (rowindex = 1UL; rowindex <= ulen; rowindex++)
  {
    northwestLtabentry = westLtabentry;
    northwestStarttabentry = westStarttabentry;
    westLtabentry = Ltabcolumn[rowindex];
    westStarttabentry = Starttabcolumn[rowindex];
    Ltabcolumn[rowindex] += gapscore;

    if ((val = northwestLtabentry + (useq[rowindex-1] ==
         b ? matchscore : mismatchscore)) > Ltabcolumn[rowindex])
    {
      Ltabcolumn[rowindex] = val;
      Starttabcolumn[rowindex] = northwestStarttabentry;
    }
    if ((val = Ltabcolumn[rowindex-1]+gapscore) > Ltabcolumn[rowindex])
    {
      Ltabcolumn[rowindex] = val;
      Starttabcolumn[rowindex]=Starttabcolumn[rowindex-1];
    }
    if ((val = 0) > Ltabcolumn[rowindex])
    {
      Ltabcolumn[rowindex] = val;
      Starttabcolumn[rowindex].a = rowindex;
      Starttabcolumn[rowindex].b = colindex;
    }
    if (Ltabcolumn[rowindex] > gt_max_get_value(max))
    {
      gt_max_set_value(max, Ltabcolumn[rowindex]);
      gt_max_set_start(max, Starttabcolumn[rowindex]);
      gt_max_set_end(max, rowindex, colindex);

    }
  }
}

static Gtmaxcoordvalue *evaluateallLScolumns(GtWord *Ltabcolumn,
                                   GtUwordPair *Starttabcolumn,
                                   const GtUchar *useq,
                                   const GtUchar *vseq,
                                   GtUword ulen, GtUword vlen,
                                   const GtWord matchscore,
                                   const GtWord mismatchscore,
                                   const GtWord gapscore)
{
  GtUword colindex;
  Gtmaxcoordvalue *max;

  firstLStabcolumn(Ltabcolumn, Starttabcolumn, ulen);
  max = gt_max_new();
  for (colindex = 1UL; colindex <= vlen; colindex++)
  {
    nextLStabcolumn(Ltabcolumn, Starttabcolumn, colindex,
                    vseq[colindex-1], useq, ulen, max,
                    matchscore, mismatchscore, gapscore);
  }
  return max;
}

static GtWord gt_calc_linearscore(const GtUchar *useq, GtUword ulen,
                                  const GtUchar *vseq,  GtUword vlen,
                                  const GtWord matchscore,
                                  const GtWord mismatchscore,
                                  const GtWord gapscore)
{
    Gtmaxcoordvalue *max;
    GtWord score, *Ltabcolumn;
    GtUwordPair *Starttabcolumn;

    Ltabcolumn = gt_malloc(sizeof *Ltabcolumn * (ulen+1));
    Starttabcolumn = gt_malloc(sizeof *Starttabcolumn * (ulen+1));

    max = evaluateallLScolumns(Ltabcolumn, Starttabcolumn,useq,vseq,
                               ulen, vlen, matchscore, mismatchscore, gapscore);
    score = gt_max_get_value(max);
    gt_max_delete(max);

    return(score);
}

static GtAlignment *gt_calc_linearalign_local(const GtUchar *useq,GtUword ulen,
                                              const GtUchar *vseq,GtUword vlen,
                                              const GtWord matchscore,
                                              const GtWord mismatchscore,
                                              const GtWord gapscore)
{
  GtWord *Ltabcolumn;
  GtUwordPair *Starttabcolumn;
  GtUword ulen_part, vlen_part;
  const GtUchar *useq_part, *vseq_part;
  GtAlignment *align;
  Gtmaxcoordvalue *max;

  Ltabcolumn = gt_malloc(sizeof *Ltabcolumn * (ulen+1));
  Starttabcolumn = gt_malloc(sizeof *Starttabcolumn * (ulen+1));

  max = evaluateallLScolumns(Ltabcolumn,
                             Starttabcolumn,
                             useq,vseq,
                             ulen, vlen,
                             matchscore,
                             mismatchscore,
                             gapscore);

  useq_part = &useq[(gt_max_get_start(max)).a];
  vseq_part = &vseq[(gt_max_get_start(max)).b];
  ulen_part = gt_max_get_row_length(max);
  vlen_part = gt_max_get_col_length(max);

  align = gt_alignment_new_with_seqs(useq_part,ulen_part,vseq_part,vlen_part);

  /*hier noch score in kosten an globale funktion umwandeln*/
  gt_calc_linearalign(useq_part, ulen_part, vseq_part, vlen_part, align);
  gt_max_delete(max);

  return align;
}

static bool gap_symbol_in_sequence(const GtUchar *seq, GtUword len)
{
  const GtUchar *sptr;

  for (sptr = seq; sptr < seq + len; sptr++)
  {
    if (*sptr == LINEAR_EDIST_GAP)
    {
      return true;
    }
  }
  return false;
}

static GtWord fillLtable(GtWord *lcolumn,
                         const GtUchar *u, GtUword ulen,
                         const GtUchar *v, GtUword vlen)
{
  GtUword i, j;
  GtWord nw, we, max = 0;
  for (i = 0; i <= ulen; i++)
    lcolumn[i] = 0;
  for (j = 1UL; j <= vlen; j++)
  {
    nw = lcolumn[0];
    lcolumn[0] = 0;
    for (i = 1UL; i <= ulen; i++)
    {
      we = lcolumn[i];
      lcolumn[i] = nw + (u[i-1] == v[j-1] ? 2 : -2); /* replacement */
      if (lcolumn[i-1] - 1 > lcolumn[i]) /* deletion */
        lcolumn[i] = lcolumn[i-1] - 1;
      if (we + 1 >lcolumn[i]) /* insertion */
        lcolumn[i] = we - 1;
      if (0 > lcolumn[i])
        lcolumn[i]=0;
      nw = we;
      if (lcolumn[i] > max)
        max = lcolumn[i];
    }
  }
  return max;
}

GtUword gt_calc_linearscore_with_table(const GtUchar *u, GtUword ulen,
                                       const GtUchar *v, GtUword vlen)
{
  GtWord *lcolumn, score;

  lcolumn = gt_malloc(sizeof *lcolumn * (MIN(ulen,vlen) + 1));
  score = fillLtable(lcolumn, ulen <= vlen ? u : v, MIN(ulen,vlen),
                     ulen <= vlen ? v : u, MAX(ulen,vlen));
  gt_free(lcolumn);
  return score;
}

void gt_computelinearspace_local(const GtUchar *useq, GtUword ulen,
                                 const GtUchar *vseq, GtUword vlen,
                                 const GtWord matchscore,
                                 const GtWord mismatchscore,
                                 const GtWord gapscore)
{
  GtAlignment *align;
  align = gt_calc_linearalign_local(useq, ulen, vseq, vlen,
                                    matchscore, mismatchscore, gapscore);

  gt_alignment_show(align, stdout, 80);
  gt_alignment_delete(align);
}

void gt_checklinearspace_local(GT_UNUSED bool forward,
                               const GtUchar *useq, GtUword ulen,
                               const GtUchar *vseq, GtUword vlen)
{
  GtUword score1, score2, score3;
  GtAlignment *align;

  if (gap_symbol_in_sequence(useq,ulen))
  {
    fprintf(stderr,"%s: sequence u contains gap symbol\n",__func__);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }
  if (gap_symbol_in_sequence(vseq,vlen))
  {
    fprintf(stderr,"%s: sequence v contains gap symbol\n",__func__);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }

  align = gt_calc_linearalign_local(useq, ulen, vseq, vlen, 2, -2, -1);
  score1 = gt_alignment_eval_with_score(align, 2,-2,-1);
  score2 = gt_calc_linearscore_with_table(useq, ulen, vseq, vlen);

  if (score1 != score2)
  {
    fprintf(stderr,"gt_calc_linearalign_local = "GT_WU" != "GT_WU
            " = gt_calc_linearscore_with_table\n", score1, score2);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }

  score3 = gt_calc_linearscore(useq, ulen, vseq, vlen, 2, -2, -1);
  if (score1 != score3)
  {
    fprintf(stderr,"gt_calc_linearalign_local = "GT_WU" != "GT_WU
            " = gt_calc_linearscore\n", score1, score3);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }

  gt_alignment_delete(align);
}
