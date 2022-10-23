cmd_/home/cluster/sonar/modules.order := {   echo /home/cluster/sonar/sonar.ko; :; } | awk '!x[$$0]++' - > /home/cluster/sonar/modules.order
