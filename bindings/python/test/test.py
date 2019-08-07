# -*- coding: utf-8 -*-

import os
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

def test_invalid_mode():
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("xxx")

def test_invalid_sentencepiece_model():
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("none", sp_model_path="xxx")

def test_invalid_bpe_model():
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("conservative", bpe_model_path="xxx")

def test_file(tmpdir):
    input_path = str(tmpdir.join("input.txt"))
    output_path = str(tmpdir.join("output.txt"))
    with open(input_path, "w") as input_file:
        input_file.write("Hello world!")
    tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True, joiner_new=True)
    tokenizer.tokenize_file(input_path, output_path)
    assert os.path.exists(output_path)
    with open(output_path) as output_file:
        assert output_file.readline().strip() == "Hello world ￭ !"
    os.remove(input_path)
    tokenizer.detokenize_file(output_path, input_path)
    assert os.path.exists(input_path)
    with open(input_path) as input_file:
        assert input_file.readline().strip() == "Hello world!"

def test_custom_joiner():
    tokenizer = pyonmttok.Tokenizer(
        "aggressive", joiner="•", joiner_annotate=True)
    tokens, _ = tokenizer.tokenize("Hello World!")
    assert tokens == ["Hello", "World", "•!"]

def test_segment_alphabet():
    tokenizer = pyonmttok.Tokenizer(mode="aggressive", segment_alphabet=["Han"])
    tokens, _ = tokenizer.tokenize("測試 abc")
    assert tokens == ["測", "試", "abc"]

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

def test_detok_with_ranges():
    tokenizer = pyonmttok.Tokenizer("conservative")
    text, ranges = tokenizer.detokenize_with_ranges(["a", "b"])
    assert text == "a b"
    assert len(ranges) == 2
    assert ranges[0] == (0, 0)
    assert ranges[1] == (2, 2)

def test_bpe_learner(tmpdir):
    tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True)
    learner = pyonmttok.BPELearner(tokenizer=tokenizer, symbols=2, min_frequency=1)
    learner.ingest("hello world")
    model_path = str(tmpdir.join("bpe.model"))
    tokenizer = learner.learn(model_path)
    with open(model_path) as model:
        assert model.read() == "#version: 0.2\ne l\nel l\n"
    tokens, _ = tokenizer.tokenize("hello")
    assert tokens == ["h￭", "ell￭", "o"]

def test_sp_learner(tmpdir):
    learner = pyonmttok.SentencePieceLearner(vocab_size=17, character_coverage=0.98)
    learner.ingest("hello word! how are you?")
    model_path = str(tmpdir.join("sp.model"))
    tokenizer = learner.learn(model_path)
    tokens, _ = tokenizer.tokenize("hello")
    assert tokens == ["▁h", "e", "l", "l", "o"]
