ARG DOCKER_TAG=latest
FROM espressif/idf:${DOCKER_TAG}

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        udev \
        nodejs \
        npm \
        bubblewrap \
        ca-certificates \
        curl \
        git \
        procps \
        ripgrep \
        nano \
    && npm install -g @openai/codex \
    && command -v codex \
    && codex --version \
    && rm -rf /var/lib/apt/lists/*

RUN echo 'source /opt/esp/idf/export.sh > /dev/null 2>&1' >> /root/.bashrc

ENTRYPOINT ["/opt/esp/entrypoint.sh"]
CMD ["/bin/bash"]