set dotenv-load := true

build:
    docker build -t u701 .

test: build
    docker run --rm u701 cargo test --workspace
