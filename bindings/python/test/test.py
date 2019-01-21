# -*- coding: utf-8 -*-

import copy
import pytest
import pyonmttok

def test_simple():
    tokenizer = pyonmttok.Tokenizer(
        "aggressive",
        joiner_annotate=True,
        joiner_new=True)
    text = "Hello World!"
    tokens, features = tokenizer.tokenize(text)
    assert tokens == ["Hello", "World", "￭", "!"]
    assert features is None
    detok = tokenizer.detokenize(tokens)
    assert detok == text

def test_named_arguments():
    tokenizer = pyonmttok.Tokenizer(mode="aggressive", joiner_annotate=True)
    text = "Hello World!"
    tokens, features = tokenizer.tokenize(text=text)
    assert tokens == ["Hello", "World", "￭!"]
    assert text == tokenizer.detokenize(tokens=tokens)

def test_deepcopy():
    text = "Hello World!"
    tok1 = pyonmttok.Tokenizer("aggressive")
    tokens1, _ = tok1.tokenize(text)
    tok2 = copy.deepcopy(tok1)
    tokens2, _ = tok2.tokenize(text)
    assert tokens1 == tokens2
    del tok1
    tokens2, _ = tok2.tokenize(text)
    assert tokens1 == tokens2
