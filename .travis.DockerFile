FROM ubuntu
MAINTAINER tobozo

ENV SRC '/data'
ENV DST '/video'

RUN apt-get -qq update && apt-get install -y sudo docker.io \
    git make btrfs-tools libdevmapper-dev curl \
    libgpgme-dev libostree-dev ffmpeg imagemagick php \
    libav-tools inotify-tools file language-pack-en

RUN mkdir -p /usr/src/M5Tube
WORKDIR /usr/src/M5Tube
COPY . /usr/src/M5Tube
RUN chmod +x /usr/src/M5Tube/workflow.sh

#ADD workflow.sh /usr/src/M5Tube/workflow.sh
#CMD /usr/src/M5Tube/workflow.sh

# VOLUME /video


#RUN tar -C /usr/local -xzf /tmp/go1.9.2.linux-amd64.tar.gz && \
#    rm /tmp/go1.9.2.linux-amd64.tar.gz && \
#ln -s /usr/local/go/bin/* /usr/local/bin/


# From Travis:
#   build container:
#     sudo docker build -t tobozo/M5Tube -f .travis.Dockerfile .
#   create container:
#     docker run -it --name M5Tube tobozo/m5tube-converter bash
#     docker run --name M5Tube -v /local/path:/opt/apache2/www -v /localhost/path2:/any/container/path image_name
#   upload video:
#     docker cp SRC_PATH CONTAINER:DEST_PATH

