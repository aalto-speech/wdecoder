## Wdecoder

This project contains different dynamic token-passing decoders to be used with AaltoASR acoustic models. The acoustic model states are read from a .ph file definition and acoustic probabilities from .lna files. Duration models (.dur) can be used optionally. The project was initially intended for evaluating subword n-grams using a constrained vocabulary. It was also found out that word n-grams work reasonably well especially if used with class n-gram models.

This code is mostly (c) Matti Varjokallio with the exception of the code that is from the AaltoASR package: Acoustics, Hmm, NowayHmmReader, LnaReaderCircular, str, conf.
Aalto internal research use is permitted.
Ask a permission for other purposes. 


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
   
