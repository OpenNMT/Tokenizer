import copy
import os
import pickle

import pytest

import pyonmttok

_DATA_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "test", "data"
)


def test_is_placeholder():
    assert not pyonmttok.is_placeholder("hello")
    assert pyonmttok.is_placeholder("｟hello｠")


def test_simple():
    tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True, joiner_new=True)
    text = "Hello World!"
    tokens, features = tokenizer.tokenize(text)
    assert tokens == ["Hello", "World", "￭", "!"]
    assert features is None
    detok = tokenizer.detokenize(tokens)
    assert detok == text


def test_empty():
    tokenizer = pyonmttok.Tokenizer("conservative")
    assert tokenizer.tokenize("") == ([], None)
    assert tokenizer.detokenize([]) == ""


def test_options():
    options = {
        "mode": "aggressive",
        "joiner_annotate": True,
        "joiner_new": True,
        "joiner": "@@",
        "segment_numbers": True,
        "segment_alphabet": ["Han"],
    }
    tokenizer = pyonmttok.Tokenizer(**options)
    for name, value in options.items():
        assert tokenizer.options[name] == value


def test_invalid_mode():
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("xxx")


def test_invalid_lang():
    with pytest.raises(ValueError, match="ISO"):
        pyonmttok.Tokenizer("conservative", lang="xxx")


def test_invalid_sentencepiece_model():
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("none", sp_model_path="xxx")


def test_invalid_bpe_model():
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("conservative", bpe_model_path="xxx")


def test_invalid_annotation():
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("conservative", joiner_annotate=True, spacer_annotate=True)
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("conservative", joiner_new=True)
    with pytest.raises(ValueError):
        pyonmttok.Tokenizer("conservative", spacer_new=True)


@pytest.mark.parametrize("tokens_delimiter", [" ", "++"])
def test_file(tmpdir, tokens_delimiter):
    tokenizer = pyonmttok.Tokenizer(
        "aggressive",
        joiner_annotate=True,
        joiner_new=True,
        case_feature=True,
    )
    text = "Hello WORLD!"
    expected_tokens = ["hello￨C", "world￨U", "￭￨N", "!￨N"]

    input_path = str(tmpdir.join("input.txt"))
    output_path = str(tmpdir.join("output.txt"))
    with open(input_path, "w", encoding="utf-8") as input_file:
        input_file.write(text)
        input_file.write("\n")

    tokenizer.tokenize_file(input_path, output_path, tokens_delimiter=tokens_delimiter)
    assert os.path.exists(output_path)
    with open(output_path, encoding="utf-8") as output_file:
        assert output_file.readline() == tokens_delimiter.join(expected_tokens) + "\n"
    os.remove(input_path)

    tokenizer.detokenize_file(
        output_path, input_path, tokens_delimiter=tokens_delimiter
    )
    assert os.path.exists(input_path)
    with open(input_path, encoding="utf-8") as input_file:
        assert input_file.readline() == text + "\n"


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
    with open(input_file, "w", encoding="utf-8") as f:
        f.write("Hello world!")
    with pytest.raises(ValueError):
        tokenizer.tokenize_file(input_file, directory)
    with pytest.raises(ValueError):
        tokenizer.detokenize_file(input_file, directory)


def test_custom_joiner():
    tokenizer = pyonmttok.Tokenizer("aggressive", joiner="•", joiner_annotate=True)
    tokens, _ = tokenizer.tokenize("Hello World!")
    assert tokens == ["Hello", "World", "•!"]


def test_segment_alphabet():
    tokenizer = pyonmttok.Tokenizer(mode="aggressive", segment_alphabet=["Han"])
    tokens, _ = tokenizer.tokenize("測試 abc")
    assert tokens == ["測", "試", "abc"]

    tokenizer = pyonmttok.Tokenizer(mode="aggressive", segment_alphabet=None)
    tokens, _ = tokenizer.tokenize("測試 abc")
    assert tokens == ["測試", "abc"]


