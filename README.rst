Sage Forker
===========

See `this sage-devel thread <http://groups.google.com/group/sage-devel/browse_thread/thread/eb8748e2fff0b73d>`_ for the original posting by Jeroen Demeyer about ``sage-forker``.  Here are Jeroen's instructions:

First of all, you need to compile::

    $ gcc fsage.c -o fsage 

Then, start up the mothersage::

    $ sage 
    ---------------------------------------------------------------------- 
    | Sage Version 4.6.1, Release Date: 2011-01-11                       | 
    | Type notebook() for the GUI, and license() for information.        | 
    ---------------------------------------------------------------------- 
    sage: load "sage-forker.pyx" 
    Compiling ./sage-forker.pyx... 
    sage: sage_forker() 

Now open a new terminal and watch the Sage prompt appear without delay::

    $ ./fsage 
    ---------------------------------------------------------------------- 
    | Sage Version 4.6.1, Release Date: 2011-01-11                       | 
    | Type notebook() for the GUI, and license() for information.        | 
    ---------------------------------------------------------------------- 
    sage: 
