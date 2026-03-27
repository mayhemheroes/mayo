FROM --platform=linux/amd64 ubuntu:22.04 as builder

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libocct-data-exchange-dev libocct-draw-dev qtbase5-dev libqt5svg5-dev build-essential clang

COPY . /repo
WORKDIR /repo
RUN chmod +x ./build.sh
RUN ./build.sh
RUN cp /repo/mayo-build/mayo /

RUN rm -rf /repo/mayo-build
RUN cp /repo/mayhem/libmayo.pro /repo/mayo.pro
RUN ./build.sh
WORKDIR /repo/mayhem
RUN qmake fuzz.pro
RUN make
RUN cp /repo/mayhem/fuzz /

FROM ubuntu:22.04 as package
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libocct-data-exchange-dev libocct-draw-dev qtbase5-dev
COPY --from=builder /mayo /
COPY --from=builder /fuzz /fuzz_consoleToPrintable
