import pyonmttok

tokenizer = pyonmttok.Tokenizer(
    "aggressive",
    joiner_annotate=True,
    joiner_new=True)

tokens, _ = tokenizer.tokenize("Hello World!")
text = tokenizer.detokenize(tokens)

print("tokenized:  ", tokens)
print("detokenized:", text)
