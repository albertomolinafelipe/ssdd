FROM ubuntu:22.04

# Install build tools and Python
RUN apt-get update && \
    apt-get install -y build-essential python3 python3-pip && \
    pip3 install --no-cache-dir -r /app/requirements.txt || true

WORKDIR /app

COPY . /app

CMD ["/bin/bash"]
