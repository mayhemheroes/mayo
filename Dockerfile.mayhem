FROM --platform=linux/amd64 ubuntu:22.04 as builder

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libocct-data-exchange-dev libocct-draw-dev qtbase5-dev libqt5svg5-dev build-essential

COPY . /repo
WORKDIR /repo
RUN chmod +x ./build.sh
RUN ./build.sh

FROM ubuntu:22.04 as package
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libocct-data-exchange-dev libocct-draw-dev qtbase5-dev
COPY --from=builder /repo/mayo-build/mayo /