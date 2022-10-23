cmd_/home/cluster/sonar/Module.symvers := sed 's/\.ko$$/\.o/' /home/cluster/sonar/modules.order | scripts/mod/modpost -m -a  -o /home/cluster/sonar/Module.symvers -e -i Module.symvers   -T -