def test_with_separators():
    tokenizer = pyonmttok.Tokenizer("conservative", with_separators=True)
    tokens, _ = tokenizer.tokenize("Exemple :")  # Non-breaking space
    assert tokens == ["Exemple", " ", ":"]


def test_sp_tokenizer():
    sp_model_path = os.path.join(_DATA_DIR, "sp-models", "wmtende.model")
    tokenizer = pyonmttok.SentencePieceTokenizer(sp_model_path)
    assert isinstance(tokenizer, pyonmttok.Tokenizer)
    assert tokenizer.tokenize("Hello")[0] == ["▁H", "ello"]


def test_sp_with_vocabulary(tmpdir):
    sp_model_path = os.path.join(_DATA_DIR, "sp-models", "wmtende.model")
    vocab_path = str(tmpdir.join("vocab.txt"))
    with open(vocab_path, "w", encoding="utf-8") as vocab_file:
        vocab_file.write("▁Wor\n")

    with pytest.raises(ValueError, match="spacer_annotate"):
        tokenizer = pyonmttok.Tokenizer(
            mode="none",
            sp_model_path=sp_model_path,
            vocabulary_path=vocab_path,
            joiner_annotate=True,
        )

    tokenizer = pyonmttok.Tokenizer(
        mode="none",
        sp_model_path=os.path.join(_DATA_DIR, "sp-models", "wmtende.model"),
        vocabulary_path=vocab_path,
    )
    assert tokenizer.tokenize("World")[0] == ["▁Wor", "l", "d"]


def test_named_arguments():
    tokenizer = pyonmttok.Tokenizer(mode="aggressive", joiner_annotate=True)
    text = "Hello World!"
    tokens, features = tokenizer.tokenize(text=text)
    assert tokens == ["Hello", "World", "￭!"]
    assert text == tokenizer.detokenize(tokens=tokens)


def test_tokenize_batch():
    tokenizer = pyonmttok.Tokenizer("aggressive")
    assert tokenizer.tokenize_batch([]) == ([], [])
    assert tokenizer.tokenize_batch([], as_token_objects=True) == []

    batch_tokens, batch_features = tokenizer.tokenize_batch(["a b c", "d e"])
    assert batch_tokens == [["a", "b", "c"], ["d", "e"]]
    assert batch_features == [None, None]

    tokenizer = pyonmttok.Tokenizer("aggressive", case_feature=True)
    batch_tokens, batch_features = tokenizer.tokenize_batch(
        ["Hello world", "HALLO Welt"]
    )
    assert batch_tokens == [["hello", "world"], ["hallo", "welt"]]
    assert batch_features == [[["C", "L"]], [["U", "C"]]]


@pytest.mark.parametrize("use_constructor", [False, True])
def test_deepcopy(use_constructor):
    text = "Hello World!"
    tok1 = pyonmttok.Tokenizer("aggressive", joiner_annotate=True)
    tokens1, _ = tok1.tokenize(text)
    if use_constructor:
        tok2 = pyonmttok.Tokenizer(tok1)
    else:
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

    _, ranges = tokenizer.detokenize_with_ranges(["测", "试"], unicode_ranges=True)
    assert len(ranges) == 2
    assert ranges[0] == (0, 0)
    assert ranges[1] == (2, 2)

    _, ranges = tokenizer.detokenize_with_ranges(
        ["测", "￭试"], unicode_ranges=True, merge_ranges=True
    )
    assert len(ranges) == 2
    assert ranges[0] == (0, 1)
    assert ranges[1] == (0, 1)


def test_subword_regularization():
    pyonmttok.set_random_seed(42)

    tokenizer = pyonmttok.Tokenizer(
        "none",
        sp_model_path=os.path.join(_DATA_DIR, "sp-models", "wmtende.model"),
        sp_nbest_size=10,
        sp_alpha=0.1,
    )
    assert tokenizer.tokenize("appealing")[0] == ["▁app", "e", "al", "ing"]
    assert tokenizer.tokenize("appealing", training=False)[0] == ["▁appealing"]

    tokenizer = pyonmttok.Tokenizer(
        "conservative",
        bpe_model_path=os.path.join(_DATA_DIR, "bpe-models", "testcode.v0.1"),
        bpe_dropout=0.3,
    )
    assert tokenizer.tokenize("improvement")[0] == [
        "i",
        "m",
        "pr",
        "ove",
        "m",
        "e",
        "n",
        "t",
    ]
    assert tokenizer.tokenize("improvement", training=False)[0] == [
        "impr",
        "ovemen",
        "t",
    ]


