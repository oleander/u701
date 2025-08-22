set dotenv-load := true

docker-build:
    docker build -t u701 .

build: docker-build
    docker run --rm -it u701 bash -c "cargo pio build --workspace"

test: docker-build
    docker run -it --rm u701 bash -c "cargo test --workspace --target aarch64-unknown-linux-gnu"
