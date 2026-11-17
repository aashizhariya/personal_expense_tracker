FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    make \
    curl \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Download the missing cpp-httplib single-header library
RUN curl -sL https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h -o httplib.h

RUN make

EXPOSE 8080
CMD ["./tracker"]