def test_bpe_case_insensitive_issue_147():
    tokenizer = pyonmttok.Tokenizer(
        "conservative",
        bpe_model_path=os.path.join(_DATA_DIR, "bpe-models", "issue-147.txt"),
    )
    tokenizer.tokenize("𝘛𝘩𝘦𝘳𝘦'𝘴 𝘯𝘰𝘵𝘩𝘪𝘯𝘨 𝘮𝘰𝘳𝘦 𝘨𝘭𝘢𝘮𝘰𝘳𝘰𝘶𝘴 𝘵𝘩𝘢𝘯 𝘭𝘰𝘰𝘬𝘪𝘯𝘨 𝘵𝘰𝘸𝘢𝘳𝘥𝘴 𝘵𝘩𝘦 𝘧𝘶𝘵𝘶𝘳𝘦")


def test_bpe_learner(tmpdir):
    tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True)
    learner = pyonmttok.BPELearner(tokenizer=tokenizer, symbols=2, min_frequency=1)
    assert isinstance(learner, pyonmttok.SubwordLearner)
    learner.ingest("hello world")
    model_path = str(tmpdir.join("bpe.model"))
    tokenizer = learner.learn(model_path)
    with open(model_path, encoding="utf-8") as model:
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
    with open(model_path, encoding="utf-8") as model:
        assert model.read() == "#version: 0.2\na b</w>\nc d</w>\n"


@pytest.mark.parametrize("keep_vocab", [False, True])
def test_sp_learner(tmpdir, keep_vocab):
    learner = pyonmttok.SentencePieceLearner(
        keep_vocab=keep_vocab, vocab_size=17, character_coverage=0.98
    )
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
    [
        pyonmttok.BPELearner(symbols=2, min_frequency=1),
        pyonmttok.SentencePieceLearner(vocab_size=17, character_coverage=0.98),
    ],
)
def test_learner_with_invalid_files(tmpdir, learner):
    with pytest.raises(ValueError):
        learner.ingest_file("notfound.txt")
    learner.ingest("hello word ! how are you ?")
    directory = tmpdir.join("directory")
    directory.ensure(dir=True)
    with pytest.raises(Exception):
        learner.learn(str(directory))


def test_token_api():
    tokenizer = pyonmttok.Tokenizer(
        "aggressive", joiner_annotate=True, case_markup=True
    )

    text = "Hello WORLD!"
    tokens = tokenizer.tokenize(text, as_token_objects=True)
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
        "￭!",
    ]

    assert all(
        a == b for a, b in zip(tokenizer.deserialize_tokens(serialized_tokens), tokens)
    )

    tokens[0].surface = "toto"
    tokens[0].casing = pyonmttok.Casing.LOWERCASE
    tokens[2].join_left = False
    assert tokenizer.detokenize(tokens) == "toto WORLD !"


def test_token_api_with_subword():
    tokenizer = pyonmttok.Tokenizer(
        "conservative",
        case_markup=True,
        joiner_annotate=True,
        bpe_model_path=os.path.join(
            _DATA_DIR, "bpe-models", "codes_suffix_case_insensitive.fr"
        ),
    )

    text = "BONJOUR MONDE"

    def _check_subword(tokens):
        assert len(tokens) == 5
        assert tokens[0].type == pyonmttok.TokenType.LEADING_SUBWORD  # bon
        assert tokens[1].type == pyonmttok.TokenType.TRAILING_SUBWORD  # j
        assert tokens[2].type == pyonmttok.TokenType.TRAILING_SUBWORD  # our
        assert tokens[3].type == pyonmttok.TokenType.LEADING_SUBWORD  # mon
        assert tokens[4].type == pyonmttok.TokenType.TRAILING_SUBWORD  # de

    tokens = tokenizer.tokenize(text, as_token_objects=True)
    _check_subword(tokens)
    serialized_tokens, _ = tokenizer.serialize_tokens(tokens)

    # Deserialization should not loose subword information.
    tokens = tokenizer.deserialize_tokens(serialized_tokens)
    _check_subword(tokens)
    assert serialized_tokens == tokenizer.serialize_tokens(tokens)[0]


