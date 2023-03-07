yum -y install gcc gcc-c++ make git automake libaio-devel openssl openssl-devel

echo "***** update gcc *****"
yum -y install centos-release-scl
yum -y install devtoolset-8
scl enable devtoolset-8 bash
