## Wdecoder

This project contains different dynamic token-passing decoders to be used with AaltoASR acoustic models. The acoustic model states are read from a .ph file definition and acoustic probabilities from .lna files. Duration models (.dur) can be used optionally and are recommended. The project was initially intended for evaluating subword n-grams using a constrained vocabulary. It was also found out that word n-grams work reasonably well especially if used with class n-gram models.

This code is mostly (c) Matti Varjokallio with the exception of the code that is from the AaltoASR package: Acoustics, Hmm, NowayHmmReader, LnaReaderCircular, str, conf.
Aalto University internal research use is permitted.
Please contact for possible other purposes. 


### Features

* Different graphs
    * Unlimited vocabulary subword decoder with `<w>` symbols
    * Unlimited vocabulary subword decoder with boundaries in the left-most subword, i.e. `_talo a kin`
    * Unlimited vocabulary subword decoder with boundaries in the right-most subword, i.e. `talo a kin_`
    * Constrained vocabulary subword decoder (up to 4M vocabulary size tested)
    * Word decoder (up to 4M vocabulary size tested)

The graphs are created by inserting the words to a prefix tree. The cross-word/cross-unit network is connected next. The suffixes are then tied to reach the minimal size in all cases. 

* Pruning and look-ahead
    * Hypothesis recombination always done using the n-gram model states
    * Combination of beam pruning, word end beam pruning, node beam pruning, histogram pruning. To start with good global beam settings, the nodes are sorted by the highest likelihood prior to propagating the tokens to the next frame.
    * Inapproximate and precomputed unigram and bigram lookahead for all the graph types. With the subword n-grams, the bigram look-aheads give the best results. With unlimited vocabulary decoders use the setting `bigram-precomputed-full`, with constrained vocabulary `bigram-precomputed-hybrid`. For large vocabularies the bigram lookahead (`large-bigram`) is computationally quite heavy (long precomputation, high memory requirements and RTF in decoding) and mostly does not improve the results compared to the unigram look-ahead (0-1% percent relative so far) so `unigram` is recommended. This is computationally simple and gives reasonable results. For smaller vocabulary sizes like for English recognition the `bigram-precomputed-full` should be ok. Please note that the bigram look-ahead models have somewhat high memory consumption. The memory consumption for the full table bigram lookahead is the number of look-ahead states times the vocabulary size times the float size. The number of look-ahead states can be checked with the `lasc`executable.

### Notes

The graph construction and lookahead algorithms are pretty well unit tested so there shouldn't be any major problems in those. The accuracy should be equal or slightly better in Finnish tasks with the `<w>` symbols compared to the original AaltoASR decoder. In some cases (for instance Estonian BN task), however, this decoder may be several percent absolute better. This is due to slightly different silence loops along with the search improvements.

This code passes the tokens by-value. This has the advantage that the decoding code is fairly simple. The recognition history is stored separately. Currently there is no support for lattice generation. In principle it shouldn't be so big effort to implement, but testing for all the graph types could take some time.

I have mainly used this with the traditional simple Finnish pronuncation lexicon, i.e. one pronunciation variant per word. Multiple pronunciation variants should work already, but it is better to test with one pronunciation variant per word at first.

Only triphone models are supported.

Graphs are created by a separate program which is given as input to the `decode` executable.

### Programs

* `cleanlex`, removes words without proper triphones
* `decode`, normal n-gram decoder for all the graph types
* `class-decode`, class n-gram decoder for word-based recognition and the unlimited vocabulary recognizers without `<w>` symbol
* `class-ip-decode`, interpolated n-gram and class n-gram decoder for word-based recognition and the unlimited vocabulary recognizers without `<w>` symbol
* `wsw-decode`, constrained vocabulary word based recognizer with a three-way model interpolation, word n-gram, class n-gram over words, subword n-gram.
* `lasc`, counts look-ahead states
* `lastates`, precomputes `large-bigram` lookahead scores
* `score`, scores utterances
* `segment`, segments utterances to triphone states, selects the most likely silence path
* `swgraph`, normal `<w>` unlimited vocabulary graph
* `swwgraph`, constrained vocabulary subword n-gram graph
* `wgraph`, word graph
* `lwbswgraph`, unlimited vocabulary graph with the word boundary marker in the left-most subword
* `rwbswgraph`, unlimited vocabulary graph with the word boundary marker in the right-most subword

### Configuration

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
