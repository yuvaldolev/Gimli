FROM silkeh/clang:15-bullseye AS builder

RUN apt update && apt-get install -y \
  ninja-build

COPY . /app

RUN /app/build.sh


FROM debian:bullseye

COPY --from=builder /app/build/gimli/gimli /app/gimli

WORKDIR /app

ENTRYPOINT ["/app/gimli"]
