FROM centos:7

RUN yum -y install epel-release && yum clean all
RUN yum -y install python-pip
RUN pip install flask

EXPOSE 8000

COPY queue.py /root/queue.py
CMD ["/bin/bash"]
