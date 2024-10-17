#!/bin/bash
validate_ip() {
  # Regular expression pattern for IP address validation
  local ip_pattern='^([0-9]{1,3}.){3}[0-9]{1,3}$'

  if [[ $1 =~ $ip_pattern ]]; then
    return 0  # Valid IP address
  else
    return 1  # Invalid IP address
  fi
}

# Take the replace string
read -p "Enter the kernel IP: " ipKernel
read -p "Enter the memory IP: " ipMemoria
read -p "Enter the fs IP: " ipFS
read -p "Enter the cpu IP: " ipCPU

# Regexs
sKer="IP_KERNEL=[0-9\.]*"
sMem="IP_MEMORIA=[0-9\.]*"
sFs="IP_FILESYSTEM=[0-9\.]*"
sCpu="IP_CPU=[0-9\.]*"

# Kernel
kc0="./kernel/config/kernel.config"
kc1="./configs/kernel/kernel_v1.config"
kc2="./configs/kernel/kernel_v2.config"
kc3="./configs/kernel/kernel_v3.config"
kc4="./configs/kernel/kernel_v4.config"
kc5="./configs/kernel/kernel_v5.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc5
# Search and replace CPU
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc0
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc1
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc2
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc3
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc4
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc5
# Search and replace FS
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc0
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc1
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc2
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc3
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc4
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc5
# Search and replace KERNEL
sed -E -i "s/$sKer/IP_KERNEL=$ipKernel/" $kc0
sed -E -i "s/$sKer/IP_KERNEL=$ipKernel/" $kc1
sed -E -i "s/$sKer/IP_KERNEL=$ipKernel/" $kc2
sed -E -i "s/$sKer/IP_KERNEL=$ipKernel/" $kc3
sed -E -i "s/$sKer/IP_KERNEL=$ipKernel/" $kc4
sed -E -i "s/$sKer/IP_KERNEL=$ipKernel/" $kc5
# Consola
cc0="./consola/config/consola.config"
# Search and replace Kernel
sed -E -i "s/$sKer/IP_KERNEL=$ipKernel/" $cc0

# FS
fsc0="./fileSystem/config/fileSystem.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $fsc0
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $fsc0

# CPU
cpc0="./cpu/config/cpu.config"
cpc1="./configs/cpu/cpu_v1.config"
cpc2="./configs/cpu/cpu_v2.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc2
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $cpc0
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $cpc1
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $cpc2

# MEMORIA
mc0="./memoria/config/memoria.config"
mc1="./memoria_v1/config/memoria.config"
mc2="./memoria_v2/config/memoria.config"
mc3="./memoria_v3/config/memoria.config"
mc4="./memoria_v4/config/memoria.config"
mc5="./memoria_v5/config/memoria.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc5
