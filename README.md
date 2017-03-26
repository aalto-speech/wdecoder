## Wdecoder

This project contains different dynamic token-passing decoders to be used with AaltoASR acoustic models. The acoustic model states are read from a .ph file definition and acoustic probabilities from .lna files. Duration models (.dur) can be used optionally. The project was initially intended for evaluating subword n-grams using a constrained vocabulary. It was also found out that word n-grams work reasonably well especially if used with class n-gram models.

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

The graphs are created by inserting the words to a prefix tree. The cross-word network is connected next. The suffixes are then tied to reach the minimal size in all cases. 

* Pruning and look-ahead
    * Hypothesis recombination always done using the n-gram model states
    * Combination of beam pruning, word end beam pruning, node beam pruning, histogram pruning. Nodes are sorted by the highest likelihood prior to pruning to start with good beam settings
    * Inapproximate and precomputed unigram and bigram lookahead for all the graph types. With the subword n-grams, the bigram look-aheads give the best results. With unlimited vocabulary decoders use the setting `bigram-precomputed-full`, with constrained vocabulary `bigram-precomputed-hybrid`. For large vocabularies the bigram lookahead (`large-bigram`) is computationally quite heavy (long precomputation, high memory requirements and RTF in decoding) and mostly does not improve the results so `unigram` is recommended. This is computationally simple and gives reasonable results. For smaller vocabulary sizes like for English recognition the `bigram-precomputed-full` should be ok

### Notes

The graph construction and lookahead algorithms are pretty well unit tested so there shouldn't be any major problems in those. The accuracy should be equal or slightly better in Finnish tasks with the `<w>` symbols compared to the original AaltoASR decoder. In some cases (for instance Estonian task), however, this decoder may be several percent absolute better. This is due to slightly different silence loops along with the search improvements.

This code passes the tokens by-value. This has the advantage that the decoding code is fairly simple. The recognition history is stored separately. Currently there is no support for lattice generation. In principle it shouldn't be so big effort to implement, but testing for all the graph types could take some time.

I have mainly used this with the traditional simple Finnish pronuncation lexicon, i.e. one pronunciation variant per word. Multiple pronunciation variants should work already, but it is better to test with one pronunciation variant per word at first.

Only triphone models are supported.

Graphs are created by a separate program which is given as input to the `decode` executable.

### Branches
* `master`, normal n-gram decoder for all the graph types
* `class-only`, class n-gram decoder for word-based recognition and the unlimited vocabulary recognizers without `<w>` symbol
* `class-ip`, interpolated n-gram and class n-gram decoder for word-based recognition and the unlimited vocabulary recognizers without `<w>` symbol
* `wsw`, constrained vocabulary word based recognizer with a three-way model interpolation, word n-gram, class n-gram over words, subword n-gram

### Programs
* `cleanlex`, removes words without proper triphones
* `decode`, main decoder
* `lasc`, counts look-ahead states
* `lastates`, precomputes `large-bigram` lookahead scores
* `score`, scores utterances
* `segment`, segments utterances to triphone states, selects the most likely silence path
* `swgraph, normal `<w>` unlimited vocabulary graph
* `swwgraph`, constrained vocabulary subword n-gram graph
* `wgraph`, word graph
* `lwbswgraph`, unlimited vocabulary graph with the left-most subword markers
* `rwbswgraph`, unlimited vocabulary graph with the right-most subword markers
