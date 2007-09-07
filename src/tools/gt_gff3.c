/*
  Copyright (c) 2005-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2005-2007 Center for Bioinformatics, University of Hamburg

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

#include "libgtcore/option.h"
#include "libgtcore/outputfile.h"
#include "libgtcore/undef.h"
#include "libgtcore/versionfunc.h"
#include "libgtext/addintrons_stream.h"
#include "libgtext/gff3_in_stream.h"
#include "libgtext/gff3_out_stream.h"
#include "libgtext/mergefeat_stream_sorted.h"
#include "libgtext/sort_stream.h"

typedef struct {
  bool sort,
       mergefeat,
       addintrons,
       verbose;
  long offset;
  GenFile *outfp;
} Gff3_arguments;

static OPrval parse_options(int *parsed_args, Gff3_arguments *arguments,
                            int argc, const char **argv, Env *env)
{
  OptionParser *op;
  OutputFileInfo *ofi;
  Option *sort_option, *mergefeat_option, *addintrons_option, *option;
  OPrval oprval;
  env_error_check(env);

  /* init */
  op = option_parser_new("[option ...] [GFF3_file ...]",
                         "Parse, possibly transform, and output GFF3 files.",
                         env);
  ofi = outputfileinfo_new(env);

  /* -sort */
  sort_option = option_new_bool("sort", "sort the GFF3 features (memory "
                                "consumption is O(file_size))",
                                &arguments->sort, false, env);
  option_parser_add_option(op, sort_option, env);

  /* -mergefeat */
  mergefeat_option = option_new_bool("mergefeat", "merge adjacent features of "
                                     "the same type", &arguments->mergefeat,
                                     false, env);
  option_is_development_option(mergefeat_option);
  option_imply(mergefeat_option, sort_option, env);
  option_parser_add_option(op, mergefeat_option, env);

  /* -addintrons */
  addintrons_option = option_new_bool("addintrons", "add intron features "
                                      "between existing exon features",
                                      &arguments->addintrons, false, env);
  option_parser_add_option(op, addintrons_option, env);

  /* -offset */
  option = option_new_long("offset",
                           "transform all features by the given offset",
                           &arguments->offset, UNDEF_LONG, env);
  option_parser_add_option(op, option, env);

  /* -v */
  option = option_new_verbose(&arguments->verbose, env);
  option_parser_add_option(op, option, env);

  /* output file options */
  outputfile_register_options(op, &arguments->outfp, ofi, env);

  /* parse options */
  oprval = option_parser_parse(op, parsed_args, argc, argv, versionfunc, env);

  /* free */
  outputfileinfo_delete(ofi, env);
  option_parser_delete(op, env);

  return oprval;
}

int gt_gff3(int argc, const char **argv, Env *env)
{
  GenomeStream *gff3_in_stream,
               *sort_stream = NULL,
               *mergefeat_stream = NULL,
               *addintrons_stream = NULL,
               *gff3_out_stream,
               *last_stream;
  Gff3_arguments arguments;
  GenomeNode *gn;
  int parsed_args, had_err;
  env_error_check(env);

  /* option parsing */
  switch (parse_options(&parsed_args, &arguments, argc, argv, env)) {
    case OPTIONPARSER_OK: break;
    case OPTIONPARSER_ERROR: return -1;
    case OPTIONPARSER_REQUESTS_EXIT: return 0;
  }

  /* create a gff3 input stream */
  gff3_in_stream = gff3_in_stream_new_unsorted(argc - parsed_args,
                                               argv + parsed_args,
                                               arguments.verbose &&
                                               arguments.outfp, env);
  last_stream = gff3_in_stream;

  /* set offset (if necessary) */
  if (arguments.offset != UNDEF_LONG)
    gff3_in_stream_set_offset(gff3_in_stream, arguments.offset);

  /* create sort stream (if necessary) */
  if (arguments.sort) {
    sort_stream = sort_stream_new(gff3_in_stream, env);
    last_stream = sort_stream;
  }

  /* create merge feature stream (if necessary) */
  if (arguments.mergefeat) {
    assert(sort_stream);
    mergefeat_stream = mergefeat_stream_sorted_new(sort_stream, env);
    last_stream = mergefeat_stream;
  }

  /* create addintrons stream (if necessary) */
  if (arguments.addintrons) {
    assert(last_stream);
    addintrons_stream = addintrons_stream_new(last_stream, env);
    last_stream = addintrons_stream;
  }

  /* create gff3 output stream */
  gff3_out_stream = gff3_out_stream_new(last_stream, arguments.outfp, env);

  /* pull the features through the stream and free them afterwards */
  while (!(had_err = genome_stream_next_tree(gff3_out_stream, &gn, env)) &&
         gn) {
    genome_node_rec_delete(gn, env);
  }

  /* free */
  genome_stream_delete(gff3_out_stream, env);
  genome_stream_delete(sort_stream, env);
  genome_stream_delete(mergefeat_stream, env);
  genome_stream_delete(addintrons_stream, env);
  genome_stream_delete(gff3_in_stream, env);
  genfile_xclose(arguments.outfp, env);

  return had_err;
}
