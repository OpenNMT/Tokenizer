# -*- coding: utf-8 -*-

import os
import copy
import pytest

try:
    # PyTorch is another pybind11 extension that uses a non-compliant toolchain.
    # If available, test that pyonmttok do not crash on import after it.
    import torch
except ImportError:
    pass

import pyonmttok

_DATA_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "test", "data")

def test_is_placeholder():
    assert not pyonmttok.is_placeholder("hello")
    assert pyonmttok.is_placeholder("｟hello｠")

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

def test_invalid_annotation():
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer(
            "conservative",
            joiner_annotate=True,
            spacer_annotate=True)
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("conservative", joiner_new=True)
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("conservative", spacer_new=True)

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

def test_invalid_files(tmpdir):
    tokenizer = pyonmttok.Tokenizer("conservative")
    output_file = str(tmpdir.join("output.txt"))
    with pytest.raises(ValueError):
        tokenizer.tokenize_file("notfound.txt", output_file)
    with pytest.raises(ValueError):
        tokenizer.detokenize_file("notfound.txt", output_file)
    directory = tmpdir.join("directory")
    directory.ensure(dir=True)
    directory = str(directory)
    input_file = str(tmpdir.join("input.txt"))
    with open(input_file, "w") as f:
        f.write("Hello world!")
    with pytest.raises(ValueError):
        tokenizer.tokenize_file(input_file, directory)
    with pytest.raises(ValueError):
        tokenizer.detokenize_file(input_file, directory)

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

def test_bpe_case_insensitive_issue_147():
    tokenizer = pyonmttok.Tokenizer(
        "conservative",
        bpe_model_path=os.path.join(_DATA_DIR, "bpe-models", "issue-147.txt"))
    tokenizer.tokenize("𝘛𝘩𝘦𝘳𝘦'𝘴 𝘯𝘰𝘵𝘩𝘪𝘯𝘨 𝘮𝘰𝘳𝘦 𝘨𝘭𝘢𝘮𝘰𝘳𝘰𝘶𝘴 𝘵𝘩𝘢𝘯 𝘭𝘰𝘰𝘬𝘪𝘯𝘨 𝘵𝘰𝘸𝘢𝘳𝘥𝘴 𝘵𝘩𝘦 𝘧𝘶𝘵𝘶𝘳𝘦")

def test_bpe_learner(tmpdir):
    tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True)
    learner = pyonmttok.BPELearner(tokenizer=tokenizer, symbols=2, min_frequency=1)
    assert isinstance(learner, pyonmttok.SubwordLearner)
    learner.ingest("hello world")
    model_path = str(tmpdir.join("bpe.model"))
    tokenizer = learner.learn(model_path)
    with open(model_path) as model:
        assert model.read() == "#version: 0.2\ne l\nel l\n"
    tokens, _ = tokenizer.tokenize("hello")
    assert tokens == ["h￭", "ell￭", "o"]

def test_bpe_learner_tokens(tmpdir):
    tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True)
    learner = pyonmttok.BPELearner(tokenizer=tokenizer, symbols=2, min_frequency=1)
    learner.ingest_token("ab￭")
    token = pyonmttok.Token("cd")
    learner.ingest_token(token)
    model_path = str(tmpdir.join("bpe.model"))
    learner.learn(model_path)
    with open(model_path) as model:
        assert model.read() == "#version: 0.2\na b</w>\nc d</w>\n"

@pytest.mark.parametrize("keep_vocab", [False, True])
def test_sp_learner(tmpdir, keep_vocab):
    learner = pyonmttok.SentencePieceLearner(
        keep_vocab=keep_vocab, vocab_size=17, character_coverage=0.98)
    assert isinstance(learner, pyonmttok.SubwordLearner)
    learner.ingest("hello word! how are you?")
    model_path = str(tmpdir.join("sp"))
    tokenizer = learner.learn(model_path)
    if keep_vocab:
        assert os.path.exists(model_path + ".model")
        assert os.path.exists(model_path + ".vocab")
    else:
        assert os.path.exists(model_path)
    tokens, _ = tokenizer.tokenize("hello")
    assert tokens == ["▁h", "e", "l", "l", "o"]

@pytest.mark.parametrize(
    "learner",
    [pyonmttok.BPELearner(symbols=2, min_frequency=1),
     pyonmttok.SentencePieceLearner(vocab_size=17, character_coverage=0.98)])
def test_learner_with_invalid_files(tmpdir, learner):
    with pytest.raises(ValueError):
        learner.ingest_file("notfound.txt")
    learner.ingest("hello word ! how are you ?")
    directory = tmpdir.join("directory")
    directory.ensure(dir=True)
    with pytest.raises(Exception):
        learner.learn(str(directory))

def test_token_api():
    tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True, case_markup=True)

    text = "Hello WORLD!"
    tokens = tokenizer.tokenize(text, as_tokens=True)
    assert len(tokens) == 3
    for token in tokens:
        assert isinstance(token, pyonmttok.Token)

    assert tokens[0].surface == "hello"
    assert tokens[0].casing == pyonmttok.Casing.CAPITALIZED
    assert tokens[1].surface == "world"
    assert tokens[1].casing == pyonmttok.Casing.UPPERCASE
    assert tokens[2].surface == "!"
    assert tokens[2].join_left

    assert tokenizer.detokenize(tokens) == text
    detokenized_text, ranges = tokenizer.detokenize_with_ranges(tokens)
    assert detokenized_text == text
    assert list(sorted(ranges.keys())) == [0, 1, 2]

    serialized_tokens, _ = tokenizer.serialize_tokens(tokens)
    assert serialized_tokens == [
        "｟mrk_case_modifier_C｠",
        "hello",
        "｟mrk_begin_case_region_U｠",
        "world",
        "｟mrk_end_case_region_U｠",
        "￭!"
    ]

    assert all(a == b for a, b in zip(tokenizer.deserialize_tokens(serialized_tokens), tokens))

    tokens[0].surface = "toto"
    tokens[0].casing = pyonmttok.Casing.LOWERCASE
    tokens[2].join_left = False
    assert tokenizer.detokenize(tokens) == "toto WORLD !"

def test_token_api_with_subword():
    tokenizer = pyonmttok.Tokenizer(
        "conservative",
        case_markup=True,
        joiner_annotate=True,
        bpe_model_path=os.path.join(_DATA_DIR, "bpe-models", "codes_suffix_case_insensitive.fr"))

    text = "BONJOUR MONDE"
    def _check_subword(tokens):
        assert len(tokens) == 5
        assert tokens[0].type == pyonmttok.TokenType.LEADING_SUBWORD   # bon
        assert tokens[1].type == pyonmttok.TokenType.TRAILING_SUBWORD  # j
        assert tokens[2].type == pyonmttok.TokenType.TRAILING_SUBWORD  # our
        assert tokens[3].type == pyonmttok.TokenType.LEADING_SUBWORD   # mon
        assert tokens[4].type == pyonmttok.TokenType.TRAILING_SUBWORD  # de

    tokens = tokenizer.tokenize(text, as_tokens=True)
    _check_subword(tokens)
    serialized_tokens, _ = tokenizer.serialize_tokens(tokens)

    # Deserialization should not loose subword information.
    tokens = tokenizer.deserialize_tokens(serialized_tokens)
    _check_subword(tokens)
    assert serialized_tokens == tokenizer.serialize_tokens(tokens)[0]