def test_token_deserialize_with_preserved_tokens():
    tokenizer = pyonmttok.Tokenizer(
        "conservative",
        joiner_annotate=True,
        segment_case=True,
        preserve_segmented_tokens=True,
    )
    tokens = tokenizer.tokenize("HelloWorld", as_token_objects=True)
    assert tokenizer.deserialize_tokens(*tokenizer.serialize_tokens(tokens)) == tokens


def test_token_api_features():
    tokenizer = pyonmttok.Tokenizer("space")
    tokens = tokenizer.tokenize("a b", as_token_objects=True)
    assert tokens[0].features == []
    assert tokens[1].features == []

    tokens = tokenizer.tokenize("a￨1 b￨2", as_token_objects=True)
    assert tokens[0].features == ["1"]
    assert tokens[1].features == ["2"]

    tokens, features = tokenizer.serialize_tokens(tokens)
    assert tokens == ["a", "b"]
    assert features == [["1", "2"]]

    # Case features should be deserialized into the casing attribute, not as features.
    tokenizer = pyonmttok.Tokenizer("space", case_feature=True)
    tokens = tokenizer.deserialize_tokens(["hello", "world"], features=[["C", "U"]])
    assert tokens[0].surface == "hello"
    assert tokens[0].casing == pyonmttok.Casing.CAPITALIZED
    assert tokens[0].features == []
    assert tokens[1].surface == "world"
    assert tokens[1].casing == pyonmttok.Casing.UPPERCASE
    assert tokens[1].features == []


def test_token_length():
    assert len(pyonmttok.Token("測試")) == 2


def test_token_copy():
    a = pyonmttok.Token("a")
    b = pyonmttok.Token(a)
    assert b.surface == "a"
    b.surface = "b"
    assert b.surface == "b"
    assert a.surface == "a"


def test_token_dict():
    a = pyonmttok.Token("a")
    b = pyonmttok.Token("b", join_left=True)
    c = pyonmttok.Token("c", features=["X"])
    d = pyonmttok.Token("d")

    collection = {a: 0, b: 1, c: 2}

    assert d not in collection
    for i, token in enumerate((a, b, c)):
        assert collection[token] == i
        # Hashing is based on token equivalence.
        assert collection[pyonmttok.Token(token)] == i


def test_token_repr():
    token = pyonmttok.Token()
    assert repr(token) == "Token()"

    token = pyonmttok.Token("Hello")
    assert repr(token) == "Token('Hello')"

    token = pyonmttok.Token(
        "Hello",
        type=pyonmttok.TokenType.LEADING_SUBWORD,
        casing=pyonmttok.Casing.MIXED,
        join_right=True,
        join_left=True,
        preserve=True,
        features=["X", "Y"],
    )
    assert repr(token) == (
        "Token('Hello', "
        "type=TokenType.LEADING_SUBWORD, "
        "join_left=True, "
        "join_right=True, "
        "preserve=True, "
        "features=['X', 'Y'], "
        "casing=Casing.MIXED)"
    )


def test_token_pickle():
    token = pyonmttok.Token(
        "Hello",
        type=pyonmttok.TokenType.LEADING_SUBWORD,
        casing=pyonmttok.Casing.MIXED,
        join_right=True,
        join_left=True,
        preserve=True,
        features=["X", "Y"],
    )

    data = pickle.dumps(token)
    token2 = pickle.loads(data)
    assert token == token2
