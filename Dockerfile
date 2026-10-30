FROM debian:bookworm-slim AS build

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential ca-certificates cmake libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN cmake -S . -B /tmp/pet-build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/pet-build --target Pet_Raising_Server --parallel 2

FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates libssl3 libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /tmp/pet-build/Pet_Raising_Server /usr/local/bin/Pet_Raising_Server
COPY --from=build /app/frontend /app/frontend
COPY --from=build /app/database /app/database

ENV PET_BIND_ADDRESS=0.0.0.0
ENV PET_DATABASE_PATH=/data/virtual_pet.db
ENV VP_SECURE_COOKIES=1

CMD ["sh", "-c", "exec /usr/local/bin/Pet_Raising_Server \"${PORT:-8080}\""]
