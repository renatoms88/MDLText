# MDLText
----------

Quick links to this file:

* [Introduction](#introduction)
* [How to use mdl-train](#how-to-use-mdl-train)
* [How to use mdl-classify](#how-to-use-mdl-classify)
* [Examples](#examples)
* [Datasets used in the reported experiments](examples/libsvm_format/textCorpora/)
* [Additional Information](#additional-information)

## Introduction
The MDLText is a text classifier based on the minimum description length principle. 

The MDLText can be tested with raw text documents or preprocessed documents stored in [LIBSVM format](https://www.csie.ntu.edu.tw/~cjlin/libsvm/). Moreover, the algorithm provides preprocessing modules, such as text normalization and stop-words removing.


## How to use mdl-train

------------------------------------------------------------------------------
Usage: ```mdl-train [options] [input_fileName] [model_fileName]``` 

```
input_fileName: 
   Relative path to a text file. Such file can be just one text sample to be trained, a index
   file with the paths to a set of samples, a file with a sample per line in the format
   <class>,<text> or a file in libsvm format  

model_fileName: 
    Name given to output model created by MDL after training

Options:
    -i input_type : set type of input file (default 0)  
        0 -- the path to just one text document  
        1 -- the path to a text file which has a list of paths to text documents  
        2 -- the path to a text file where each line is a sample in the format <class>,<text>      
        3 -- the path to a file in LIBSVM format
        4 -- a string
   -c class: document class (necessary only when input_type = 0 or input_type = 4)
   -w term weighting scheme: set the term weighting scheme (default 1)
   	    0 -- if input type is a path to a file in LIBSVM format, it will be used the weigths shown in the file,
   			otherwise it will be used the raw term-frequency (TF) weighting scheme
   	    1 -- term frequency-inverse document frequency (TF-IDF)
   	    2 -- binary
   -b batch_tfidf : set wheter the TF-IDF weight will be calculated in batch learning or does not (default 1)
   			(necessary only when term weighting scheme = 1)
       0 -- false: the TF-IDF weigth will be calculated incrementally
       1 -- true: the TF-IDF will be calculated in batch, that is by using information of all training documents
   -t tokenizer_id : set the type of tokenizer (default 1)
       1 -- tokenizer A: Convert any non-alphanumeric char to whitespace and tokenize by space
       2 -- tokenizer B: Tokenize by {. , ; space enter return tab} and preserve the first
                         and last chars. The remainder ones are kept if they are alphanumeric
   -n apply_normalization : (default 0)
       0 -- false (don't normalize words, e.g. 'going' -> 'go')
       1 -- true (apply text normalization)
   -r remove_stopWords : (default 1)
       0 -- false (don't remove the stop words)
       1 -- true (remove the stop words)
   -s save_type : how model should be updated (default 0)
       0 -- the model is updated only after all documents are trained
       1 -- the model is updated after each document is trained
``` 


## How to use mdl-classify

------------------------------------------------------------------------------
Usage: ```mdl-classify [options] [input_fileName] [model_fileName] [output_fileName]```

```
input_fileName:
   Relative path to a text file. Such file can be just one text sample to be trained, a index
   file with the paths to a set of samples, a file with a sample per line in the format
   <class>,<text> or a file in libsvm format

model_fileName:
   File name of the model used by MDL to classify the messages

Options:

   -i input_type : set the type of input file (default 0)
       0 -- the path to just one text document
       1 -- the path to a text file which has a list of paths to text documents
       2 -- the path to a text file where each line is a sample
       3 -- the path to a file in LIBSVM format
       4 -- a string
   -w term weighting scheme: set the term weighting scheme (default 1)
   	    0 -- if input type is a path to a file in LIBSVM format, it will be used the weigths shown in the file,
   			otherwise it will be used the raw term-frequency (TF) weighting scheme
   	    1 -- term frequency-inverse document frequency (TF-IDF)
   	    2 -- binary
   -t tokenizer_id : set the type of tokenizer (default 1)
       1 -- tokenizer A: Convert any non-alphanumeric char to whitespace and tokenize by space
       2 -- tokenizer B: Tokenize by {. , ; space enter return tab} and preserve the first
                         and last chars. The remainder ones are kept if they are alphanumeric
   -n apply_normalization : (default 0)
       0 -- false (don't normalize words, e.g. 'going' -> 'go')
       1 -- true (apply text normalization)
   -r remove_stopWords : (default 1)
       0 -- false (don't remove the stop words)
       1 -- true (remove the stop words)
   -f feature_relevance_function : function to calculate the relevance of tokens (default CF)
       CF -- Confidence Factors
       DFS -- Distinguishing Feature Selector
       NO -- not use any function
   -o omega : set omega (vocabulary size) (default 2^10) 
```

## Examples

We provide some text collections in folder ```examples/```


To employ MDL classifier on polarityReview text collection in which each sample is a text file:

* For training:
		
		./mdl-train -i 1 examples/polarityReview/polarityReview_train models/mdl_polarityReview.mod
		
* For classifying:
		
		./md-classify -i 1 examples/polarityReview/polarityReview_test models/mdl_polarityReview.mod results/mdlCF_polarityReview.res
		


To employ MDL classifier on SMS Spam Collection in which each sample is a line of a text file:

* For training:
		
		./mdl-train -i 2 examples/SMSSpamCollection/smsspamcollection_train models/mdl_SMS.mod
		
* For classifying:
		
		./mdl-classify -i 2 examples/SMSSpamCollection/smsspamcollection_test models/mdl_SMS.mod results/mdlCF_SMS.res
		

To employ MDL classifier on datasets stored in LIBSVM format:

* For training:
		
		./mdl-train -i 3 examples/libsvm_format/reuters_train.libsvm models/mdl_reuters.mod
		
* For classifying:
		
		./mdl-classify -i 3 examples/libsvm_format/reuters_test.libsvm models/mdl_reuters.mod results/mdlCF_reuters.res

To employ MDL classifier on a text string:

* For training:
		
		./mdl-train -i 4 -c spam "check out the real poker online at this cool site" models/mdl_string.mod
		
* For classifying:
		
		./mdl-classify -i 4 "this is a site where you can find cool things to buy" models/mdl_string.mod results/mdlCF_string.res

## Additional Information
If you find MDLText helpful, please cite it as:

Silva, R. M., Almeida, T. A., & Yamakami, A. (2017). MDLText: An efficient and lightweight text classifier. Knowledge-Based Systems, 118, 152-164.
doi:[http://dx.doi.org/10.1016/j.knosys.2016.11.018](http://dx.doi.org/10.1016/j.knosys.2016.11.018).

* BibTeX:
		
    @article{silva-almeida-yamakami-knosys:2017,
    	author = {Renato M. Silva and Tiago A. Almeida and Akebo Yamakami},
    	title = {{MDLText}: An efficient and lightweight text classifier},
    	journal = {Knowledge-Based Systems},
    	volume = {118},
    	pages = {152--164},
    	year = {2017},
    	month = feb,
    	issn = {0950-7051},
    	doi = {http://dx.doi.org/10.1016/j.knosys.2016.11.018},
    	publisher={Elsevier}
    }
        


