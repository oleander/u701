set dotenv-load := true

docker-build:
    docker build -t u701 .

build: docker-build
    docker run --rm u701 cargo pio build --workspace

test: docker-build
    docker run --rm u701 cargo test --workspace
