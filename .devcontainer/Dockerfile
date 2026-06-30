ARG DOCKER_TAG=latest
FROM espressif/idf:${DOCKER_TAG}

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        udev \
        nodejs \
        npm \
        bubblewrap \
        ca-certificates \
        curl \
        git \
    && npm install -g @openai/codex \
    && rm -rf /var/lib/apt/lists/*

RUN echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc

ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]

CMD ["/bin/bash", "-c"]