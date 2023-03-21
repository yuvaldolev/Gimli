FROM silkeh/clang:15-bullseye AS builder

RUN apt update && apt-get install -y \
  ninja-build

COPY . /app

RUN /app/build.sh


FROM debian:bullseye

COPY data/gimli.tar.gz /var/lib/

RUN mkdir /var/lib/gimli

RUN tar -C /var/lib/gimli -xzf /var/lib/gimli.tar.gz

RUN rm /var/lib/gimli.tar.gz

COPY --from=builder /app/build/gimli/gimli /app/gimli

WORKDIR /app

ENTRYPOINT ["/app/gimli"]
