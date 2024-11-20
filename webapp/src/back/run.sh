cd /home/dipsomask/Документы/sit/lr6/webapp/src/back/ && \
./main &
cd /home/dipsomask/Документы/sit/lr6/webapp/src/back/compilledWorkers/ && (
    gnome-terminal -- bash -c './w65001 65001; exec bash' &
    gnome-terminal -- bash -c './w65002 65002; exec bash' & 
    gnome-terminal -- bash -c './w65003 65003; exec bash' &
    gnome-terminal -- bash -c './w65004 65004; exec bash' & 
    gnome-terminal -- bash -c './w65005 65005; exec bash' &
) && echo "app is running..." || echo "app isn't running..."
