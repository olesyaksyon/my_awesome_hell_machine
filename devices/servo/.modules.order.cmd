cmd_/home/cluster/servo/modules.order := {   echo /home/cluster/servo/servo.ko; :; } | awk '!x[$$0]++' - > /home/cluster/servo/modules.order
