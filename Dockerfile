FROM ubuntu:22.04
RUN apt update && apt install -y g++ make libreadline-dev
WORKDIR /app
COPY . .
RUN make
CMD ["./bin/myshell"] 
