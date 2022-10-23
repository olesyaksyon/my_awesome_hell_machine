cmd_/home/cluster/servo/Module.symvers := sed 's/\.ko$$/\.o/' /home/cluster/servo/modules.order | scripts/mod/modpost -m -a  -o /home/cluster/servo/Module.symvers -e -i Module.symvers   -T -
