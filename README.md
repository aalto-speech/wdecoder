## Wdecoder

This project contains different dynamic token-passing decoders to be used with AaltoASR acoustic models.
The acoustic model states are read from a .ph file definition and acoustic probabilities from .lna files.
Duration models (.dur) can be used optionally.
The project was initially intended for evaluating subword n-grams using a constrained vocabulary.
Currently there is support also for other graph types.


### Features

* Different graphs
    * Unlimited vocabulary subword decoder with word boundary symbols `<w>`
    * Unlimited vocabulary subword decoder with boundaries in the left-most subword, i.e. `_talo a kin`
    * Unlimited vocabulary subword decoder with boundaries in the right-most subword, i.e. `talo a kin_`
    * Unlimited vocabulary subword decoder with boundaries both in the left-most and the right-most subword, i.e. `_talo a kin_`
    * Constrained vocabulary subword decoder (up to 4M vocabulary size tested)
    * Word decoder (up to 4M vocabulary size tested)

The graphs are created by inserting the words to a prefix tree.
The cross-word/cross-unit network is connected next.
The suffixes are then tied to reach the minimal size in all cases. 

* Pruning
    * Hypothesis recombination always done using the n-gram model states
    * Combination of beam pruning, word end beam pruning, node beam pruning, histogram pruning.

* Inapproximate and precomputed language model look-ahead for all decoder types
    * `unigram` is simple and fast approach for all decoders
    * `bigram-precomputed-full` is a good choice for unlimited vocabulary decoders and smaller vocabulary sizes like for English recognition
    * `bigram-precomputed-hybrid` can be used for the constrained vocabulary subword decoder
    * `large-bigram` implements the bigram look-ahead for large word vocabularies,
        this is computationally quite heavy (long precomputation, high memory requirements and RTF in decoding) and mostly does not improve the results compared to the unigram look-ahead (0-1% percent relative so far)
    * `class-bigram` implements class bigram -based look-ahead which works well for very large vocabularies

### Notes

The graph construction and lookahead algorithms are pretty well unit tested so there shouldn't be any major problems in those.
The accuracy should be equal or slightly better in Finnish tasks with the `<w>` symbols compared to the original AaltoASR decoder.
In some cases (for instance Estonian BN task), however, this decoder may be several percent absolute better.
This is due to slightly different silence loops along with the search improvements.

This code passes the tokens by-value.
This has the advantage that the decoding code is fairly simple.
The recognition history is stored separately.
Currently there is no support for lattice generation, but alternative hypotheses can be written to a n-best list file.
The decoder has mainly been used with the traditional simple Finnish pronuncation lexicon, i.e. one pronunciation variant per word.
Support for multiple pronunciation variants should be fairly easy to implement.

Only triphone models are supported at the moment.

Graphs are created by separate programs which is given as input to the decoder executables.

### Programs

##### Graph construction
* `wgraph`, word graph
* `swwgraph`, constrained vocabulary subword n-gram graph
* `swgraph`, normal unlimited vocabulary graph with `<w>` word boundary markers
* `lwbswgraph`, unlimited vocabulary graph with the word boundary marker in the left-most subword
* `rwbswgraph`, unlimited vocabulary graph with the word boundary marker in the right-most subword
* `lrwbswgraph`, unlimited vocabulary graph with the word boundary markers in both the left-most and the right-most subwords

##### Decoders
* `decode`, normal n-gram decoder for all the graph types
* `class-decode`, class n-gram decoder for word-based recognition and the unlimited vocabulary recognizers without `<w>` symbol
* `class-ip-decode`, interpolated n-gram and class n-gram decoder for word-based recognition and the unlimited vocabulary recognizers without `<w>` symbol
* `wsw-decode`, constrained vocabulary word based recognizer with a three-way model interpolation, word n-gram, class n-gram over words, subword n-gram.

##### Other
* `nbest-rescore-am`, rescores acoustic model scores in a n-best list
* `cleanlex`, removes words without proper triphones
* `lasc`, counts look-ahead states
* `lastates`, precomputes `large-bigram` lookahead scores
* `score`, scores utterances
* `segment`, segments utterances to triphone states, selects the most likely silence path

### Configuration file

Example configuration below. The lm scale, global beam, word end beam, duration scale and transition scale values are similar to the earlier AaltoASR decoder.

`lm_scale 32.0`  
`token_limit 35000`  
`duration_scale 3.0`  
`transition_scale 1.0`  
`global_beam 240.0`  
`word_end_beam 160.0`  
`node_beam 160.0`  
`force_sentence_end true`  
`history_clean_frame_interval 25`  
`stats 0`

If word boundary symbols are used, add the following

`word_boundary_symbol <w>`

Also if using interpolated models with `class-decode`, `class-ip-decode` or `wsw-decode` decoders, the interpolation weights need to be configured.
